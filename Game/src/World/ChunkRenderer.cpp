#include "GMpch.h"
#include "ChunkRenderer.h"
#include "Chunk.h"
#include "Block/Block.h"

/*
  Renderer 2D data
*/
static Engine::Shared<Engine::Shader> s_BlockFaceShader;
static Engine::Shared<Engine::TextureArray> s_TextureArray;
static constexpr uint8_t s_TextureSlot = 0;



void ChunkRenderer::Initialize()
{
  EN_PROFILE_FUNCTION();

  s_BlockFaceShader = Engine::Shader::Create("assets/shaders/BlockFace.glsl");
  s_BlockFaceShader->bind();
  s_BlockFaceShader->setInt("u_TextureArray", s_TextureSlot);

  s_TextureArray = Engine::TextureArray::Create(16, 128);
  for (BlockTexture texture : BlockTextureIterator())
    s_TextureArray->addTexture(Block::GetTexturePath(texture));
}

void ChunkRenderer::BeginScene(const Engine::Camera& camera)
{
  s_BlockFaceShader->bind();
  s_BlockFaceShader->setFloat("u_BlockLength", static_cast<float>(Block::Length()));
  s_BlockFaceShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());
  s_TextureArray->bind(s_TextureSlot);
}

void ChunkRenderer::EndScene()
{
}

void ChunkRenderer::DrawChunk(const Chunk* chunk)
{
  uint32_t meshIndexCount = 6 * static_cast<uint32_t>(chunk->getMesh().size()) / 4;

  if (meshIndexCount == 0)
    return; // Nothing to draw

  chunk->bindBuffers();
  s_BlockFaceShader->setFloat3("u_ChunkPosition", chunk->anchorPoint());
  Engine::RenderCommand::DrawIndexed(chunk->getVertexArray(), meshIndexCount);
}