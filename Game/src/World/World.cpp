#include "GMpch.h"
#include "World.h"
#include "ChunkRenderer.h"
#include "Engine/Renderer/Renderer.h"

static constexpr uint8_t modulo(int64_t a, uint8_t b)
{
  const int result = a % b;
  return static_cast<uint8_t>(result >= 0 ? result : result + b);
}

void World::initialize()
{
  while (m_ChunkManager.loadNewChunks(10000))
  {
  }
}

void World::onUpdate(std::chrono::duration<seconds> timestep)
{
  EN_PROFILE_FUNCTION();

  /* Player update stage */
  Player::UpdateBegin(timestep);
  playerCollisionHandling(timestep);
  Player::UpdateEnd();
  
  m_PlayerRayCast = castRay(Player::Camera().getPosition(), Player::ViewDirection(), 1000 * Block::Length());
  if (m_PlayerRayCast.intersectionOccured)
  {
    Engine::Renderer::BeginScene(Player::Camera());
    const BlockIndex& blockIndex = m_PlayerRayCast.blockIndex;
    const LocalIndex& chunkIndex = m_PlayerRayCast.chunkIndex;
    const Vec3 blockCenter = Chunk::Length() * Vec3(chunkIndex.i, chunkIndex.j, chunkIndex.k) + Block::Length() * (Vec3(blockIndex.i, blockIndex.j, blockIndex.k) + Vec3(0.5));
    Engine::Renderer::DrawCubeFrame(blockCenter, 1.01 * Block::Length() * Vec3(1.0), Float4(0.1f, 0.1f, 0.1f, 1.0f));
    Engine::Renderer::EndScene();
  }

  if (Engine::Input::IsMouseButtonPressed(MouseButton::Button0) || Engine::Input::IsMouseButtonPressed(MouseButton::Button1))
  {
    if (m_PlayerRayCast.intersectionOccured)
    {
      const LocalIndex& chunkIndex = m_PlayerRayCast.chunkIndex;
      const BlockIndex& blockIndex = m_PlayerRayCast.blockIndex;
      const BlockFace& face = m_PlayerRayCast.face;
      Chunk* const chunk = m_ChunkManager.findChunk(chunkIndex);

      if (chunk != nullptr)
      {
        if (Engine::Input::IsMouseButtonPressed(MouseButton::Button0))
        {
          chunk->removeBlock(blockIndex);
          m_ChunkManager.sendChunkUpdate(chunk);

          for (BlockFace face : BlockFaceIterator())
          {
            const uint8_t faceID = static_cast<uint8_t>(face);
            const uint8_t coordID = faceID / 2;

            if (blockIndex[coordID] == (faceID % 2 == 0 ? Chunk::Size() - 1 : 0))
              m_ChunkManager.sendChunkUpdate(chunk->getNeighbor(face));
          }
        }
        else if (m_PlayerRayCast.distance > 2.5 * Block::Length())
        {
          chunk->placeBlock(blockIndex, face, BlockType::Sand);
          m_ChunkManager.sendChunkUpdate(chunk);

          if (chunk->isBlockNeighborInAnotherChunk(blockIndex, face))
            m_ChunkManager.sendChunkUpdate(chunk->getNeighbor(face));
        }
      }
    }
  }

  /* Rendering stage */
  if (!m_RenderingPaused)
  {
    m_ChunkManager.loadNewChunks(200);
    m_ChunkManager.render();
    m_ChunkManager.clean();
  }
  else
    m_ChunkManager.render();
}

Intersection World::castRaySegment(const Vec3& pointA, const Vec3& pointB) const
{
  static constexpr Vec3 normals[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

  const Vec3 rayDirection = pointB - pointA;

  // Look for solid block face intersections in the x,y,z directions
  length_t tmin = 2.0;
  Intersection firstIntersection{};
  for (uint8_t i = 0; i < 3; ++i)
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
        const uint8_t u = i;
        const uint8_t v = (i + 1) % 3;
        const uint8_t w = (i + 2) % 3;

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
          const uint8_t faceID = 2 * u + alignedWithPositiveAxis;
          tmin = t;

          firstIntersection.intersectionOccured = true;
          firstIntersection.face = static_cast<BlockFace>(faceID);
          firstIntersection.blockIndex = blockIndex;
          firstIntersection.chunkIndex = chunkIndex;

          continue;
        }
      }
    }
  }

  if (firstIntersection.intersectionOccured)
    firstIntersection.distance = tmin * glm::length(rayDirection);

  // Return first intersection
  return firstIntersection;
}

