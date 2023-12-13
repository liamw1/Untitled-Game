#include "ENpch.h"
#include "Data.h"

namespace eng::mem
{
  const void* GenericData::raw() const { return m_Data; }
  uSize GenericData::size() const { return m_ElementSize * m_ElementCount; }
  uSize GenericData::elementSize() const { return m_ElementSize; }
  uSize GenericData::elementCount() const { return m_ElementCount; }
  bool GenericData::empty() const { return size() == 0; }

  GenericData::GenericData()
    : GenericData(nullptr, 0, 0) {}
  GenericData::GenericData(const void* data, uSize elementSize, uSize elementCount)
    : m_Data(data), m_ElementSize(elementSize), m_ElementCount(elementCount) {}



  RenderData::RenderData() = default;
  RenderData::RenderData(const void* data, uSize elementSize, uSize elementCount)
    : GenericData(data, elementSize, elementCount) {}



  IndexData::IndexData() = default;
  IndexData::IndexData(const std::vector<u32>& vector)
    : GenericData(vector.data(), sizeof(u32), vector.size()) {}

  IndexData::operator RenderData() const { return RenderData(raw(), elementSize(), elementCount()); }



  UniformData::UniformData() = default;
}
