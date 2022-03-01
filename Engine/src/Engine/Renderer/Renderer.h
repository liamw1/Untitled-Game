#pragma once
#include "RendererAPI.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"

/*
  A general-purpose renderer for 3D objects/effects.
*/
namespace Engine
{
  namespace Renderer
  {
    void Initialize();
    void Shutdown();

    inline RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

    void BeginScene(const Camera& camera);
    void EndScene();

    void DrawCube(const Vec3& position, const Vec3& size, Shared<Texture2D> texture = nullptr);
    void DrawCubeFrame(const Vec3& position, const Vec3& size, const Float4& color = Float4(0.0f, 0.0f, 0.0f, 1.0f));

    void OnWindowResize(uint32_t width, uint32_t height);

    template<typename Vertex>
    void UploadMesh(Shared<VertexArray>& target, const BufferLayout& bufferLayout, const std::vector<Vertex>& meshVertices, const Shared<IndexBuffer>& meshIndexBuffer)
    {
      uint32_t vertexCount = static_cast<uint32_t>(meshVertices.size());

      // Generate vertex array
      auto vertexArray = VertexArray::Create();
      auto vertexBuffer = VertexBuffer::Create(vertexCount * sizeof(Vertex));
      vertexBuffer->setLayout(bufferLayout);
      vertexArray->addVertexBuffer(vertexBuffer);

      vertexArray->setIndexBuffer(meshIndexBuffer);

      uintptr_t dataSize = vertexCount * sizeof(Vertex);
      vertexBuffer->setData(meshVertices.data(), dataSize);
      target = std::move(vertexArray);
    }

    template<typename Vertex>
    void UploadMesh(Shared<VertexArray>& target, const BufferLayout& bufferLayout, const std::vector<Vertex>& meshVertices, const std::vector<uint32_t>& meshIndices)
    {
      uint32_t vertexCount = static_cast<uint32_t>(meshVertices.size());
      uint32_t indexCount = static_cast<uint32_t>(meshIndices.size());

      // Generate vertex array
      auto vertexArray = VertexArray::Create();
      auto vertexBuffer = VertexBuffer::Create(vertexCount * sizeof(Vertex));
      vertexBuffer->setLayout(bufferLayout);
      vertexArray->addVertexBuffer(vertexBuffer);

      Shared<IndexBuffer> indexBuffer = IndexBuffer::Create(meshIndices.data(), indexCount);
      vertexArray->setIndexBuffer(indexBuffer);

      uintptr_t dataSize = vertexCount * sizeof(Vertex);
      vertexBuffer->setData(meshVertices.data(), dataSize);
      target = std::move(vertexArray);
    }
  };
}