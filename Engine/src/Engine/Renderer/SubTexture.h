#pragma once
#include "Texture.h"
#include <glm/glm.hpp>

namespace Engine
{
  class SubTexture2D
  {
  public:
    SubTexture2D(const Shared<Texture2D>& spriteSheet, const glm::vec2& minCoord, const glm::vec2& maxCoord);

    const Shared<Texture2D> getSpriteSheet() const { return m_SpriteSheet; }
    const glm::vec2* getTextureCoordinates() const { return m_TexCoords; }

    /*
      Creates a SubTexture with texture coordinates corresponding to the indices given.
      Assumes that the sprite sheet is divided into equally-sized regions of size "cellSize".
      i and j refer to the coordinates of the bottom-right corner of the desired sprite,
      which could potentially be a size of mxn cells.
     
      \param spriteSheet An atlas of 2D textures.
      \param cellSize Size of smallest subdivision of the spriteSheet.
      \param i The x-coord of the sprite in units of cellSize.
      \param j The y-coord of the sprite in units of cellSize.
      \param spriteSize The dimensions of the sprite in units of cellSize.
    */
    static Shared<SubTexture2D> CreateFromIndices(const Shared<Texture2D>& spriteSheet, uint32_t cellSize, uint32_t i, uint32_t j, const glm::vec2& spriteSize = { 1.0f, 1.0f });

  private:
    Shared<Texture2D> m_SpriteSheet;
    glm::vec2 m_TexCoords[4];
  };
}