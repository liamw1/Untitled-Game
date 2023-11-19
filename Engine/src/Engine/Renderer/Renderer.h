#pragma once
#include "VertexArray.h"
#include "Texture.h"
#include "Engine/Scene/Entity.h"

/*
  A general-purpose renderer for 3D objects/effects.
*/
namespace eng::render
{
  void beginScene(Entity viewer);
  void endScene();

  void drawCubeFrame(const math::Vec3& position, const math::Vec3& size, const math::Float4& color = math::Float4(0.0f, 0.0f, 0.0f, 1.0f));

  void onWindowResize(u32 width, u32 height);

  template<typename Vertex>
  void uploadMesh(VertexArray* target, const std::vector<Vertex>& vertices, const std::vector<u32>& indices)
  {
    ENG_ASSERT(target, "Vertex array has not been initialized!");

    uintptr_t dataSize = vertices.size() * sizeof(Vertex);
    target->setVertexBuffer(vertices);
    target->setIndexBuffer(IndexBuffer(indices));
  }
}