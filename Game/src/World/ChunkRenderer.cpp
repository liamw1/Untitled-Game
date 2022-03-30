#include "GMpch.h"
#include "ChunkRenderer.h"
#include "Chunk.h"
#include "Block/Block.h"
#include "Player/Player.h"

static constexpr int s_TextureSlot = 0;
static constexpr float s_LODNearPlaneDistance = static_cast<float>(10 * Block::Length());
static constexpr float s_LODFarPlaneDistance = static_cast<float>(1e10 * Block::Length());

struct CameraUniforms
{
  Float4x4 viewProjection;
};
struct ChunkUniforms
{
  Float3 anchor;
  const float blockLength = Block::Length();
  const int textureSlot = s_TextureSlot;
};
struct LODUniforms
{
  Float3 anchor;
  float textureScaling;
  float nearPlaneDistance = s_LODNearPlaneDistance;
  float farPlaneDistance = s_LODFarPlaneDistance;
};

/*
  Renderer data
*/
static Unique<Engine::Shader> s_BlockFaceShader;
static Unique<Engine::Shader> s_LODShader;
static Unique<Engine::UniformBuffer> s_CameraUniformBuffer;
static Unique<Engine::UniformBuffer> s_ChunkUniformBuffer;
static Unique<Engine::UniformBuffer> s_LODUniformBuffer;
static CameraUniforms s_CameraUniforms{};
static ChunkUniforms s_ChunkUniforms{};
static LODUniforms s_LODUniforms{};

static Unique<Engine::TextureArray> s_TextureArray;


void ChunkRenderer::Initialize()
{
  EN_PROFILE_FUNCTION();

  s_BlockFaceShader = Engine::Shader::Create("assets/shaders/Chunk.glsl");
  s_LODShader = Engine::Shader::Create("assets/shaders/ChunkLOD.glsl");

  s_TextureArray = Engine::TextureArray::Create(16, 128);
  for (BlockTexture texture : BlockTextureIterator())
    s_TextureArray->addTexture(Block::GetTexturePath(texture));

  s_CameraUniformBuffer = Engine::UniformBuffer::Create(sizeof(CameraUniforms), 1);
  s_ChunkUniformBuffer = Engine::UniformBuffer::Create(sizeof(ChunkUniforms), 2);
  s_LODUniformBuffer = Engine::UniformBuffer::Create(sizeof(LODUniforms), 3);
}

void ChunkRenderer::BeginScene(const Mat4& viewProjection)
{
  s_CameraUniforms.viewProjection = viewProjection;
  s_CameraUniformBuffer->setData(&s_CameraUniforms, sizeof(CameraUniforms));
  s_TextureArray->bind(s_TextureSlot);
}

void ChunkRenderer::EndScene()
{
}

void ChunkRenderer::DrawChunk(const Chunk* chunk)
{
  uint32_t meshIndexCount = 6 * static_cast<uint32_t>(chunk->getQuadCount());

  if (meshIndexCount == 0)
    return; // Nothing to draw

  s_ChunkUniforms.anchor = chunk->anchorPosition();
  s_ChunkUniformBuffer->setData(&s_ChunkUniforms, sizeof(ChunkUniforms));

  s_BlockFaceShader->bind();
  chunk->bindBuffers();
  Engine::RenderCommand::DrawIndexed(chunk->getVertexArray().get(), meshIndexCount);
}

// NOTE: Should use glMultiDrawArrays to draw primary mesh and transition meshes in single draw call
void ChunkRenderer::DrawLOD(const LOD::Octree::Node* node)
{
  uint32_t primaryMeshIndexCount = static_cast<uint32_t>(node->data->primaryMesh.indices.size());

  if (primaryMeshIndexCount == 0)
    return; // Nothing to draw

  // Set local anchor position and texture scaling
  s_LODUniforms.anchor = Chunk::Length() * static_cast<Vec3>(node->anchor - Player::OriginIndex());
  s_LODUniforms.textureScaling = static_cast<float>(bit(node->LODLevel()));
  s_LODUniformBuffer->setData(&s_LODUniforms, sizeof(LODUniforms));

  s_LODShader->bind();
  node->data->primaryMesh.vertexArray->bind();
  Engine::RenderCommand::DrawIndexed(node->data->primaryMesh.vertexArray.get(), primaryMeshIndexCount);

  for (BlockFace face : BlockFaceIterator())
  {
    int faceID = static_cast<int>(face);

    uint32_t transitionMeshIndexCount = static_cast<uint32_t>(node->data->transitionMeshes[faceID].indices.size());

    if (transitionMeshIndexCount == 0 || !(node->data->transitionFaces & bit(faceID)))
      continue;

    node->data->transitionMeshes[faceID].vertexArray->bind();
    Engine::RenderCommand::DrawIndexed(node->data->transitionMeshes[faceID].vertexArray.get(), transitionMeshIndexCount);
  }
}
