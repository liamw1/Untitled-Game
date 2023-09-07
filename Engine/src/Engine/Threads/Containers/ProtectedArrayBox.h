#pragma once
#include "Engine/Math/ArrayBox.h"

namespace Engine::Threads
{
  /*
    Thread-safe version of the ArrayBox.
  */
  template<typename T, std::integral IntType, IntType MinX, IntType MaxX, IntType MinY = MinX, IntType MaxY = MaxX, IntType MinZ = MinX, IntType MaxZ = MaxX>
  class ProtectedArrayBox : private NonCopyable, NonMovable
  {
  public:
    ProtectedArrayBox() = default;
    ProtectedArrayBox(AllocationPolicy policy)
      : m_ArrayBox(policy) {}
    ProtectedArrayBox(const T& initialValue)
      : m_ArrayBox(initialValue) {}
    ~ProtectedArrayBox() = default;

    operator bool() const
    {
      std::lock_guard lock(m_Mutex);
      return m_ArrayBox;
    }

    T operator()(const IVec3<IntType>& index) const
    {
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox)
        return m_DefaultValue;

      return m_ArrayBox(index);
    }

    bool contains(const T& value) const
    {
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox)
        return value == m_DefaultValue;

      return m_ArrayBox.contains(value);
    }

    bool filledWith(const T& value) const
    {
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox)
        return value == m_DefaultValue;

      return m_ArrayBox.filledWith(value);
    }

    template<IntType... Args>
    bool contentsEqual(const IBox3<IntType>& compareSection, const ArrayBox<T, IntType, Args...>& container, const IBox3<IntType>& containerSection) const
    {
      EN_CORE_ASSERT(compareSection.extents() == containerSection.extents(), "Compared sections are not the same dimensions!");
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox && !container)
        return true;
      if (!m_ArrayBox)
        return container.allOf(containerSection, [](const T& value) { return value == m_DefaultValue; });
      if (!container)
        return m_ArrayBox.allOf(compareSection, [](const T& value) { return value == m_DefaultValue; });

      return m_ArrayBox.contentsEqual(compareSection, container, containerSection);
    }

    template<InvocableWithReturnType<bool, T> F>
    bool allOf(const IBox3<IntType>& section, const F& condition) const
    {
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox)
        return condition(m_DefaultValue);

      return m_ArrayBox.allOf(section, condition);
    }

    template<InvocableWithReturnType<bool, T> F>
    bool anyOf(const IBox3<IntType>& section, const F& condition) const
    {
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox)
        return condition(m_DefaultValue);

      return m_ArrayBox.anyOf(section, condition);
    }

    template<InvocableWithReturnType<bool, T> F>
    bool noneOf(const IBox3<IntType>& section, const F& condition) const
    {
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox)
        return !condition(m_DefaultValue);

      return m_ArrayBox.noneOf(section, condition);
    }

    void set(const IVec3<IntType>& index, const T& value)
    {
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox)
        allocateAndFillWithDefaultValue();

      m_ArrayBox(index) = value;
    }

#if 0
    void fill(const T& value)
    {
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox)
        m_ArrayBox.allocate();

      m_ArrayBox.fill(value);
    }

    void fill(const IBox3<IntType>& fillSection, const T& value)
    {
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox)
        allocateAndFillWithDefaultValue();

      m_ArrayBox.fill(fillSection, value);
    }

    template<IntType... Args>
    void fill(const IBox3<IntType>& fillSection, const ArrayBox<T, IntType, Args...>& container, const IBox3<IntType>& containerSection)
    {
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox && !container)
        return;
      if (!container)
      {
        m_ArrayBox.fill(fillSection, m_DefaultValue);
        return;
      }
      if (!m_ArrayBox)
      {
        if (container.allOf(containerSection, [](const T& value) { return value == m_DefaultValue; }))
          return;

        allocateAndFillWithDefaultValue();
      }

      m_ArrayBox.fill(fillSection, container, containerSection);
    }
#endif

    void setData(ArrayBox<T, IntType, MinX, MaxX, MinY, MaxY, MinZ, MaxZ>&& newArrayBox)
    {
      std::lock_guard lock(m_Mutex);
      m_ArrayBox = std::move(newArrayBox);
    }

    void resetIfFilledWithDefault()
    {
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox)
        return;

      if (m_ArrayBox.filledWith(m_DefaultValue))
        m_ArrayBox.reset();
    }
  
  private:
    mutable std::shared_mutex m_Mutex;
    ArrayBox<T, IntType, MinX, MaxX, MinY, MaxY, MinZ, MaxZ> m_ArrayBox;
    T m_DefaultValue;

    void allocateAndFillWithDefaultValue()
    {
      m_ArrayBox.allocate();
      m_ArrayBox.fill(m_DefaultValue);
    }
  };
}