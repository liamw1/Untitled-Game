#include "GMpch.h"
#include "World.h"
#include "ChunkRenderer.h"

constexpr uint8_t modulo(int64_t a, uint8_t b)
{
  const int result = a % b;
  return static_cast<uint8_t>(result >= 0 ? result : result + b);
}

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
  glm::vec3 playerPosition = player.getPosition();
  ChunkIndex playerChunkIndex = Chunk::ChunkIndexFromPos(playerPosition);
  Chunk* playerChunk = m_ChunkManager.findChunk(playerChunkIndex);

  static const int playerWidth = static_cast<int>(ceil(Player::Width() / Block::Length()));
  static const int playerHeight = static_cast<int>(ceil(Player::Height() / Block::Length()));
  glm::vec3 anchorPoint = playerPosition - 0.5f * glm::vec3(Player::Width(), Player::Width(), Player::Height());
  for (int i = 0; i <= playerWidth; ++i)
    for (int j = 0; j <= playerWidth; ++j)
      for (int k = 0; k <= playerHeight; ++k)
      {
        glm::vec3 cornerPos = anchorPoint + Block::Length() * glm::vec3(i, j, k);

        if (Chunk::ChunkIndexFromPos(cornerPos) == playerChunkIndex && playerChunk != nullptr)
        {
          BlockType block = playerChunk->getBlockType(cornerPos);
          if (Block::HasCollision(block))
          {
            LocalIndex blockIndex = playerChunk->blockIndexFromPos(cornerPos);
            glm::vec3 blockAnchor = playerChunk->anchorPoint() + Block::Length() * glm::vec3(blockIndex.i, blockIndex.j, blockIndex.k);

            auto intersection = castRaySegment(cornerPos - player.getVelocity() * dt, cornerPos);
            if (!intersection.first)
              EN_ERROR("No intersection found!");
            else
            {
              switch (intersection.second)
              {
              case BlockFace::East:   EN_TRACE("Collision: East");   break;
              case BlockFace::West:   EN_TRACE("Collision: West");   break;
              case BlockFace::North:  EN_TRACE("Collision: North");  break;
              case BlockFace::South:  EN_TRACE("Collision: South");  break;
              case BlockFace::Top:    EN_TRACE("Collision: Top");    break;
              case BlockFace::Bottom: EN_TRACE("Collision: Bottom"); break;
              default: EN_ERROR("Unknown block face");
              }

              const uint8_t faceID = static_cast<uint8_t>(intersection.second);
              glm::vec3 displacement = -glm::dot(normals[faceID], player.getVelocity() * dt) * normals[faceID];
              player.setPosition(playerPosition + displacement);

              goto endCollisionDetection;
            }
          }
        }
      }

  endCollisionDetection:;

  player.updateEnd();

  ChunkRenderer::BeginScene(playerCamera);

  if (m_RenderingPaused)
    m_ChunkManager.render(playerCamera);
  else
  {
    m_ChunkManager.updatePlayerChunk(playerCamera.getPosition());

    m_ChunkManager.clean();
    m_ChunkManager.render(playerCamera);
    m_ChunkManager.loadNewChunks(200);
  }

  ChunkRenderer::EndScene();
}

bool World::onKeyPressEvent(Engine::KeyPressEvent& event)
{
  if (event.getKeyCode() == Key::F3)
    m_RenderingPaused = !m_RenderingPaused;

  return false;
}

std::pair<bool, BlockFace> World::castRaySegment(const glm::vec3& pointA, const glm::vec3& pointB) const
{
  static constexpr glm::vec3 normals[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

  glm::vec3 rayDirection = pointB - pointA;
  
  // Find plane intersections in the x,y,z directions
  float tmin = 1.1f;
  std::pair<bool, BlockFace> firstCollision = std::pair(false, BlockFace::First);
  for (uint8_t i = 0; i < 3; ++i)
  {
    int64_t n0, nf;
    if (pointB[i] >= pointA[i])
    {
      n0 = static_cast<int64_t>(floor(pointA[i] / Block::Length()));
      nf = static_cast<int64_t>(ceil(pointB[i] / Block::Length()));
    }
    else
    {
      n0 = static_cast<int64_t>(floor(pointB[i] / Block::Length()));
      nf = static_cast<int64_t>(ceil(pointA[i] / Block::Length()));
    }

    for (int64_t n = n0; n <= nf; ++n)
    {
      float t = (n * Block::Length() - pointA[i]) / rayDirection[i];

      // if (t >= 1.0f)
      //   break;

      if (t >= 0.0f && t <= 1.0f && t < tmin)
      {
        const uint8_t u = i;
        const uint8_t v = (i + 1) % 3;
        const uint8_t w = (i + 2) % 3;
        glm::vec3 intersection = pointA + t * rayDirection;

        int64_t N = n;
        bool alignedWithPositiveNormal = glm::dot(rayDirection, normals[u]) >= 0.0f;
        if (!alignedWithPositiveNormal)
          N--;

        // Don't intialize these this way dummy!
        ChunkIndex chunkIndex;
        chunkIndex[u] = N / Chunk::Size();
        chunkIndex[v] = static_cast<int64_t>(floor(intersection[v] / Chunk::Length()));
        chunkIndex[w] = static_cast<int64_t>(floor(intersection[w] / Chunk::Length()));
        if (N < 0)
          chunkIndex[u]--;

        LocalIndex localIndex;
        localIndex[u] = modulo(N, Chunk::Size());
        localIndex[v] = modulo(static_cast<int64_t>(floor(intersection[v] / Block::Length())), Chunk::Size());
        localIndex[w] = modulo(static_cast<int64_t>(floor(intersection[w] / Block::Length())), Chunk::Size());

        Chunk* chunk = m_ChunkManager.findChunk(chunkIndex);
        if (chunk == nullptr)
          continue;

        if (Block::HasCollision(chunk->getBlockType(localIndex)))
        {
          tmin = t;

          uint8_t faceID = 2 * u + alignedWithPositiveNormal;
          firstCollision = std::pair(true, static_cast<BlockFace>(faceID));
        }
      }
    }
  }

  return firstCollision;
}

void World::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));
}
