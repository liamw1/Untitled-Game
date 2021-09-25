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

    void BeginScene(Camera& camera);
    void EndScene();

    void DrawCube(const glm::vec3& position, const glm::vec3& size, Shared<Texture2D> texture = nullptr);

    void OnWindowResize(uint32_t width, uint32_t height);
  };
}