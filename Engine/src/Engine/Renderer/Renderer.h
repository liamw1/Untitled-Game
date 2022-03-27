#pragma once
#include "RendererAPI.h"
#include "Shader.h"
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

    void BeginScene(const Mat4& viewProjection);
    void EndScene();

    void DrawCube(const Vec3& position, const Vec3& size, Shared<Texture2D> texture = nullptr);
    void DrawCubeFrame(const Vec3& position, const Vec3& size, const Float4& color = Float4(0.0f, 0.0f, 0.0f, 1.0f));

    void OnWindowResize(uint32_t width, uint32_t height);

    void UploadMesh(Unique<VertexArray>& target, const BufferLayout& bufferLayout, const void* data, uintptr_t dataSize, const uint32_t* meshIndices, uint32_t indexCount);

    template<typename Vertex>
    void UploadMesh(Unique<VertexArray>& target, const BufferLayout& bufferLayout, const std::vector<Vertex>& meshVertices, const std::vector<uint32_t>& meshIndices)
    {
      uintptr_t dataSize = sizeof(Vertex) * meshVertices.size();
      UploadMesh(target, bufferLayout, meshVertices.data(), dataSize, meshIndices.data(), static_cast<uint32_t>(meshIndices.size()));
    }

    template<typename Vertex>
    void UploadMesh(Unique<VertexArray>& target, const BufferLayout& bufferLayout, const std::vector<Vertex>& meshVertices, const uint32_t* meshIndices, uint32_t indexCount)
    {
      uintptr_t dataSize = sizeof(Vertex) * meshVertices.size();
      UploadMesh(target, bufferLayout, meshVertices.data(), dataSize, meshIndices, indexCount);
    }
  };
}