#pragma once
#include "Engine/Math/ArrayBox.h"

namespace eng::thread
{
  /*
    Thread-safe version of the ArrayBox.
  */
  template<typename T, std::integral IntType>
  class ProtectedArrayBox : private NonCopyable, NonMovable
  {
    mutable std::shared_mutex m_Mutex;
    math::ArrayBox<T, IntType> m_ArrayBox;
    T m_DefaultValue;

  public:
    ProtectedArrayBox(const math::IBox3<IntType>& bounds, const T& defaultValue)
      : m_ArrayBox(bounds, AllocationPolicy::Deferred), m_DefaultValue(defaultValue) {}
    ~ProtectedArrayBox() = default;

    operator bool() const
    {
      std::shared_lock lock(m_Mutex);
      return m_ArrayBox;
    }

    T get(const math::IVec3<IntType>& index) const
    {
      std::shared_lock lock(m_Mutex);

      if (!m_ArrayBox)
        return m_DefaultValue;

      return m_ArrayBox(index);
    }

    template<std::invocable<const math::ArrayBox<T, IntType>&> F>
    std::invoke_result_t<F, const math::ArrayBox<T, IntType>&> readOperation(const F& operation) const
    {
      std::shared_lock lock(m_Mutex);
      return operation(m_ArrayBox);
    }

    template<std::invocable<const math::ArrayBox<T, IntType>&, const T&> F>
    std::invoke_result_t<F, const math::ArrayBox<T, IntType>&, const T&> readOperation(const F& operation) const
    {
      std::shared_lock lock(m_Mutex);
      return operation(m_ArrayBox, m_DefaultValue);
    }

    void set(const math::IVec3<IntType>& index, const T& value)
    {
      std::lock_guard lock(m_Mutex);
      setIfNecessary(index, value);
    }

    template<InvocableWithReturnType<bool, T> F>
    bool setIf(const math::IVec3<IntType>& index, const T& value, const F& condition)
    {
      std::lock_guard lock(m_Mutex);

      const T& containedValue = m_ArrayBox ? m_ArrayBox(index) : m_DefaultValue;
      if (!condition(containedValue))
        return false;

      setIfNecessary(index, value);
      return true;
    }

    [[nodiscard]] T replace(const math::IVec3<IntType>& index, const T& value)
    {
      std::lock_guard lock(m_Mutex);

      T containedValue = m_ArrayBox ? m_ArrayBox(index) : m_DefaultValue;
      setIfNecessary(index, value);

      return containedValue;
    }

    void setData(math::ArrayBox<T, IntType>&& newArrayBox)
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

    template<std::invocable<math::ArrayBox<T, IntType>&> F>
    std::invoke_result_t<F, math::ArrayBox<T, IntType>&> modifyingOperation(const F& operation)
    {
      std::lock_guard lock(m_Mutex);
      return operation(m_ArrayBox);
    }

    template<std::invocable<math::ArrayBox<T, IntType>&, const T&> F>
    std::invoke_result_t<F, math::ArrayBox<T, IntType>&, const T&> modifyingOperation(const F& operation)
    {
      std::lock_guard lock(m_Mutex);
      return operation(m_ArrayBox, m_DefaultValue);
    }

  private:
    void allocateAndFillWithDefaultValue()
    {
      m_ArrayBox.allocate();
      m_ArrayBox.fill(m_DefaultValue);
    }

    void setIfNecessary(const math::IVec3<IntType>& index, const T& value)
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