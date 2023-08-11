#include "GMpch.h"
#include "World.h"
#include "Player/Player.h"

#define USE_LODS false

static constexpr bool createLODs = false;

static constexpr blockIndex_t modulo(globalIndex_t a, blockIndex_t b)
{
  const int result = a % b;
  return static_cast<blockIndex_t>(result >= 0 ? result : result + b);
}

void World::initialize()
{
  m_ChunkManager.initialize();

  m_ChunkManager.setLoadModeTerrain();
  // m_ChunkManager.loadChunk({ 0, -1, 2 }, Block::Type::Air);
  // m_ChunkManager.loadChunk({ 0, -1, 1 }, Block::Type::Sand);
  m_ChunkManager.launchLoadThread();
  m_ChunkManager.launchUpdateThread();

#if USE_LODS
  if (createLODs)
    m_ChunkManager.initializeLODs();
#endif
}

void World::onUpdate(Engine::Timestep timestep)
{
  EN_PROFILE_FUNCTION();

  /* Player update stage */
  Player::HandleDirectionalInput();
  playerCollisionHandling(timestep);
  Player::UpdatePosition(timestep);

  /* Rendering stage */
  Engine::Renderer::BeginScene(Engine::Scene::ActiveCamera());

  m_ChunkManager.render();

#if USE_LODS
  if (createLODs)
  {
    m_ChunkManager.manageLODs();
    m_ChunkManager.renderLODs();
    Engine::RenderCommand::ClearDepthBuffer();
  }
#endif

  playerWorldInteraction();

  Engine::Renderer::EndScene();

  m_ChunkManager.update();
  m_ChunkManager.clean();
}

