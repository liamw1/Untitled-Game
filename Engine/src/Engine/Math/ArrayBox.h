#pragma once
#include "IBox3.h"
#include "Engine/Utilities/Constraints.h"

enum class AllocationPolicy
{
  ForOverWrite
};

template<typename T, std::integral IntType, IntType MinZ, IntType MaxZ>
class ArrayBoxStrip
{
public:
  ArrayBoxStrip(T* begin)
    : m_Begin(begin) {}

  T& operator[](IntType index)
  {
    return const_cast<T&>(static_cast<const ArrayBoxStrip*>(this)->operator[](index));
  }
  const T& operator[](IntType index) const
  {
    EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, MinZ, MaxZ), "Index is out of bounds!");
    return m_Begin[index - MinZ];
  }

private:
  T* m_Begin;

  ~ArrayBoxStrip() = default;
};

template<typename T, std::integral IntType, IntType MinY, IntType MaxY, IntType MinZ, IntType MaxZ>
class ArrayBoxLayer
{
public:
  using Strip = ArrayBoxStrip<T, IntType, MinZ, MaxZ>;

public:
  ArrayBoxLayer(T* begin)
    : m_Begin(begin) {}

  Strip operator[](IntType index)
  {
    return static_cast<const ArrayBoxLayer&>(*this).operator[](index);
  }
  const Strip operator[](IntType index) const
  {
    EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, MinY, MaxY), "Index is out of bounds!");
    return Strip(m_Begin + (MaxZ - MinZ) * (index - MinY));
  }

private:
  T* m_Begin;

  ~ArrayBoxLayer() = default;
};



/*
  A 3D-style array that stores its members in a single
  heap-allocated block in memory.

  Elements can be accessed with brackets:
  arr[i][j][k]
*/
template<typename T, std::integral IntType, IntType MinX, IntType MaxX, IntType MinY = MinX, IntType MaxY = MaxX, IntType MinZ = MinX, IntType MaxZ = MaxX>
class ArrayBox : private Engine::NonCopyable
{
public:
  using Layer = ArrayBoxLayer<T, IntType, MinY, MaxY, MinZ, MaxZ>;
  using Strip = ArrayBoxStrip<T, IntType, MinZ, MaxZ>;

public:
  ArrayBox()
    : m_Data(nullptr) {}
  ArrayBox(AllocationPolicy policy)
    : m_Data(new T[size()]) {}
  ArrayBox(const T& initialValue)
    : ArrayBox(AllocationPolicy::ForOverWrite) { fill(initialValue); }
  ~ArrayBox() { delete[] m_Data; }

  ArrayBox(ArrayBox&& other) noexcept = default;
  ArrayBox& operator=(ArrayBox&& other) noexcept
  {
    if (&other != this)
    {
      delete[] m_Data;
      m_Data = other.m_Data;
      other.m_Data = nullptr;
    }
    return *this;
  }

  operator bool() const { return m_Data; }

  T& operator()(const IVec3<IntType>& index)
  {
    return const_cast<T&>(static_cast<const ArrayBox*>(this)->operator()(index));
  }

  const T& operator()(const IVec3<IntType>& index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");

    static constexpr IVec3<int> extents = static_cast<IVec3<int>>(c_Bounds.extents());
    static constexpr IVec3<size_t> strides(extents.j * extents.k, extents.k, 1);

    IVec3<IntType> indexRelativeToBase = index - c_Bounds.min;
    return m_Data[strides.i * indexRelativeToBase.i + strides.j * indexRelativeToBase.j + strides.k * indexRelativeToBase.k];
  }

  Layer operator[](IntType index)
  {
    return static_cast<const ArrayBox*>(this)->operator[](index);
  }
  const Layer operator[](IntType index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, MinX, MaxX), "Index is out of bounds!");

    static constexpr size_t stride = static_cast<size_t>(MaxY - MinY) * static_cast<size_t>(MaxZ - MinZ);
    return Layer(m_Data + stride * index);
  }

  T* get() const { return m_Data; }

  constexpr size_t size() const { return c_Bounds.volume(); }
  constexpr IBox3<IntType> bounds() const { return c_Bounds; }

  bool contains(const T& value) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return std::any_of(m_Data, m_Data + size(), [&value](const T& data)
      {
        return data == value;
      });
  }

  bool filledWith(const T& value) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return std::all_of(m_Data, m_Data + size(), [&value](const T& data)
      {
        return data == value;
      });
  }

  template<IntType... Args>
  bool contentsEqual(const IBox3<IntType>& compareSection, const ArrayBox<T, IntType, Args...>& container, const IBox3<IntType>& containerSection, const T& defaultValue) const
  {
    EN_CORE_ASSERT(compareSection.extents() == containerSection.extents(), "Compared sections are not the same dimensions!");

    if (!m_Data && !container)
      return true;
    if (!m_Data)
      return container.allOf(containerSection, [&defaultValue](const T& value) { return value == defaultValue; });
    if (!container)
      return this->allOf(compareSection, [&defaultValue](const T& value) { return value == defaultValue; });

    IVec3<IntType> offset = containerSection.min - compareSection.min;
    return compareSection.allOf([this, &container, &offset](const IVec3<IntType>& index)
      {
        return (*this)(index) == container(index + offset);
      });
  }

  template<InvocableWithReturnType<bool, T> F>
  bool allOf(const IBox3<IntType>& section, const F& condition) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.allOf([this, &condition](const IVec3<IntType>& index)
      {
        return condition((*this)(index));
      });
  }

  template<InvocableWithReturnType<bool, T> F>
  bool anyOf(const IBox3<IntType>& section, const F& condition) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.anyOf([this, &condition](const IVec3<IntType>& index)
      {
        return condition((*this)(index));
      });
  }

  template<InvocableWithReturnType<bool, T> F>
  bool noneOf(const IBox3<IntType>& section, const F& condition) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.noneOf([this, &condition](const IVec3<IntType>& index)
      {
        return condition((*this)(index));
      });
  }

  void fill(const T& value)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    std::fill(m_Data, m_Data + size(), value);
  }

  void fill(const IBox3<IntType>& fillSection, const T& value)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    fillSection.forEach([this, &value](const IVec3<IntType>& index)
      {
        (*this)(index) = value;
      });
  }

  template<IntType... Args>
  void fill(const IBox3<IntType>& fillSection, const ArrayBox<T, IntType, Args...>& container, const IBox3<IntType>& containerSection, const T& defaultValue)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(fillSection.extents() == containerSection.extents(), "Read and write sections are not the same dimensions!");

    if (!container)
    {
      fill(fillSection, defaultValue);
      return;
    }

    IVec3<IntType> offset = containerSection.min - fillSection.min;
    fillSection.forEach([this, &container, &offset](const IVec3<IntType>& index)
      {
        (*this)(index) = container(index + offset);
      });
  }

  void allocate()
  {
    if (m_Data)
    {
      EN_CORE_WARN("Data already allocated to ArrayBox. Ignoring...");
      return;
    }

    m_Data = new T[size()];
  }

  void reset()
  {
    delete[] m_Data;
    m_Data = nullptr;
  }

  static constexpr const IBox3<IntType>& Bounds() { return c_Bounds; }

private:
  static constexpr IBox3<IntType> c_Bounds = IBox3<IntType>(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);

  T* m_Data;
};