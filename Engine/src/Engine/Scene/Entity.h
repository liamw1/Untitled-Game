#pragma once
#include "Engine/Core/Log.h"
#include "Engine/Events/Event.h"

#pragma warning(push)
#pragma warning (disable : ALL_CODE_ANALYSIS_WARNINGS)
#include <entt/entt.hpp>
#pragma warning(pop)

namespace Engine
{
  class Entity
  {
  public:
    Entity() = default;

    Entity(entt::registry& registry, entt::entity entityID)
      : m_Handle(registry, entityID) {}

    template<typename T>
    bool has() const
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
    const T& get() const
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

    uint32_t id() const { return static_cast<uint32_t>(m_Handle.entity()); }

    bool isValid() const { return static_cast<bool>(m_Handle); }

    operator entt::entity() const { return m_Handle.entity(); }

    bool operator==(const Entity& other) const { return m_Handle == other.m_Handle; }
    bool operator!=(const Entity& other) const { return m_Handle != other.m_Handle; }

  private:
    entt::handle m_Handle;
  };



  class ScriptableEntity
  {
  public:
    Entity entity;

    virtual ~ScriptableEntity() {}

    template<typename T>
    bool has() const { return entity.has<T>(); }

    template<typename T>
    T& get() { return entity.get<T>(); }

    template<typename T>
    const T& get() const { return entity.get<T>(); }

    template<typename T, typename... Args>
    T& add(Args&&... args) { return entity.add<T>(std::forward<Args>(args)...); }

    template<typename T>
    void remove() { entity.remove<T>(); }

    virtual void onCreate() {};
    virtual void onDestroy() {};

    virtual void onUpdate(Timestep timestep) {};
    virtual void onEvent(Event& event) {};
  };
}