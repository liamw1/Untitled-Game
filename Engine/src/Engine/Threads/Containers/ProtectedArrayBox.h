#pragma once
#include "Engine/Math/ArrayBox.h"

namespace Engine::Threads
{
  /*
    Thread-safe version of the ArrayBox.
  */
  template<typename T, std::integral IntType>
  class ProtectedArrayBox : private NonCopyable, NonMovable
  {
  public:
    ProtectedArrayBox(const IBox3<IntType>& bounds, const T& defaultValue)
      : m_ArrayBox(bounds, AllocationPolicy::Deferred), m_DefaultValue(defaultValue) {}
    ~ProtectedArrayBox() = default;

    operator bool() const
    {
      std::shared_lock lock(m_Mutex);
      return m_ArrayBox;
    }

    T get(const IVec3<IntType>& index) const
    {
      std::shared_lock lock(m_Mutex);

      if (!m_ArrayBox)
        return m_DefaultValue;

      return m_ArrayBox(index);
    }

    template<std::invocable<const ArrayBox<T, IntType>&> F>
    std::invoke_result_t<F, const ArrayBox<T, IntType>&> readOperation(const F& operation) const
    {
      std::shared_lock lock(m_Mutex);
      return operation(m_ArrayBox);
    }

    template<std::invocable<const ArrayBox<T, IntType>&, const T&> F>
    std::invoke_result_t<F, const ArrayBox<T, IntType>&, const T&> readOperation(const F& operation) const
    {
      std::shared_lock lock(m_Mutex);
      return operation(m_ArrayBox, m_DefaultValue);
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

    [[nodiscard]] T replace(const IVec3<IntType>& index, const T& value)
    {
      std::lock_guard lock(m_Mutex);

      T containedValue = m_ArrayBox ? m_ArrayBox(index) : m_DefaultValue;
      setIfNecessary(index, value);

      return containedValue;
    }

    void setData(ArrayBox<T, IntType>&& newArrayBox)
    {
      std::lock_guard lock(m_Mutex);
      m_ArrayBox = std::move(newArrayBox);
    }

    void clearIfFilledWithDefault()
    {
      std::lock_guard lock(m_Mutex);

      if (!m_ArrayBox)
        return;

      if (m_ArrayBox.filledWith(m_DefaultValue))
        m_ArrayBox.clear();
    }

    template<std::invocable<ArrayBox<T, IntType>&> F>
    std::invoke_result_t<F, ArrayBox<T, IntType>&> modifyingOperation(const F& operation)
    {
      std::lock_guard lock(m_Mutex);
      return operation(m_ArrayBox);
    }

    template<std::invocable<ArrayBox<T, IntType>&, const T&> F>
    std::invoke_result_t<F, ArrayBox<T, IntType>&, const T&> modifyingOperation(const F& operation)
    {
      std::lock_guard lock(m_Mutex);
      return operation(m_ArrayBox, m_DefaultValue);
    }

  private:
    mutable std::shared_mutex m_Mutex;
    ArrayBox<T, IntType> m_ArrayBox;
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