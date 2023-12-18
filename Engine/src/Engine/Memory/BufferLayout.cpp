#include "ENpch.h"
#include "BufferLayout.h"
#include "Engine/Core/Exception.h"

namespace eng::mem
{
  static u32 dataTypeSize(DataType type)
  {
    switch (type)
    {
      case DataType::Bool:        return 1 * sizeof(bool);
      case DataType::Unsigned:    return 1 * sizeof(u32);
      case DataType::Int:         return 1 * sizeof(i32);
      case DataType::Int2:        return 2 * sizeof(i32);
      case DataType::Int3:        return 3 * sizeof(i32);
      case DataType::Int4:        return 4 * sizeof(i32);
      case DataType::Float:       return 1 * sizeof(f32);
      case DataType::Float2:      return 2 * sizeof(f32);
      case DataType::Float3:      return 3 * sizeof(f32);
      case DataType::Float4:      return 4 * sizeof(f32);
      case DataType::Mat3:        return 3 * 3 * sizeof(f32);
      case DataType::Mat4:        return 4 * 4 * sizeof(f32);
    }
    throw CoreException("Invalid ShaderDataType!");
  }



  BufferElement::BufferElement(DataType type, const std::string& name, bool normalized)
    : name(name), type(type), size(dataTypeSize(type)), offset(0), normalized(normalized) {}

  i32 BufferElement::getComponentCount() const
  {
    switch (type)
    {
      case DataType::Bool:      return 1;
      case DataType::Unsigned:  return 1;
      case DataType::Int:       return 1;
      case DataType::Int2:      return 2;
      case DataType::Int3:      return 3;
      case DataType::Int4:      return 4;
      case DataType::Float:     return 1;
      case DataType::Float2:    return 2;
      case DataType::Float3:    return 3;
      case DataType::Float4:    return 4;
      case DataType::Mat3:      return 3;
      case DataType::Mat4:      return 4;
    }
    throw CoreException("Invalid ShaderDataType!");
  }



  BufferLayout::BufferLayout()
    : BufferLayout({}) {}
  BufferLayout::BufferLayout(const std::initializer_list<BufferElement>& elements)
    : m_Elements(elements), m_Stride(0) { calculateOffsetsAndStride(); }

  u32 BufferLayout::stride() const { return m_Stride; }
  const std::vector<BufferElement>& BufferLayout::elements() const { return m_Elements; }

  void BufferLayout::calculateOffsetsAndStride()
  {
    m_Stride = 0;
    for (BufferElement& element : m_Elements)
    {
      element.offset = m_Stride;
      m_Stride += element.size;
    }
  }
}