RayIntersection World::castRaySegment(const Vec3& pointA, const Vec3& pointB) const
{
  const Vec3 rayDirection = pointB - pointA;

  // Look for solid block face intersections in the x,y,z directions
  length_t tmin = 2.0;
  RayIntersection firstIntersection{};
  for (int i = 0; i < 3; ++i)
  {
    const bool pointedUpstream = rayDirection[i] > 0.0;

    // Global indices of first and last planes that segment will intersect in direction i
    globalIndex_t n0, nf;
    if (pointedUpstream)
    {
      n0 = static_cast<globalIndex_t>(ceil(pointA[i] / Block::Length()));
      nf = static_cast<globalIndex_t>(ceil(pointB[i] / Block::Length()));
    }
    else
    {
      n0 = static_cast<globalIndex_t>(floor(pointA[i] / Block::Length()));
      nf = static_cast<globalIndex_t>(floor(pointB[i] / Block::Length()));
    }

    // n increases to nf if ray is aligned with positive i-axis and decreases to nf otherwise
    for (globalIndex_t n = n0; pointedUpstream ? n <= nf : n >= nf; pointedUpstream ? ++n : --n)
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
        Vec3 intersection = pointA + t * rayDirection;

        // If ray hit West/South/Bottom block face, we can use n for block coordinate, otherwise, we need to step back a block
        globalIndex_t N = n;
        if (!pointedUpstream)
          N--;

        // Get index of chunk in which intersection took place
        LocalIndex chunkIndex = LocalIndex::CreatePermuted(static_cast<localIndex_t>(N / Chunk::Size()),
                                                           static_cast<localIndex_t>(floor(intersection[v] / Chunk::Length())),
                                                           static_cast<localIndex_t>(floor(intersection[w] / Chunk::Length())), i);
        if (N < 0 && N % Chunk::Size() != 0)
          chunkIndex[u]--;

        // Get local index of block that was hit by ray
        BlockIndex blockIndex = BlockIndex::CreatePermuted(modulo(N, Chunk::Size()),
                                                           modulo(static_cast<globalIndex_t>(floor(intersection[v] / Block::Length())), Chunk::Size()),
                                                           modulo(static_cast<globalIndex_t>(floor(intersection[w] / Block::Length())), Chunk::Size()), i);

        // Search to see if chunk is loaded
        auto [chunk, lock] = m_ChunkManager.acquireChunk(chunkIndex);
        if (!chunk || !chunk->composition())
          continue;

        // If block has collision, note the intersection and move to next spatial direction
        if (Block::HasCollision(chunk->composition()(blockIndex)))
        {
          int faceID = 2 * u + !pointedUpstream;
          tmin = t;

          firstIntersection.face = static_cast<Direction>(faceID);
          firstIntersection.blockIndex = blockIndex;
          firstIntersection.chunkIndex = chunkIndex;
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

void World::playerCollisionHandling(Engine::Timestep timestep) const
{
  static constexpr int maxIterations = 100;

  // Player width and height in blocks
  static const int playerWidth = static_cast<int>(ceil(Player::Width() / Block::Length()));
  static const int playerHeight = static_cast<int>(ceil(Player::Height() / Block::Length()));
  static const length_t widthInterval = Player::Width() / playerWidth;
  static const length_t heightInterval = Player::Height() / playerHeight;
  seconds dt = timestep.sec();  // Time between frames in seconds

  Vec3 playerPosition = Player::Position();
  Vec3 playerVelocity = Player::Velocity();

  int iterations = 0;
  while (iterations++ < maxIterations)
  {
    length_t distanceMoved = dt * glm::length(playerVelocity);
    Vec3 anchorPoint = playerPosition - 0.5 * Vec3(Player::Width(), Player::Width(), Player::Height());

    if (distanceMoved == 0.0)
      break;

    RayIntersection firstCollision{};
    for (int i = 0; i <= playerWidth; ++i)
      for (int j = 0; j <= playerWidth; ++j)
        for (int k = 0; k <= playerHeight; ++k)
        {
          Vec3 samplePosition = anchorPoint + Vec3(i * widthInterval, j * widthInterval, k * heightInterval);
          RayIntersection collision = castRaySegment(samplePosition, samplePosition + playerVelocity * dt);

          if (collision.distance < firstCollision.distance)
            firstCollision = collision;
        }

    if (distanceMoved < firstCollision.distance)
      break;

    Vec3 blockFaceNormal = static_cast<Vec3>(BlockIndex::Dir(firstCollision.face));
    length_t velocityComponentAlongBlockNormal = glm::dot(playerVelocity, -blockFaceNormal);

    length_t distanceToMinDistanceToWall = firstCollision.distance - c_MinDistanceToWall * distanceMoved / velocityComponentAlongBlockNormal;
    length_t velocityScaling = distanceToMinDistanceToWall / distanceMoved;

    playerPosition += velocityScaling * playerVelocity * dt;
    playerVelocity += velocityComponentAlongBlockNormal * blockFaceNormal;
  }

  if (iterations >= maxIterations)
    EN_ERROR("Player collision handling exceeded maximum number of iterations!");

  Player::SetPosition(playerPosition);
  Player::SetVelocity(playerVelocity);
}

void World::playerWorldInteraction()
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };
  static constexpr length_t maxInteractionDistance = 1000 * Block::Length();

  m_PlayerRayCast = castRay(Player::CameraPosition(), Player::ViewDirection(), maxInteractionDistance);
  if (m_PlayerRayCast.distance <= maxInteractionDistance)
  {
    const BlockIndex& blockIndex = m_PlayerRayCast.blockIndex;
    const LocalIndex& chunkIndex = m_PlayerRayCast.chunkIndex;
    Vec3 blockCenter = Chunk::Length() * static_cast<Vec3>(chunkIndex) + Block::Length() * (static_cast<Vec3>(blockIndex) + Vec3(0.5));

    Engine::Renderer::DrawCubeFrame(blockCenter, 1.01 * Block::Length() * Vec3(1.0), Float4(0.1f, 0.1f, 0.1f, 1.0f));
  }

  if (m_PlayerRayCast.distance <= maxInteractionDistance)
  {
    const LocalIndex& localIndex = m_PlayerRayCast.chunkIndex;
    const BlockIndex& blockIndex = m_PlayerRayCast.blockIndex;
    const Direction& rayCastFace = m_PlayerRayCast.face;

    GlobalIndex originChunk = Player::OriginIndex();
    GlobalIndex chunkIndex(originChunk.i + localIndex.i, originChunk.j + localIndex.j, originChunk.k + localIndex.k);

    if (Engine::Input::IsMouseButtonPressed(Mouse::ButtonLeft))
      m_ChunkManager.removeBlock(chunkIndex, blockIndex);
    else if (Engine::Input::IsMouseButtonPressed(Mouse::ButtonRight) && m_PlayerRayCast.distance > 2.5 * Block::Length())
      m_ChunkManager.placeBlock(chunkIndex, blockIndex, rayCastFace, Block::Type::Glass);
  }
}

void World::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));
}

bool World::onKeyPressEvent(Engine::KeyPressEvent& event)
{
  if (event.keyCode() == Key::F3)
    m_RenderingPaused = !m_RenderingPaused;

  return false;
}