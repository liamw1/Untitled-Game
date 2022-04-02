#include "GMpch.h"
#include "World.h"
#include "Engine/Renderer/Renderer.h"

static constexpr uint8_t modulo(int64_t a, uint8_t b)
{
  const int result = a % b;
  return static_cast<uint8_t>(result >= 0 ? result : result + b);
}

void World::initialize()
{
  m_ChunkManager.initialize();
}

void World::onUpdate(Timestep timestep)
{
  EN_PROFILE_FUNCTION();

  /* Player update stage */
  Player::UpdateBegin(timestep);
  playerCollisionHandling(timestep);
  Player::UpdateEnd();

  /* Rendering stage */
  Engine::Renderer::BeginScene(Engine::Scene::ActiveCameraViewProjection());
  if (!m_RenderingPaused)
  {
    // m_ChunkManager.manageLODs();
    // m_ChunkManager.renderLODs();
    // Engine::RenderCommand::ClearDepthBuffer();
    m_ChunkManager.loadNewChunks(200);
    m_ChunkManager.render();
    m_ChunkManager.clean();
  }
  else
  {
    m_ChunkManager.renderLODs();
    Engine::RenderCommand::ClearDepthBuffer();
    m_ChunkManager.render();
  }

  playerWorldInteraction();
  Engine::Renderer::EndScene();
}

RayIntersection World::castRaySegment(const Vec3& pointA, const Vec3& pointB) const
{
  const Vec3 rayDirection = pointB - pointA;

  // Look for solid block face intersections in the x,y,z directions
  length_t tmin = 2.0;
  RayIntersection firstIntersection{};
  for (int i = 0; i < 3; ++i)
  {
    const bool alignedWithPositiveAxis = rayDirection[i] > 0.0;

    // Global indices of first and last planes that segment will intersect in direction i
    int64_t n0, nf;
    if (alignedWithPositiveAxis)
    {
      n0 = static_cast<int64_t>(ceil(pointA[i] / Block::Length()));
      nf = static_cast<int64_t>(ceil(pointB[i] / Block::Length()));
    }
    else
    {
      n0 = static_cast<int64_t>(floor(pointA[i] / Block::Length()));
      nf = static_cast<int64_t>(floor(pointB[i] / Block::Length()));
    }

    // n increases to nf if ray is aligned with positive i-axis and decreases to nf otherwise
    for (int64_t n = n0; alignedWithPositiveAxis ? n <= nf : n >= nf; alignedWithPositiveAxis ? ++n : --n)
    {
      const length_t t = (n * Block::Length() - pointA[i]) / rayDirection[i];

      if (t > 1.0 || !isfinite(t))
        break;

      if (t < tmin)
      {
        // Relabeling coordinate indices
        const int u = i;
        const int v = (i + 1) % 3;
        const int w = (i + 2) % 3;

        // Intersection point between ray and plane
        const Vec3 intersection = pointA + t * rayDirection;

        // If ray hit West/South/Bottom block face, we can use n for block coordinate, otherwise, we need to step back a block
        int64_t N = n;
        if (!alignedWithPositiveAxis)
          N--;

        // Get index of chunk in which intersection took place
        LocalIndex chunkIndex{};
        chunkIndex[u] = static_cast<localIndex_t>(N / Chunk::Size());
        chunkIndex[v] = static_cast<localIndex_t>(floor(intersection[v] / Chunk::Length()));
        chunkIndex[w] = static_cast<localIndex_t>(floor(intersection[w] / Chunk::Length()));
        if (N < 0 && N % Chunk::Size() != 0)
          chunkIndex[u]--;

        // Get local index of block that was hit by ray
        BlockIndex blockIndex{};
        blockIndex[u] = modulo(N, Chunk::Size());
        blockIndex[v] = modulo(static_cast<int64_t>(floor(intersection[v] / Block::Length())), Chunk::Size());
        blockIndex[w] = modulo(static_cast<int64_t>(floor(intersection[w] / Block::Length())), Chunk::Size());

        // Search to see if chunk is loaded
        const Chunk* chunk = m_ChunkManager.findChunk(chunkIndex);
        if (chunk == nullptr)
          continue;

        // If block has collision, note the intersection and move to next spatial direction
        if (Block::HasCollision(chunk->getBlockType(blockIndex)))
        {
          const int faceID = 2 * u + !alignedWithPositiveAxis;
          tmin = t;

          firstIntersection.face = static_cast<Block::Face>(faceID);
          firstIntersection.blockIndex = blockIndex;
          firstIntersection.chunkIndex = chunkIndex;

          continue;
        }
      }
    }
  }

  if (tmin <= 1.0)
    firstIntersection.distance = tmin * glm::length(rayDirection);

  // Return first intersection
  return firstIntersection;
}

RayIntersection World::castRay(const Vec3& rayOrigin, const Vec3& rayDirection, length_t maxDistance) const
{
  const Vec3& pointA = rayOrigin;
  const Vec3  pointB = rayOrigin + maxDistance * glm::normalize(rayDirection);
  return castRaySegment(pointA, pointB);
}

