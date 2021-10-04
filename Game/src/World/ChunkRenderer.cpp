#include "ChunkRenderer.h"
#include "Chunk.h"
#include "Block/Block.h"
#include <glm/gtc/matrix_transform.hpp>

/*
  Renderer 2D data
*/
static Engine::Shared<Engine::Shader> s_BlockFaceShader;
static Engine::Shared<Engine::Texture2D> s_TextureAtlas;
constexpr static uint8_t s_TextureSlot = 1;



void ChunkRenderer::Initialize()
{
  EN_PROFILE_FUNCTION();

  s_BlockFaceShader = Engine::Shader::Create("assets/shaders/BlockFace.glsl");
  s_BlockFaceShader->bind();
  s_BlockFaceShader->setInt("u_TextureAtlas", s_TextureSlot);

  s_TextureAtlas = Engine::Texture2D::Create("assets/textures/C-tetra_1.7/blocks/sand.png");
}

void ChunkRenderer::BeginScene(Engine::Camera& camera)
{
  s_BlockFaceShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());
  s_TextureAtlas->bind(s_TextureSlot);
  s_BlockFaceShader->bind();
}

void ChunkRenderer::EndScene()
{
}

void ChunkRenderer::DrawChunk(const Chunk* chunk)
{
  uint32_t meshIndexCount = 6 * (uint32_t)chunk->getMesh().size() / 4;

  if (meshIndexCount == 0)
    return; // Nothing to draw

  chunk->bindBuffers();
  s_BlockFaceShader->setFloat3("u_ChunkPosition", chunk->position());
  Engine::RenderCommand::DrawIndexed(chunk->getVertexArray(), meshIndexCount);
}