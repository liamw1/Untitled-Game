#include "ENpch.h"
#include "BufferLayout.h"
#include "Engine/Core/Exception.h"

namespace eng::mem
{
  static u32 shaderDataTypeSize(ShaderDataType type)
  {
    switch (type)
    {
      case ShaderDataType::Bool:        return 1 * sizeof(bool);
      case ShaderDataType::Unsigned:    return 1 * sizeof(u32);
      case ShaderDataType::Int:         return 1 * sizeof(i32);
      case ShaderDataType::Int2:        return 2 * sizeof(i32);
      case ShaderDataType::Int3:        return 3 * sizeof(i32);
      case ShaderDataType::Int4:        return 4 * sizeof(i32);
      case ShaderDataType::Float:       return 1 * sizeof(f32);
      case ShaderDataType::Float2:      return 2 * sizeof(f32);
      case ShaderDataType::Float3:      return 3 * sizeof(f32);
      case ShaderDataType::Float4:      return 4 * sizeof(f32);
      case ShaderDataType::Mat3:        return 3 * 3 * sizeof(f32);
      case ShaderDataType::Mat4:        return 4 * 4 * sizeof(f32);
    }
    throw CoreException("Invalid ShaderDataType!");
  }



  BufferElement::BufferElement()
    : name(""), type(ShaderDataType::None), size(0), offset(0), normalized(false) {}
  BufferElement::BufferElement(ShaderDataType type, const std::string& name, bool normalized)
    : name(name), type(type), size(shaderDataTypeSize(type)), offset(0), normalized(normalized) {}

  i32 BufferElement::getComponentCount() const
  {
    switch (type)
    {
      case ShaderDataType::Bool:      return 1;
      case ShaderDataType::Unsigned:  return 1;
      case ShaderDataType::Int:       return 1;
      case ShaderDataType::Int2:      return 2;
      case ShaderDataType::Int3:      return 3;
      case ShaderDataType::Int4:      return 4;
      case ShaderDataType::Float:     return 1;
      case ShaderDataType::Float2:    return 2;
      case ShaderDataType::Float3:    return 3;
      case ShaderDataType::Float4:    return 4;
      case ShaderDataType::Mat3:      return 3;
      case ShaderDataType::Mat4:      return 4;
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