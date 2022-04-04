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
    Entity(entt::entity entityID)
      : m_EntityID(entityID) {}

    template<typename T>
    bool has() const
    {
      EN_CORE_ASSERT(isValid(), "Entity handle is invalid!");
      return s_Registry.any_of<T>(m_EntityID);
    }
    
    template<typename T, typename... Args>
    T& add(Args&&... args)
    {
      EN_CORE_ASSERT(!has<T>(), "Entity already has component!");
      return s_Registry.emplace<T>(m_EntityID, std::forward<Args>(args)...);
    }
    
    template<typename T>
    T& get()
    {
      EN_CORE_ASSERT(has<T>(), "Entity does not have component!");
      return s_Registry.get<T>(m_EntityID);
    }

    template<typename T>
    const T& get() const
    {
      EN_CORE_ASSERT(has<T>(), "Entity does not have component!");
      return s_Registry.get<T>(m_EntityID);
    }
    
    template<typename T>
    void remove()
    {
      EN_CORE_ASSERT(has<T>(), "Entity does not have component!");
      s_Registry.remove<T>(m_EntityID);
    }

    uint32_t id() const { return static_cast<uint32_t>(m_EntityID); }

    bool isValid() const { return m_EntityID != entt::null; }

    operator entt::entity() const { return m_EntityID; }

    bool operator==(const Entity& other) const { return m_EntityID == other.m_EntityID; }
    bool operator!=(const Entity& other) const { return m_EntityID != other.m_EntityID; }

  private:
    entt::entity m_EntityID = entt::null;

    static entt::registry s_Registry;

    static entt::registry& Registry() { return s_Registry; }

    friend struct ECS;
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