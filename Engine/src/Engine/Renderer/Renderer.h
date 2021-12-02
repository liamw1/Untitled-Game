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
  };
}