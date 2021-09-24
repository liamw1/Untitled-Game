#include "ENpch.h"
#include "SubTexture.h"

namespace Engine
{
  SubTexture2D::SubTexture2D(const Shared<Texture2D>& spriteSheet, const glm::vec2& minCoord, const glm::vec2& maxCoord)
    : m_SpriteSheet(spriteSheet)
  {
    m_TexCoords[0] = { minCoord.x, minCoord.y };
    m_TexCoords[1] = { maxCoord.x, minCoord.y };
    m_TexCoords[2] = { maxCoord.x, maxCoord.y };
    m_TexCoords[3] = { minCoord.x, maxCoord.y };
  }

  Shared<SubTexture2D> SubTexture2D::CreateFromIndices(const Shared<Texture2D>& spriteSheet, uint32_t cellSize, uint32_t i, uint32_t j, const glm::vec2& spriteSize)
  {
    const float dx = spriteSize.x * cellSize / spriteSheet->getWidth();
    const float dy = spriteSize.y * cellSize / spriteSheet->getHeight();
    const float x = (float)i * cellSize / spriteSheet->getWidth();
    const float y = (float)j * cellSize / spriteSheet->getHeight();

    EN_CORE_ASSERT(x >= 0.0f && x + dx <= 1.0f && y >= 0.0f && y + dy <= 1.0f, "SubTexture coordinates are out of bounds!");

    return createShared<SubTexture2D>(spriteSheet, glm::vec2(x, y), glm::vec2(x + dx, y + dy));
  }
}