void World::playerCollisionHandling(Timestep timestep) const
{
  EN_PROFILE_FUNCTION();

  static constexpr Vec3 normals[6] = { { -1, 0, 0}, { 1, 0, 0}, { 0, -1, 0}, { 0, 1, 0}, { 0, 0, -1}, { 0, 0, 1} };
                                  //       West        East        South        North       Bottom        Top

  // Player width and height in blocks
  static const int playerWidth = static_cast<int>(ceil(Player::Width() / Block::Length()));
  static const int playerHeight = static_cast<int>(ceil(Player::Height() / Block::Length()));
  seconds dt = timestep.sec();  // Time between frames in seconds

beginCollisionDetection:;
  length_t distanceMoved = dt * glm::length(Player::Velocity());
  Vec3 anchorPoint = Player::Position() - 0.5 * Vec3(Player::Width(), Player::Width(), Player::Height());

  if (distanceMoved == 0.0)
    return;

  glm::vec3 correctCorner{};
  RayIntersection firstCollision{};
  for (int i = 0; i <= playerWidth; ++i)
    for (int j = 0; j <= playerWidth; ++j)
      for (int k = 0; k <= playerHeight; ++k)
      {
        Vec3 cornerPos = anchorPoint + Block::Length() * Vec3(i == playerWidth ? i - 0.2 : i, j == playerWidth ? j - 0.2 : j, k == playerHeight ? k - 0.2 : k);
        RayIntersection collision = castRaySegment(cornerPos - dt * Player::Velocity(), cornerPos);

        if (collision.distance < firstCollision.distance)
        {
          firstCollision = collision;
          correctCorner = cornerPos;
        }
      }

  if (firstCollision.distance <= distanceMoved)
  {
    int faceID = static_cast<int>(firstCollision.face);
    length_t t = firstCollision.distance / distanceMoved;

    // Calculate distance player should be pushed out from solid block
    Vec3 collisionDisplacement = ((1.0 - t) * dt *  glm::dot(normals[faceID], -Player::Velocity()) + s_MinDistanceToWall) * normals[faceID];

    Player::SetPosition(Player::Position() + collisionDisplacement);

    // Velocity becomes (prevPlayerPosition - positionAfterCollision) / dt
    Player::SetVelocity((Player::Velocity() + collisionDisplacement / dt));

    goto beginCollisionDetection;
  }
}

void World::playerWorldInteraction()
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };
  static constexpr length_t maxInteractionDistance = 1000 * Block::Length();

  m_PlayerRayCast = castRay(Player::Position(), Player::ViewDirection(), maxInteractionDistance);
  if (m_PlayerRayCast.distance <= maxInteractionDistance)
  {
    const BlockIndex& blockIndex = m_PlayerRayCast.blockIndex;
    const LocalIndex& chunkIndex = m_PlayerRayCast.chunkIndex;
    Vec3 blockCenter = Chunk::Length() * static_cast<Vec3>(chunkIndex) + Block::Length() * (static_cast<Vec3>(blockIndex) + Vec3(0.5));

    Engine::Renderer::DrawCubeFrame(blockCenter, 1.01 * Block::Length() * Vec3(1.0), Float4(0.1f, 0.1f, 0.1f, 1.0f));
  }

  if (Engine::Input::IsMouseButtonPressed(Mouse::ButtonLeft) || Engine::Input::IsMouseButtonPressed(Mouse::ButtonRight))
  {
    if (m_PlayerRayCast.distance <= maxInteractionDistance)
    {
      const LocalIndex& chunkIndex = m_PlayerRayCast.chunkIndex;
      const BlockIndex& blockIndex = m_PlayerRayCast.blockIndex;
      const Block::Face& rayCastFace = m_PlayerRayCast.face;
      Chunk* chunk = m_ChunkManager.findChunk(chunkIndex);

      if (chunk != nullptr)
      {
        if (Engine::Input::IsMouseButtonPressed(Mouse::ButtonLeft))
        {
          chunk->removeBlock(blockIndex);
          m_ChunkManager.sendChunkUpdate(chunk);

          for (Block::Face face : Block::FaceIterator())
          {
            const int faceID = static_cast<int>(face);
            const int coordID = faceID / 2;

            if (blockIndex[coordID] == chunkLimits[faceID % 2])
              m_ChunkManager.sendChunkUpdate(chunk->getNeighbor(face));
          }
        }
        else if (m_PlayerRayCast.distance > 2.5 * Block::Length())
        {
          chunk->placeBlock(blockIndex, rayCastFace, Block::Type::Snow);
          m_ChunkManager.sendChunkUpdate(chunk);

          if (chunk->isBlockNeighborInAnotherChunk(blockIndex, rayCastFace))
            m_ChunkManager.sendChunkUpdate(chunk->getNeighbor(rayCastFace));
        }
      }
    }
  }
}

void World::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));
}

bool World::onKeyPressEvent(Engine::KeyPressEvent& event)
{
  if (event.getKeyCode() == Key::F3)
    m_RenderingPaused = !m_RenderingPaused;

  return false;
}