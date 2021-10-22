#include "GMpch.h"
#include "ChunkRenderer.h"
#include "Chunk.h"
#include "Block/Block.h"

/*
  Renderer 2D data
*/
static Engine::Shared<Engine::Shader> s_BlockFaceShader;
static Engine::Shared<Engine::TextureArray> s_TextureArray;
constexpr static uint8_t s_TextureSlot = 0;



void ChunkRenderer::Initialize()
{
  EN_PROFILE_FUNCTION();

  s_BlockFaceShader = Engine::Shader::Create("assets/shaders/BlockFace.glsl");
  s_BlockFaceShader->bind();
  s_BlockFaceShader->setInt("u_TextureArray", s_TextureSlot);

  s_TextureArray = Engine::TextureArray::Create(8, 128);
  s_TextureArray->addTexture("assets/textures/voxel-pack/PNG/Tiles/grass_top.png");
  s_TextureArray->addTexture("assets/textures/voxel-pack/PNG/Tiles/dirt_grass.png");
  s_TextureArray->addTexture("assets/textures/voxel-pack/PNG/Tiles/dirt.png");
}

void ChunkRenderer::BeginScene(const Engine::Camera& camera)
{
  s_BlockFaceShader->setFloat("u_BlockLength", Block::Length());
  s_BlockFaceShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());
  s_TextureArray->bind(s_TextureSlot);
  s_BlockFaceShader->bind();
}

void ChunkRenderer::EndScene()
{
}

void ChunkRenderer::DrawChunk(const Chunk* chunk)
{
  uint32_t meshIndexCount = 6 * static_cast<uint32_t>(chunk->getMesh().size()) / 4;

  chunk->bindBuffers();
  s_BlockFaceShader->setFloat3("u_ChunkPosition", chunk->anchorPoint());
  Engine::RenderCommand::DrawIndexed(chunk->getVertexArray(), meshIndexCount);
}