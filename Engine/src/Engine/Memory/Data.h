#pragma once
#include "Engine/Core/Concepts.h"
#include "Engine/Utilities/Constraints.h"
#include "Engine/Utilities/EnumUtilities.h"

namespace eng::mem
{
  class GenericData
  {
    const void* m_Data;
    uSize m_ElementSize;
    uSize m_ElementCount;

  public:
    const void* raw() const;
    uSize size() const;
    uSize elementSize() const;
    uSize elementCount() const;
    bool empty() const;

  protected:
    GenericData();
    GenericData(const void* data, uSize elementSize, uSize elementCount);
  };

  class RenderData : public GenericData
  {
  public:
    RenderData();

    template<typename T>
    RenderData(const std::vector<T>& vector)
      : GenericData(vector.data(), sizeof(T), vector.size()) {}

    template<typename T, uSize N>
    RenderData(const std::array<T, N>& arr)
      : GenericData(arr.data(), sizeof(T), N) {}

    template<typename T, IterableEnum E>
    RenderData(const EnumArray<T, E>& arr)
      : GenericData(arr.data(), sizeof(T), arr.size()) {}

  private:
    RenderData(const void* data, uSize elementSize, uSize elementCount);

    friend class IndexData;
    friend class UniformData;
  };

  class IndexData : public GenericData
  {
  public:
    IndexData();
    IndexData(const std::vector<u32>& vector);

    template<uSize N>
    IndexData(const std::array<u32, N>& arr)
      : GenericData(arr.data(), sizeof(u32), N) {}

    template<IterableEnum E>
    IndexData(const EnumArray<u32, E>& arr)
      : GenericData(arr.data(), sizeof(u32), arr.size()) {}

    explicit operator RenderData() const;
  };

  class UniformData : public GenericData
  {
  public:
    UniformData();

    template<StandardLayout T>
    UniformData(const T& uniform)
      : GenericData(&uniform, sizeof(T), 1) {}

    explicit operator RenderData() const;
  };
}