#include "GMpch.h"
#include "ChunkRenderer.h"
#include "Chunk.h"
#include "Block/Block.h"
#include "Player/Player.h"

/*
  Renderer data
*/
static Shared<Engine::Shader> s_BlockFaceShader;
static Shared<Engine::Shader> s_LODShader;
static Shared<Engine::TextureArray> s_TextureArray;
static constexpr int s_TextureSlot = 0;

static constexpr float s_LODNearPlaneDistance = static_cast<float>(10 * Block::Length());
static constexpr float s_LODFarPlaneDistance = static_cast<float>(1e6 * Block::Length());


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
  s_LODShader->setFloat("u_NearPlaneDistance", s_LODNearPlaneDistance);
  s_LODShader->setFloat("u_FarPlaneDistance", s_LODFarPlaneDistance);
}

void ChunkRenderer::BeginScene(const Mat4& viewProjection)
{
  s_BlockFaceShader->bind();
  s_BlockFaceShader->setFloat("u_BlockLength", static_cast<float>(Block::Length()));
  s_BlockFaceShader->setMat4("u_ViewProjection", viewProjection);
  s_TextureArray->bind(s_TextureSlot);

  s_LODShader->bind();
  s_LODShader->setMat4("u_ViewProjection", viewProjection);
}

void ChunkRenderer::EndScene()
{
}

void ChunkRenderer::DrawChunk(const Chunk* chunk)
{
  uint32_t meshIndexCount = 6 * static_cast<uint32_t>(chunk->getQuadCount());

  if (meshIndexCount == 0)
    return; // Nothing to draw

  chunk->bindBuffers();
  s_BlockFaceShader->bind();
  s_BlockFaceShader->setFloat3("u_ChunkPosition", chunk->anchorPosition());
  Engine::RenderCommand::DrawIndexed(chunk->getVertexArray(), meshIndexCount);
}

// NOTE: Should use glMultiDrawArrays to draw primary mesh and transition meshes in single draw call
void ChunkRenderer::DrawLOD(const LOD::Octree::Node* node)
{
  uint32_t primaryMeshIndexCount = static_cast<uint32_t>(node->data->primaryMesh.indices.size());

  if (primaryMeshIndexCount == 0)
    return; // Nothing to draw

  Vec3 localAnchorPosition = Chunk::Length() * static_cast<Vec3>(node->anchor - Player::OriginIndex());

  node->data->primaryMesh.vertexArray->bind();
  s_LODShader->bind();
  s_LODShader->setFloat("u_TextureScaling", static_cast<float>(bit(node->LODLevel())));
  s_LODShader->setFloat3("u_LODPosition", localAnchorPosition);
  Engine::RenderCommand::DrawIndexed(node->data->primaryMesh.vertexArray, primaryMeshIndexCount);

  for (BlockFace face : BlockFaceIterator())
  {
    int faceID = static_cast<int>(face);

    uint32_t transitionMeshIndexCount = static_cast<uint32_t>(node->data->transitionMeshes[faceID].indices.size());

    if (transitionMeshIndexCount == 0 || !(node->data->transitionFaces & bit(faceID)))
      continue;

    node->data->transitionMeshes[faceID].vertexArray->bind();
    Engine::RenderCommand::DrawIndexed(node->data->transitionMeshes[faceID].vertexArray, transitionMeshIndexCount);
  }
}
