#include "GMpch.h"
#include "World.h"
#include "ChunkRenderer.h"

static constexpr uint8_t modulo(int64_t a, uint8_t b)
{
  const int result = a % b;
  return static_cast<uint8_t>(result >= 0 ? result : result + b);
}

static constexpr int p = modulo(-32, 32);

World::World(const glm::vec3& initialPosition)
{
  m_ChunkManager.updatePlayerChunk(initialPosition);

  while (m_ChunkManager.loadNewChunks(10000))
  {
  }
}

void World::onUpdate(std::chrono::duration<float> timestep, Player& player)
{
  EN_PROFILE_FUNCTION();

  static constexpr glm::vec3 normals[6] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
                                       //      East         West        North       South         Top        Bottom

  const float dt = timestep.count();  // Time between frames in seconds
  const Engine::Camera& playerCamera = player.getCamera();

  player.updateBegin(timestep);

  /* Player collision detection */
beginCollisionDetection:;

  const glm::vec3 playerPosition = player.getPosition();

  static const int playerWidth = static_cast<int>(ceil(Player::Width() / Block::Length()));
  static const int playerHeight = static_cast<int>(ceil(Player::Height() / Block::Length()));
  const glm::vec3 anchorPoint = playerPosition - 0.5f * glm::vec3(Player::Width(), Player::Width(), Player::Height());

  float tmin = 2.0f;
  BlockFace firstCollision = BlockFace::First;
  for (int i = 0; i <= playerWidth; ++i)
    for (int j = 0; j <= playerWidth; ++j)
      for (int k = 0; k <= playerHeight; ++k)
      {
        glm::vec3 cornerPos = anchorPoint + Block::Length() * glm::vec3(i, j, k);

        auto intersection = castRaySegment(cornerPos - player.getVelocity() * dt, cornerPos);
        const float& t = intersection.first;
        const BlockFace& face = intersection.second;

        if (t < tmin)
        {
          tmin = t;
          firstCollision = face;
        }
      }

  if (tmin <= 1.0f)
  {
#if 0
    switch (firstCollision)
    {
      case BlockFace::East:   EN_TRACE("Collision: East");   break;
      case BlockFace::West:   EN_TRACE("Collision: West");   break;
      case BlockFace::North:  EN_TRACE("Collision: North");  break;
      case BlockFace::South:  EN_TRACE("Collision: South");  break;
      case BlockFace::Top:    EN_TRACE("Collision: Top");    break;
      case BlockFace::Bottom: EN_TRACE("Collision: Bottom"); break;
      default: EN_ERROR("Unknown block face");
    }
#endif

    const uint8_t faceID = static_cast<uint8_t>(firstCollision);
    glm::vec3 displacement = -glm::dot(normals[faceID], player.getVelocity() * dt) * normals[faceID];
    player.setPosition(playerPosition + displacement);

    goto beginCollisionDetection;
  }

  player.updateEnd();

  /* Rendering stage */
  if (!m_RenderingPaused)
  {
    m_ChunkManager.updatePlayerChunk(playerCamera.getPosition());

    m_ChunkManager.clean();
    m_ChunkManager.render(playerCamera);
    m_ChunkManager.loadNewChunks(200);
  }
  else
    m_ChunkManager.render(playerCamera);
}

bool World::onKeyPressEvent(Engine::KeyPressEvent& event)
{
  if (event.getKeyCode() == Key::F3)
    m_RenderingPaused = !m_RenderingPaused;

  return false;
}

std::pair<float, BlockFace> World::castRaySegment(const glm::vec3& pointA, const glm::vec3& pointB) const
{
  static constexpr glm::vec3 normals[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

  const glm::vec3 rayDirection = pointB - pointA;
  
  // Find solid block face intersections in the x,y,z directions
  float tmin = 2.0f;
  BlockFace firstIntersection = BlockFace::First;
  for (uint8_t i = 0; i < 3; ++i)
  {
    const bool alignedWithPositiveAxis = rayDirection[i] > 0.0f;

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
      const float t = (n * Block::Length() - pointA[i]) / rayDirection[i];

      if (t > 1.0f || !isfinite(t))
        break;

      if (t < tmin)
      {
        // Relabeling coordinate indices
        const uint8_t u = i;
        const uint8_t v = (i + 1) % 3;
        const uint8_t w = (i + 2) % 3;

        // Intersection point between ray and plane
        const glm::vec3 intersection = pointA + t * rayDirection;

        // If ray hit West/South/Bottom block face, we can use n for block coordinate, otherwise, we need to step back a block
        int64_t N = n;
        if (!alignedWithPositiveAxis)
          N--;

        // Get index of chunk in which intersection took place
        ChunkIndex chunkIndex{};
        chunkIndex[u] = N / Chunk::Size();
        chunkIndex[v] = static_cast<int64_t>(floor(intersection[v] / Chunk::Length()));
        chunkIndex[w] = static_cast<int64_t>(floor(intersection[w] / Chunk::Length()));
        if (N < 0 && N % Chunk::Size() != 0)
          chunkIndex[u]--;

        // Get local index of block that was hit by ray
        LocalIndex localIndex{};
        localIndex[u] = modulo(N, Chunk::Size());
        localIndex[v] = modulo(static_cast<int64_t>(floor(intersection[v] / Block::Length())), Chunk::Size());
        localIndex[w] = modulo(static_cast<int64_t>(floor(intersection[w] / Block::Length())), Chunk::Size());

        // Search to see if chunk is loaded
        const Chunk* chunk = m_ChunkManager.findChunk(chunkIndex);
        if (chunk == nullptr)
          continue;

        // If block has collision, note the intersection and move to next spatial direction
        if (Block::HasCollision(chunk->getBlockType(localIndex)))
        {
          tmin = t;

          const uint8_t faceID = 2 * u + alignedWithPositiveAxis;
          firstIntersection = static_cast<BlockFace>(faceID);

          continue;
        }
      }
    }
  }

  // Return first intersection
  return std::pair(tmin, firstIntersection);
}

void World::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));
}
