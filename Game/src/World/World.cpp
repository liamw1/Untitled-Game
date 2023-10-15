#include "GMpch.h"
#include "World.h"
#include "Player/Player.h"

static constexpr blockIndex_t modulo(globalIndex_t a, blockIndex_t b)
{
  const int result = a % b;
  return static_cast<blockIndex_t>(result >= 0 ? result : result + b);
}

void World::initialize()
{
  player::initialize(GlobalIndex(0, 0, 2), block::length() * eng::math::Vec3(16.0));
  m_ChunkManager.initialize();
}

void World::onUpdate(eng::Timestep timestep)
{
  EN_PROFILE_FUNCTION();

  /* Player update stage */
  player::handleDirectionalInput();
  playerCollisionHandling(timestep);
  player::updatePosition(timestep);

  /* Rendering stage */
  eng::renderer::beginScene(eng::scene::ActiveCamera());

  playerWorldInteraction();
  m_ChunkManager.update();
  m_ChunkManager.render();

  eng::renderer::endScene();

  m_ChunkManager.loadNewChunks();
  m_ChunkManager.clean();
}

RayIntersection World::castRaySegment(const eng::math::Vec3& pointA, const eng::math::Vec3& pointB) const
{
  const eng::math::Vec3 rayDirection = pointB - pointA;

  // Look for solid block face intersections in the x,y,z directions
  length_t tmin = 2.0;
  RayIntersection firstIntersection{};
  for (eng::math::Axis axis : eng::math::Axes())
  {
    int axisID = static_cast<int>(axis);
    bool pointedUpstream = rayDirection[axisID] > 0.0;

    // Global indices of first and last planes that segment will intersect in direction i
    globalIndex_t n0, nf;
    if (pointedUpstream)
    {
      n0 = static_cast<globalIndex_t>(ceil(pointA[axisID] / block::length()));
      nf = static_cast<globalIndex_t>(ceil(pointB[axisID] / block::length()));
    }
    else
    {
      n0 = static_cast<globalIndex_t>(floor(pointA[axisID] / block::length()));
      nf = static_cast<globalIndex_t>(floor(pointB[axisID] / block::length()));
    }

    // n increases to nf if ray is aligned with positive i-axis and decreases to nf otherwise
    for (globalIndex_t n = n0; pointedUpstream ? n <= nf : n >= nf; pointedUpstream ? ++n : --n)
    {
      const length_t t = (n * block::length() - pointA[axisID]) / rayDirection[axisID];

      if (t > 1.0 || !isfinite(t))
        break;

      if (t < tmin)
      {
        // Relabeling coordinate indices
        int u = axisID;
        int v = (u + 1) % 3;
        int w = (u + 2) % 3;

        // Intersection point between ray and plane
        eng::math::Vec3 intersection = pointA + t * rayDirection;

        // If ray hit West/South/Bottom block face, we can use n for block coordinate, otherwise, we need to step back a block
        globalIndex_t N = n;
        if (!pointedUpstream)
          N--;

        // Get index of chunk in which intersection took place
        LocalIndex chunkIndex = LocalIndex::CreatePermuted(static_cast<localIndex_t>(N / Chunk::Size()),
                                                           static_cast<localIndex_t>(floor(intersection[v] / Chunk::Length())),
                                                           static_cast<localIndex_t>(floor(intersection[w] / Chunk::Length())), axis);
        if (N < 0 && N % Chunk::Size() != 0)
          chunkIndex[static_cast<eng::math::Axis>(u)]--;

        // Get local index of block that was hit by ray
        BlockIndex blockIndex = BlockIndex::CreatePermuted(modulo(N, Chunk::Size()),
                                                           modulo(static_cast<globalIndex_t>(floor(intersection[v] / block::length())), Chunk::Size()),
                                                           modulo(static_cast<globalIndex_t>(floor(intersection[w] / block::length())), Chunk::Size()), axis);

        // Search to see if chunk is loaded
        std::shared_ptr<const Chunk> chunk = m_ChunkManager.getChunk(chunkIndex);
        if (!chunk)
          continue;

        // If block has collision, note the intersection and move to next spatial direction
        if (chunk->composition().get(blockIndex).hasCollision())
        {
          int faceID = 2 * u + !pointedUpstream;
          tmin = t;

          firstIntersection.face = static_cast<eng::math::Direction>(faceID);
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

RayIntersection World::castRay(const eng::math::Vec3& rayOrigin, const eng::math::Vec3& rayDirection, length_t maxDistance) const
{
  const eng::math::Vec3& pointA = rayOrigin;
  const eng::math::Vec3  pointB = rayOrigin + maxDistance * glm::normalize(rayDirection);
  return castRaySegment(pointA, pointB);
}

void World::playerCollisionHandling(eng::Timestep timestep) const
{
  static constexpr int maxIterations = 100;

  // Player width and height in blocks
  static const int playerWidth = static_cast<int>(ceil(player::width() / block::length()));
  static const int playerHeight = static_cast<int>(ceil(player::height() / block::length()));
  static const length_t widthInterval = player::width() / playerWidth;
  static const length_t heightInterval = player::height() / playerHeight;
  seconds dt = timestep.sec();  // Time between frames in seconds

  eng::math::Vec3 playerPosition = player::position();
  eng::math::Vec3 playerVelocity = player::velocity();

  int iterations = 0;
  while (iterations++ < maxIterations)
  {
    length_t distanceMoved = dt * glm::length(playerVelocity);
    eng::math::Vec3 anchorPoint = playerPosition - 0.5 * eng::math::Vec3(player::width(), player::width(), player::height());

    if (distanceMoved == 0.0)
      break;

    RayIntersection firstCollision{};
    for (int i = 0; i <= playerWidth; ++i)
      for (int j = 0; j <= playerWidth; ++j)
        for (int k = 0; k <= playerHeight; ++k)
        {
          eng::math::Vec3 samplePosition = anchorPoint + eng::math::Vec3(i * widthInterval, j * widthInterval, k * heightInterval);
          RayIntersection collision = castRaySegment(samplePosition, samplePosition + playerVelocity * dt);

          if (collision.distance < firstCollision.distance)
            firstCollision = collision;
        }

    if (distanceMoved < firstCollision.distance)
      break;

    eng::math::Vec3 blockFaceNormal = static_cast<eng::math::Vec3>(BlockIndex::Dir(firstCollision.face));
    length_t velocityComponentAlongBlockNormal = glm::dot(playerVelocity, -blockFaceNormal);

    length_t distanceToMinDistanceToWall = firstCollision.distance - c_MinDistanceToWall * distanceMoved / velocityComponentAlongBlockNormal;
    length_t velocityScaling = distanceToMinDistanceToWall / distanceMoved;

    playerPosition += velocityScaling * playerVelocity * dt;
    playerVelocity += velocityComponentAlongBlockNormal * blockFaceNormal;
  }

  if (iterations >= maxIterations)
    EN_ERROR("Player collision handling exceeded maximum number of iterations!");

  player::setPosition(playerPosition);
  player::setVelocity(playerVelocity);
}

void World::playerWorldInteraction()
{
  EN_PROFILE_FUNCTION();

  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };
  static constexpr length_t maxInteractionDistance = 1000 * block::length();

  m_PlayerRayCast = castRay(player::cameraPosition(), player::viewDirection(), maxInteractionDistance);
  if (m_PlayerRayCast.distance <= maxInteractionDistance)
  {
    const BlockIndex& blockIndex = m_PlayerRayCast.blockIndex;
    const LocalIndex& chunkIndex = m_PlayerRayCast.chunkIndex;
    eng::math::Vec3 blockCenter = Chunk::Length() * static_cast<eng::math::Vec3>(chunkIndex) + block::length() * (static_cast<eng::math::Vec3>(blockIndex) + eng::math::Vec3(0.5));

    eng::renderer::drawCubeFrame(blockCenter, 1.01 * block::length() * eng::math::Vec3(1.0), eng::math::Float4(0.1f, 0.1f, 0.1f, 1.0f));
  }

  if (m_PlayerRayCast.distance <= maxInteractionDistance)
  {
    const LocalIndex& localIndex = m_PlayerRayCast.chunkIndex;
    const BlockIndex& blockIndex = m_PlayerRayCast.blockIndex;
    const eng::math::Direction& rayCastFace = m_PlayerRayCast.face;

    GlobalIndex originChunk = player::originIndex();
    GlobalIndex chunkIndex(originChunk.i + localIndex.i, originChunk.j + localIndex.j, originChunk.k + localIndex.k);

    if (eng::input::isMouseButtonPressed(eng::input::Mouse::ButtonLeft))
      m_ChunkManager.removeBlock(chunkIndex, blockIndex);
    else if (eng::input::isMouseButtonPressed(eng::input::Mouse::ButtonRight) && m_PlayerRayCast.distance > 2.5 * block::length())
      m_ChunkManager.placeBlock(chunkIndex, blockIndex, rayCastFace, block::ID::Clay);
  }
}

void World::onEvent(eng::event::Event& event)
{
  eng::event::EventDispatcher dispatcher(event);
  dispatcher.dispatch<eng::event::KeyPress>(&World::onKeyPress, this);
}

bool World::onKeyPress(eng::event::KeyPress& event)
{
  if (event.keyCode() == eng::input::Key::F3)
    m_RenderingPaused = !m_RenderingPaused;

  return false;
}