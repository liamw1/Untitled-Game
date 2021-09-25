#include "ChunkRenderer.h"
#include "Chunk.h"
#include "Block/Block.h"
#include <glm/gtc/matrix_transform.hpp>

struct BlockVertex
{
  glm::vec3 position;
  glm::vec2 texCoord;
};

/*
  Renderer 2D data
*/
static constexpr glm::vec4 s_BlockFacePositions[6][4] 
  = {   // Top Face
      { { -s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2,  1.0f, },
        {  s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2,  1.0f, },
        {  s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2,  1.0f, },
        { -s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2,  1.0f  } },
                                
        // Bottom Face
      { { -s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2,  1.0f, },
        {  s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2,  1.0f, },
        {  s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2,  1.0f, },
        { -s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2,  1.0f  } },
                                
        // North Face
      { { -s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2,  1.0f, },
        {  s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2,  1.0f, },
        {  s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2,  1.0f, },
        { -s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2,  1.0f  } },
                                
        // South Face
      { {  s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2,  1.0f, },
        { -s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2,  1.0f, },
        { -s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2,  1.0f, },
        {  s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2,  1.0f  } },
                                
        // East Face
      { { -s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2,  1.0f, },
        { -s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2,  1.0f, },
        { -s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2,  1.0f, },
        { -s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2,  1.0f  } },
                                
        // West Face
      { {  s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2,  1.0f, },
        {  s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2,  1.0f, },
        {  s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2,  1.0f, },
        {  s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2,  1.0f  } } };

// Maximum values per chunk mesh
// NOTE: Can maybe make this from template argument to save memory
static constexpr uint32_t s_MaxQuads = 6 * Chunk::Volume();
static constexpr uint32_t s_MaxVertices = 4 * s_MaxQuads;
static constexpr uint32_t s_MaxIndices = 6 * s_MaxQuads;

static Engine::Shared<Engine::VertexArray> s_MeshVertexArray;
static Engine::Shared<Engine::VertexBuffer> s_MeshVertexBuffer;
static Engine::Shared<Engine::Shader> s_TextureShader;
static Engine::Shared<Engine::Texture2D> s_TextureAtlas;
constexpr static uint8_t s_TextureSlot = 1;

static uint32_t s_MeshIndexCount = 0;
static BlockVertex* s_MeshVertexBufferBase = nullptr;
static BlockVertex* s_MeshVertexBufferPtr = nullptr;



void ChunkRenderer::Initialize()
{
  EN_PROFILE_FUNCTION();

  s_MeshVertexArray = Engine::VertexArray::Create();
  s_MeshVertexBuffer = Engine::VertexBuffer::Create((uint64_t)s_MaxVertices * sizeof(BlockVertex));

  s_MeshVertexBuffer->setLayout({ { ShaderDataType::Float3, "a_Position" },
                                  { ShaderDataType::Float2, "a_TexCoord" } });
  s_MeshVertexArray->addVertexBuffer(s_MeshVertexBuffer);

  s_MeshVertexBufferBase = new BlockVertex[s_MaxVertices];
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

  s_TextureShader = Engine::Shader::Create("assets/shaders/BlockFaceTexture.glsl");
  s_TextureShader->bind();
  s_TextureShader->setInt("u_Texture", s_TextureSlot);

  s_TextureAtlas = Engine::Texture2D::Create("assets/textures/C-tetra_1.7/blocks/sand.png");
}

void ChunkRenderer::Shutdown()
{
  delete[] s_MeshVertexBufferBase;
}

void ChunkRenderer::BeginScene(Engine::Camera& camera)
{
  s_TextureShader->bind();
  s_TextureShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

  s_MeshIndexCount = 0;
  s_MeshVertexBufferPtr = s_MeshVertexBufferBase;
}

void ChunkRenderer::EndScene()
{
  // Cast to 1-byte value to determine number of bytes in vertexBufferPtr
  uintptr_t dataSize = (uint8_t*)s_MeshVertexBufferPtr - (uint8_t*)s_MeshVertexBufferBase;
  s_MeshVertexBuffer->setData(s_MeshVertexBufferBase, dataSize);

  // Bind textures
  s_TextureAtlas->bind(s_TextureSlot);

  Engine::RenderCommand::DrawIndexed(s_MeshVertexArray, s_MeshIndexCount);
}

void ChunkRenderer::DrawBlockFace(const BlockFaceParams& params, const glm::vec3& chunkPosition)
{
  constexpr glm::vec2 textureCoordinates[4] = { {0.0f, 0.0f},
                                                {1.0f, 0.0f},
                                                {1.0f, 1.0f},
                                                {0.0f, 1.0f} };

  EN_ASSERT(s_MeshIndexCount < s_MaxIndices, "Index count has exceeded limit!");

  glm::mat4 transform = glm::translate(glm::mat4(1.0f), chunkPosition + params.relativePosition);

  for (int i = 0; i < 4; ++i)
  {
    s_MeshVertexBufferPtr->position = transform * s_BlockFacePositions[(uint8_t)params.normal][i];
    s_MeshVertexBufferPtr->texCoord = textureCoordinates[i];
    s_MeshVertexBufferPtr++;
  }

  s_MeshIndexCount += 6;
}