Intersection World::castRay(const Vec3& rayOrigin, const Vec3& rayDirection, length_t maxDistance) const
{
  const Vec3& pointA = rayOrigin;
  const Vec3  pointB = rayOrigin + maxDistance * glm::normalize(rayDirection);
  return castRaySegment(pointA, pointB);
}

void World::playerCollisionHandling(std::chrono::duration<seconds> timestep) const
{
  EN_PROFILE_FUNCTION();

  static constexpr Vec3 normals[6] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
                                       //      East         West        North       South         Top        Bottom

  // Player width and height in blocks
  static const int playerWidth = static_cast<int>(ceil(Player::Width() / Block::Length()));
  static const int playerHeight = static_cast<int>(ceil(Player::Height() / Block::Length()));
  const seconds dt = timestep.count();  // Time between frames in seconds

beginCollisionDetection:;
  const Vec3 playerPosition = Player::Position();
  const length_t distanceMoved = dt * glm::length(Player::Velocity());
  const Vec3 anchorPoint = playerPosition - 0.5 * Vec3(Player::Width(), Player::Width(), Player::Height());

  if (distanceMoved == 0.0)
    return;

  glm::vec3 correctCorner{};
  Intersection firstCollision = { false, 2 * distanceMoved, BlockFace::First, { 0, 0, 0 }, { 0, 0, 0 } };
  for (int i = 0; i <= playerWidth; ++i)
    for (int j = 0; j <= playerWidth; ++j)
      for (int k = 0; k <= playerHeight; ++k)
      {
        const Vec3 cornerPos = anchorPoint + Block::Length() * Vec3(i == playerWidth ? i - 0.2 : i, j == playerWidth ? j - 0.2 : j, k == playerHeight ? k - 0.2 : k);

        const Intersection collision = castRaySegment(cornerPos - Player::Velocity() * dt, cornerPos);
        const length_t& collisionDistance = collision.distance;

        if (collision.intersectionOccured && collision.distance < firstCollision.distance)
        {
          firstCollision = collision;
          correctCorner = cornerPos;
        }
      }

  if (firstCollision.intersectionOccured)
  {
    const uint8_t faceID = static_cast<uint8_t>(firstCollision.face);
    const length_t t = firstCollision.distance / distanceMoved;

    // Calculate distance player should be pushed out from solid block
    const Vec3 collisionDisplacement = ((1.0 - t) * glm::dot(normals[faceID], -Player::Velocity() * dt) + s_MinDistanceToWall) * normals[faceID];
    Player::SetPosition(playerPosition + collisionDisplacement);

    goto beginCollisionDetection;
  }
}

void World::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::MouseButtonPressEvent>(EN_BIND_EVENT_FN(onMouseButtonPressEvent));
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));
}

bool World::onMouseButtonPressEvent(Engine::MouseButtonPressEvent& event)
{
#if 0
  if (m_PlayerRayCast.intersectionOccured)
  {
    const LocalIndex& chunkIndex = m_PlayerRayCast.chunkIndex;
    const BlockIndex& blockIndex = m_PlayerRayCast.blockIndex;
    const BlockFace&  face = m_PlayerRayCast.face;
    Chunk* const chunk = m_ChunkManager.findChunk(chunkIndex);

    if (chunk != nullptr)
    {
      if (event.getMouseButton() == MouseButton::Button0)
      {
        chunk->removeBlock(blockIndex);
        m_ChunkManager.sendChunkUpdate(chunk);

        for (BlockFace face : BlockFaceIterator())
        {
          const uint8_t faceID = static_cast<uint8_t>(face);
          const uint8_t coordID = faceID / 2;

          if (blockIndex[coordID] == (faceID % 2 == 0 ? Chunk::Size() - 1 : 0))
            m_ChunkManager.sendChunkUpdate(chunk->getNeighbor(face));
        }
      }
      else if (event.getMouseButton() == MouseButton::Button1)
      {
        chunk->placeBlock(blockIndex, face, BlockType::Sand);
        m_ChunkManager.sendChunkUpdate(chunk);

        if (chunk->isBlockNeighborInAnotherChunk(blockIndex, face))
          m_ChunkManager.sendChunkUpdate(chunk->getNeighbor(face));
      }
    }
  }
#endif

  return false;
}

bool World::onKeyPressEvent(Engine::KeyPressEvent& event)
{
  if (event.getKeyCode() == Key::F3)
    m_RenderingPaused = !m_RenderingPaused;

  return false;
}