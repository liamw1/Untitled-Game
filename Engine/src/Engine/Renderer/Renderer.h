#pragma once
#include "VertexArray.h"
#include "Texture.h"
#include "Engine/Scene/Entity.h"

/*
  A general-purpose renderer for 3D objects/effects.
*/
namespace eng::renderer
{
  void initialize();
  void shutdown();

  void beginScene(Entity viewer);
  void endScene();

  void drawCube(const math::Vec3& position, const math::Vec3& size, const Texture* texture = nullptr);
  void drawCubeFrame(const math::Vec3& position, const math::Vec3& size, const math::Float4& color = math::Float4(0.0f, 0.0f, 0.0f, 1.0f));

  void onWindowResize(uint32_t width, uint32_t height);

  template<typename Vertex>
  void uploadMesh(VertexArray* target, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
  {
    EN_ASSERT(target, "Vertex array has not been initialized!");

    uintptr_t dataSize = vertices.size() * sizeof(Vertex);
    target->setVertexBuffer(vertices.data(), dataSize);
    target->setIndexBuffer(IndexBuffer(indices));
  }
}