#pragma once
#include "VertexArray.h"
#include "Shader.h"
#include "Texture.h"
#include "UniformBuffer.h"

/*
  A general-purpose renderer for 3D objects/effects.
*/
namespace Engine
{
  namespace Renderer
  {
    void Initialize();
    void Shutdown();

    void BeginScene(const Mat4& viewProjection);
    void EndScene();

    void DrawCube(const Vec3& position, const Vec3& size, const Texture2D* texture = nullptr);
    void DrawCubeFrame(const Vec3& position, const Vec3& size, const Float4& color = Float4(0.0f, 0.0f, 0.0f, 1.0f));

    void OnWindowResize(uint32_t width, uint32_t height);

    template<typename Vertex>
    void UploadMesh(VertexArray* target, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
      EN_ASSERT(target, "Vertex array has not been initialized!");

      uintptr_t dataSize = vertices.size() * sizeof(Vertex);
      target->setVertexBuffer(vertices.data(), dataSize);
      target->setIndexBuffer(IndexBuffer::Create(indices));
    }
  };
}