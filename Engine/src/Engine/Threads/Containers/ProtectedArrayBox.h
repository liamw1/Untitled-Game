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
    using Underlying = ArrayBox<T, IntType, MinX, MaxX, MinY, MaxY, MinZ, MaxZ>;

  public:
    ProtectedArrayBox(const T& defaultValue)
      : m_DefaultValue(defaultValue) {}
    ~ProtectedArrayBox() = default;

    operator bool() const
    {
      std::shared_lock lock(m_Mutex);
      return m_ArrayBox;
    }

    T operator()(const IVec3<IntType>& index) const
    {
      std::shared_lock lock(m_Mutex);

      if (!m_ArrayBox)
        return m_DefaultValue;

      return m_ArrayBox(index);
    }

    bool contains(const T& value) const
    {
      std::shared_lock lock(m_Mutex);

      if (!m_ArrayBox)
        return value == m_DefaultValue;

      return m_ArrayBox.contains(value);
    }

    bool filledWith(const T& value) const
    {
      std::shared_lock lock(m_Mutex);

      if (!m_ArrayBox)
        return value == m_DefaultValue;

      return m_ArrayBox.filledWith(value);
    }

    template<IntType... Args>
    bool contentsEqual(const IBox3<IntType>& compareSection, const ArrayBox<T, IntType, Args...>& container, const IBox3<IntType>& containerSection) const
    {
      EN_CORE_ASSERT(compareSection.extents() == containerSection.extents(), "Compared sections are not the same dimensions!");
      std::shared_lock lock(m_Mutex);

      if (!m_ArrayBox && !container)
        return true;
      if (!m_ArrayBox)
        return container.allOf(containerSection, [this](const T& value) { return value == m_DefaultValue; });
      if (!container)
        return m_ArrayBox.allOf(compareSection, [this](const T& value) { return value == m_DefaultValue; });

      return m_ArrayBox.contentsEqual(compareSection, container, containerSection);
    }

    template<InvocableWithReturnType<bool, T> F>
    bool allOf(const IBox3<IntType>& section, const F& condition) const
    {
      std::shared_lock lock(m_Mutex);

      if (!m_ArrayBox)
        return condition(m_DefaultValue);

      return m_ArrayBox.allOf(section, condition);
    }

    template<InvocableWithReturnType<bool, T> F>
    bool anyOf(const IBox3<IntType>& section, const F& condition) const
    {
      std::shared_lock lock(m_Mutex);

      if (!m_ArrayBox)
        return condition(m_DefaultValue);

      return m_ArrayBox.anyOf(section, condition);
    }

    template<InvocableWithReturnType<bool, T> F>
    bool noneOf(const IBox3<IntType>& section, const F& condition) const
    {
      std::shared_lock lock(m_Mutex);

      if (!m_ArrayBox)
        return !condition(m_DefaultValue);

      return m_ArrayBox.noneOf(section, condition);
    }

    template<IntType... Args>
    void useToFill(const IBox3<IntType>& readSection, ArrayBox<T, IntType, Args...>& container, const IBox3<IntType>& fillSection) const
    {
      EN_CORE_ASSERT(container, "Container data has not yet been allocated!");
      std::shared_lock lock(m_Mutex);

      if (!m_ArrayBox)
        container.fill(fillSection, m_DefaultValue);
      else
        container.fill(fillSection, m_ArrayBox, readSection);
    }

    template<InvocableWithReturnType<void, const Underlying&> F>
    void readOperation(const F& operation) const
    {
      std::shared_lock lock(m_Mutex);
      operation(m_ArrayBox);
    }

    void set(const IVec3<IntType>& index, const T& value)
    {
      std::lock_guard lock(m_Mutex);
      setIfNecessary(index, value);
    }

    template<InvocableWithReturnType<bool, T> F>
    bool setIf(const IVec3<IntType>& index, const T& value, const F& condition)
    {
      std::lock_guard lock(m_Mutex);

      const T& containedValue = m_ArrayBox ? m_ArrayBox(index) : m_DefaultValue;
      if (!condition(containedValue))
        return false;

      setIfNecessary(index, value);
      return true;
    }

    [[nodiscard]] T getAndSet(const IVec3<IntType>& index, const T& value)
    {
      std::lock_guard lock(m_Mutex);

      T containedValue = m_ArrayBox ? m_ArrayBox(index) : m_DefaultValue;
      setIfNecessary(index, value);

      return containedValue;
    }

    // Methods disabled until proven useful. May remove otherwise as they may be difficult to debug and maintain.
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

    void setData(Underlying&& newArrayBox)
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
    Underlying m_ArrayBox;
    T m_DefaultValue;

    void allocateAndFillWithDefaultValue()
    {
      m_ArrayBox.allocate();
      m_ArrayBox.fill(m_DefaultValue);
    }

    void setIfNecessary(const IVec3<IntType>& index, const T& value)
    {
      if (!m_ArrayBox)
      {
        if (value == m_DefaultValue)
          return;

        allocateAndFillWithDefaultValue();
      }

      m_ArrayBox(index) = value;
    }
  };
}