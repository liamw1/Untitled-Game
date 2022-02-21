#include "GMpch.h"
#include "ChunkRenderer.h"
#include "Chunk.h"
#include "Block/Block.h"
#include "Player/Player.h"

/*
  Renderer 2D data
*/
static Engine::Shared<Engine::Shader> s_BlockFaceShader;
static Engine::Shared<Engine::Shader> s_LODShader;
static Engine::Shared<Engine::TextureArray> s_TextureArray;
static constexpr int s_TextureSlot = 0;



void ChunkRenderer::Initialize()
{
  EN_PROFILE_FUNCTION();

  s_BlockFaceShader = Engine::Shader::Create("assets/shaders/BlockFace.glsl");
  s_BlockFaceShader->bind();
  s_BlockFaceShader->setInt("u_TextureArray", s_TextureSlot);

  s_TextureArray = Engine::TextureArray::Create(16, 128);
  for (BlockTexture texture : BlockTextureIterator())
    s_TextureArray->addTexture(Block::GetTexturePath(texture));

  s_LODShader = Engine::Shader::Create("assets/shaders/ChunkLOD.glsl");
  s_LODShader->bind();
  s_LODShader->setInt("u_TextureArray", s_TextureSlot);
}

void ChunkRenderer::BeginScene(const Engine::Camera& camera)
{
  s_BlockFaceShader->bind();
  s_BlockFaceShader->setFloat("u_BlockLength", static_cast<float>(Block::Length()));
  s_BlockFaceShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());
  s_TextureArray->bind(s_TextureSlot);

  const radians playerFOV = Player::CameraController().getFOV();
  const float playerAspectRatio = Player::CameraController().getAspectRatio();
  Engine::Camera farCamera = Engine::Camera(camera, playerFOV, playerAspectRatio, static_cast<length_t>(0.01 * Block::Length()), static_cast<length_t>(100000.0 * Block::Length()));

  s_LODShader->bind();
  s_LODShader->setMat4("u_ViewProjection", farCamera.getViewProjectionMatrix());
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
  s_BlockFaceShader->bind();
  s_BlockFaceShader->setFloat3("u_ChunkPosition", chunk->anchorPosition());
  Engine::RenderCommand::DrawIndexed(chunk->getVertexArray(), meshIndexCount);
}

void ChunkRenderer::DrawLOD(const LOD::Octree::Node* node)
{
  if (node->data->meshData.size() == 0)
    return; // Nothing to draw

  Vec3 localAnchorPosition = Chunk::Length() * static_cast<Vec3>(node->anchor - Player::OriginIndex());

  node->data->vertexArray->bind();
  s_LODShader->bind();
  s_LODShader->setFloat("u_TextureScaling", static_cast<float>(bit(node->LODLevel())));
  s_LODShader->setFloat3("u_LODPosition", localAnchorPosition);
  Engine::RenderCommand::DrawVertices(node->data->vertexArray, static_cast<uint32_t>(node->data->meshData.size()));

  for (BlockFace face : BlockFaceIterator())
  {
    int faceID = static_cast<int>(face);

    if (node->data->transitionMeshData[faceID].size() == 0 || !node->data->isTransitionFace[faceID])
      continue;

    node->data->transitionVertexArrays[faceID]->bind();
    Engine::RenderCommand::DrawVertices(node->data->transitionVertexArrays[faceID], static_cast<uint32_t>(node->data->transitionMeshData[faceID].size()));
  }
}
