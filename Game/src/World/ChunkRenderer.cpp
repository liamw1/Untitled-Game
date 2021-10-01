#include "ChunkRenderer.h"
#include "Chunk.h"
#include "Block/Block.h"
#include <glm/gtc/matrix_transform.hpp>

/*
  Renderer 2D data
*/
// Maximum values per chunk mesh
// NOTE: Can maybe make this from template argument to save memory
static constexpr uint32_t s_MaxQuads = 3 * Chunk::TotalBlocks();
static constexpr uint32_t s_MaxVertices = 4 * s_MaxQuads;
static constexpr uint32_t s_MaxIndices = 6 * s_MaxQuads;

static Engine::Shared<Engine::VertexArray> s_MeshVertexArray;
static Engine::Shared<Engine::VertexBuffer> s_MeshVertexBuffer;
static Engine::Shared<Engine::Shader> s_BlockFaceShader;
static Engine::Shared<Engine::Texture2D> s_TextureAtlas;
constexpr static uint8_t s_TextureSlot = 1;



void ChunkRenderer::Initialize()
{
  EN_PROFILE_FUNCTION();

  s_MeshVertexArray = Engine::VertexArray::Create();

  s_MeshVertexBuffer = Engine::VertexBuffer::Create((uint64_t)s_MaxVertices * sizeof(BlockVertex));
  s_MeshVertexBuffer->setLayout({ { ShaderDataType::Float3, "a_Position" },
                                  { ShaderDataType::Float2, "a_TexCoord" } });
  s_MeshVertexArray->addVertexBuffer(s_MeshVertexBuffer);

  uint32_t* meshIndices = new uint32_t[s_MaxIndices];

  uint32_t offset = 0;
  for (uint32_t i = 0; i < s_MaxIndices; i += 6)
  {
    // Triangle 1
    meshIndices[i + 0] = offset + 0;
    meshIndices[i + 1] = offset + 1;
    meshIndices[i + 2] = offset + 2;

    // Triangle 2
    meshIndices[i + 3] = offset + 2;
    meshIndices[i + 4] = offset + 3;
    meshIndices[i + 5] = offset + 0;

    offset += 4;
  }

  Engine::Shared<Engine::IndexBuffer> meshIndexBuffer = Engine::IndexBuffer::Create(meshIndices, s_MaxIndices);
  s_MeshVertexArray->setIndexBuffer(meshIndexBuffer);
  delete[] meshIndices;

  s_BlockFaceShader = Engine::Shader::Create("assets/shaders/BlockFaceTexture.glsl");
  s_BlockFaceShader->bind();
  s_BlockFaceShader->setInt("u_TextureAtlas", s_TextureSlot);

  s_TextureAtlas = Engine::Texture2D::Create("assets/textures/C-tetra_1.7/blocks/sand.png");
}

void ChunkRenderer::BeginScene(Engine::Camera& camera)
{
  s_BlockFaceShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());
}

void ChunkRenderer::EndScene()
{
}

void ChunkRenderer::DrawChunk(const Chunk& chunk)
{
  EN_PROFILE_FUNCTION();

  const auto& mesh = chunk.getMesh();

  uint32_t meshIndexCount = 6 * (uint32_t)mesh.size();

  if (meshIndexCount == 0)
    return; // Nothing to draw

  // Cast to 1-byte value to determine number of bytes in vertexBufferPtr
  uintptr_t dataSize = sizeof(BlockVertex) * mesh.size();
  s_MeshVertexBuffer->setData(mesh.data(), dataSize);

  s_TextureAtlas->bind(s_TextureSlot);
  s_BlockFaceShader->bind();

  Engine::RenderCommand::DrawIndexed(s_MeshVertexArray, meshIndexCount);
}
