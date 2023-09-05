#pragma once
#include "Engine/Math/ArrayBox.h"

namespace Engine::Threads
{
  template<typename T, int MinZ, int MaxZ>
  class MTArrayBoxStrip
  {
  public:
    MTArrayBoxStrip(T* begin, const std::shared_ptr<std::unique_lock<std::shared_mutex>>& lock)
      : m_Begin(begin), m_Lock(lock) {}
  
    T& operator[](int index)
    {
      return const_cast<T&>(static_cast<const MTArrayBoxStrip*>(this)->operator[](index));
    }
    const T& operator[](int index) const
    {
      EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, MinZ, MaxZ), "Index is out of bounds!");
      return m_Begin[index - MinZ];
    }
  
  private:
    T* m_Begin;
    std::shared_ptr<std::unique_lock<std::shared_mutex>> m_Lock;
  };
  
  template<typename T, int MinY, int MaxY, int MinZ, int MaxZ>
  class MTArrayBoxLayer
  {
  public:
    MTArrayBoxLayer(T* begin, std::unique_lock<std::shared_mutex>&& lock)
      : m_Begin(begin), m_Lock(std::make_shared<std::unique_lock<std::shared_mutex>>(std::move(lock))) {}
  
    MTArrayBoxStrip<T, MinZ, MaxZ> operator[](int index)
    {
      return static_cast<const MTArrayBoxLayer&>(*this).operator[](index);
    }
    const MTArrayBoxStrip<T, MinZ, MaxZ> operator[](int index) const
    {
      EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, MinY, MaxY), "Index is out of bounds!");
      return MTArrayBoxStrip<T, MinZ, MaxZ>(m_Begin + (MaxZ - MinZ) * (index - MinY), m_Lock);
    }
  
  private:
    T* m_Begin;
    std::shared_ptr<std::unique_lock<std::shared_mutex>> m_Lock;
  };
  
  
  
  /*
    Thread-safe version of ArrayBox.
  */
  template<typename T, int MinX, int MaxX, int MinY = MinX, int MaxY = MaxX, int MinZ = MinX, int MaxZ = MaxX>
  class MTArrayBox : Engine::NonCopyable, Engine::NonMovable
  {
  public:
    MTArrayBox()
      : m_Data(nullptr) {}
    MTArrayBox(AllocationPolicy policy)
      : m_Data(policy) {}
    MTArrayBox(const T& initialValue)
      : m_Data(initialValue) {}
  
    operator bool() const { return m_Data; }
  
    template<std::integral IndexType>
    T operator()(const IVec3<IndexType>& index)
    {
      return static_cast<const MTArrayBox*>(this)->operator()(index);
    }
  
    template<std::integral IndexType>
    const T operator()(const IVec3<IndexType>& index) const
    {
      return m_Data(index);
    }
  
    _Acquires_lock_(return) MTArrayBoxLayer<T, MinY, MaxY, MinZ, MaxZ> operator[](int index)
    {
      return static_cast<const MTArrayBox*>(this)->operator[](index);
    }
    _Acquires_lock_(return) const MTArrayBoxLayer<T, MinY, MaxY, MinZ, MaxZ> operator[](int index) const
    {
      EN_CORE_ASSERT(m_Data.get(), "Data has not yet been allocated!");
      EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, MinX, MaxX), "Index is out of bounds!");
  
      static constexpr size_t stride = static_cast<size_t>(MaxY - MinY) * static_cast<size_t>(MaxZ - MinZ);
      return MTArrayBoxLayer<T, MinY, MaxY, MinZ, MaxZ>(m_Data.get() + stride * index, std::unique_lock(m_Mutex));
    }
  
    constexpr size_t size() const { return m_Data.size(); }
  
    template<std::signed_integral IndexType>
    constexpr IBox3<IndexType> box() const { return m_Data.box(); }
  
    bool contains(const T& value) const
    {
      std::shared_lock lock(m_Mutex);
      return m_Data.contains(value);
    }
  
    bool filledWith(const T& value) const
    {
      std::shared_lock lock(m_Mutex);
      return m_Data.filledWith(value);
    }
  
    template<std::integral IndexType>
    bool filledWith(const IBox3<IndexType>& section, const T& value) const
    {
      std::shared_lock lock(m_Mutex);
      return m_Data.filledWith(section, value);
    }
  
    template<std::integral IndexType, int... Args>
    bool contentsEqual(const IBox3<IndexType>& compareSection, const ArrayBox<T, Args...>& arr, const IBox3<IndexType>& arrSection) const
    {
      std::shared_lock lock(m_Mutex);
      return m_Data.contentsEqual(compareSection, arr, arrSection);
    }
  
    void fill(const T& value)
    {
      std::lock_guard lock(m_Mutex);
      m_Data.fill(value);
    }
  
    template<std::integral IndexType>
    void fill(const IBox3<IndexType>& fillSection, const T& value)
    {
      std::lock_guard lock(m_Mutex);
      m_Data.fill(fillSection, value);
    }
  
    template<std::integral IndexType, int... Args>
    void fill(const IBox3<IndexType>& fillSection, const ArrayBox<T, Args...>& arr, const IBox3<IndexType>& arrSection)
    {
      std::lock_guard lock(m_Mutex);
      m_Data.fill(fillSection, arr, arrSection);
    }
  
    void reset()
    {
      std::lock_guard lock(m_Mutex);
      m_Data.reset();
    }
  
  private:
    mutable std::shared_mutex m_Mutex;
    ArrayBox<T, MinX, MaxX, MinY, MaxY, MinZ, MaxZ> m_Data;
  };
}