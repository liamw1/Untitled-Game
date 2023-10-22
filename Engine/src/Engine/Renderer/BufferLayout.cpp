#include "ENpch.h"
#include "BufferLayout.h"

namespace eng
{
  static uint32_t shaderDataTypeSize(ShaderDataType type)
  {
    switch (type)
    {
      case ShaderDataType::Bool:        return 1 * sizeof(bool);
      case ShaderDataType::Uint32:      return 1 * sizeof(uint32_t);
      case ShaderDataType::Int:         return 1 * sizeof(int);
      case ShaderDataType::Int2:        return 2 * sizeof(int);
      case ShaderDataType::Int3:        return 3 * sizeof(int);
      case ShaderDataType::Int4:        return 4 * sizeof(int);
      case ShaderDataType::Float:       return 1 * sizeof(float);
      case ShaderDataType::Float2:      return 2 * sizeof(float);
      case ShaderDataType::Float3:      return 3 * sizeof(float);
      case ShaderDataType::Float4:      return 4 * sizeof(float);
      case ShaderDataType::Mat3:        return 3 * 3 * sizeof(float);
      case ShaderDataType::Mat4:        return 4 * 4 * sizeof(float);
      default: throw std::invalid_argument("Unknown ShaderDataType!");
    }
  }



  BufferElement::BufferElement()
    : name(""), type(ShaderDataType::None), size(0), offset(0), normalized(false) {}
  BufferElement::BufferElement(ShaderDataType type, const std::string& name, bool normalized)
    : name(name), type(type), size(shaderDataTypeSize(type)), offset(0), normalized(normalized) {}

  int BufferElement::getComponentCount() const
  {
    switch (type)
    {
      case ShaderDataType::Bool:        return 1;
      case ShaderDataType::Uint32:      return 1;
      case ShaderDataType::Int:         return 1;
      case ShaderDataType::Int2:        return 2;
      case ShaderDataType::Int3:        return 3;
      case ShaderDataType::Int4:        return 4;
      case ShaderDataType::Float:       return 1;
      case ShaderDataType::Float2:      return 2;
      case ShaderDataType::Float3:      return 3;
      case ShaderDataType::Float4:      return 4;
      case ShaderDataType::Mat3:        return 3;
      case ShaderDataType::Mat4:        return 4;
      default: throw std::invalid_argument("Unknown ShaderDataType!");
    }
  }



  BufferLayout::BufferLayout()
    : BufferLayout({}) {}
  BufferLayout::BufferLayout(const std::initializer_list<BufferElement>& elements)
    : m_Elements(elements), m_Stride(0) { calculateOffsetsAndStride(); }

  uint32_t BufferLayout::stride() const { return m_Stride; }
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