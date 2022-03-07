#pragma once
#include "Entity.h"
#include "Engine/Renderer/SceneCamera.h"

#pragma warning( push )
#pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )
#include <entt/entt.hpp>
#pragma warning( pop )

namespace Engine
{
  namespace Scene
  {
    Entity CreateEntity(const std::string& name = std::string());

    void OnUpdate(Timestep timestep);

    const SceneCamera& GetActiveCamera();

    void OnViewportResize(uint32_t width, uint32_t height);

    entt::registry& Registry();
  };
}