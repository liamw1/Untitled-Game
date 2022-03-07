#pragma once
#include "Engine/Core/Log.h"

#pragma warning( push )
#pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )
#include <entt/entt.hpp>
#pragma warning( pop )

namespace Engine
{
  class Entity
  {
  public:
    Entity() = default;

    Entity(entt::registry& registry, entt::entity entity)
      : m_Handle(registry, entity) {}

    template<typename T>
    bool has()
    {
      EN_CORE_ASSERT(isValid(), "Entity handle is invalid!");
      return m_Handle.any_of<T>();
    }
    
    template<typename T, typename... Args>
    T& add(Args&&... args)
    {
      EN_CORE_ASSERT(!has<T>(), "Entity already has component!");
      return m_Handle.emplace<T>(std::forward<Args>(args)...);
    }
    
    template<typename T>
    T& get()
    {
      EN_CORE_ASSERT(has<T>(), "Entity does not have component!");
      return m_Handle.get<T>();
    }
    
    template<typename T>
    void remove()
    {
      EN_CORE_ASSERT(has<T>(), "Entity does not have component!");
      m_Handle.remove<T>();
    }

  private:
    entt::handle m_Handle;

    bool isValid() { return static_cast<bool>(m_Handle); }
  };



  class ScriptableEntity
  {
  public:
    Entity entity;

    virtual ~ScriptableEntity() {}

    template<typename T>
    T& get()
    {
      return entity.get<T>();
    }

    virtual void onCreate() {};
    virtual void onDestroy() {};
    virtual void onUpdate(Timestep timestep) {};
  };
}