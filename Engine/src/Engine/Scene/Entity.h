#pragma once
#include "Engine/Core/FixedWidthTypes.h"
#include "Engine/Debug/Assert.h"

#include <entt/entt.hpp>

namespace eng
{
  class Entity
  {
    static inline entt::registry s_Registry{};
    entt::entity m_EntityID = entt::null;

  public:
    Entity() = default;

    template<typename T>
    bool has() const
    {
      ENG_CORE_ASSERT(isValid(), "Entity handle is invalid!");
      return s_Registry.any_of<T>(m_EntityID);
    }
    
    template<typename T, typename... Args>
    T& add(Args&&... args)
    {
      ENG_CORE_ASSERT(!has<T>(), "Entity already has component!");
      return s_Registry.emplace<T>(m_EntityID, std::forward<Args>(args)...);
    }
    
    template<typename T>
    T& get()
    {
      ENG_CORE_ASSERT(has<T>(), "Entity does not have component!");
      return s_Registry.get<T>(m_EntityID);
    }

    template<typename T>
    const T& get() const
    {
      ENG_CORE_ASSERT(has<T>(), "Entity does not have component!");
      return s_Registry.get<T>(m_EntityID);
    }
    
    template<typename T>
    void remove()
    {
      ENG_CORE_ASSERT(has<T>(), "Entity does not have component!");
      s_Registry.remove<T>(m_EntityID);
    }

    u32 id() const { return static_cast<u32>(m_EntityID); }

    bool isValid() const { return m_EntityID != entt::null; }

    operator entt::entity() const { return m_EntityID; }

    bool operator==(const Entity& other) const = default;

  private:
    static entt::registry& Registry() { return s_Registry; }

    Entity(entt::entity entityID)
      : m_EntityID(entityID) {}

    friend class ECS;
  };
}