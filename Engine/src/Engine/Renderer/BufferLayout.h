#pragma once
#include "Engine/Core/Log.h"

/*
  Indicates underyling type of data to be send to shader.

  There are three basic categories of data: ints, floats, and mats.
  These are handled differently from each other, and so it is important
  that they are grouped together.

  Int types should be enumerated first, then floats, then mats.
  The beginning and ending of these groups are given their own identifier,
  which is used in the vertex array for differentiating between these types.
*/
enum class ShaderDataType : int
{
  None,
  Bool,
  Uint32,
  Int, Int2, Int3, Int4,
  Float, Float2, Float3, Float4,
  Mat3, Mat4,

  IntTypeBegin = Bool,    IntTypeEnd = Int4,
  FloatTypeBegin = Float, FloatTypeEnd = Float4,
  MatTypeBegin = Mat3,    MatTypeEnd = Mat4
};

namespace Engine
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
      default: EN_CORE_ASSERT(false, "Unknown ShaderDataType!"); return 0;
    }
  }

  struct BufferElement
  {
    std::string name;
    ShaderDataType type;
    uint32_t size;
    uint32_t offset;
    bool normalized;

    BufferElement() 
      : name(""), type(ShaderDataType::None), size(0), offset(0), normalized(false) {}
    BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
      : name(name), type(type), size(shaderDataTypeSize(type)), offset(0), normalized(normalized) {}

    /*
      Gets the number of components packed into the buffer element type.
      Note that for mats, the underlying component is a float vector, so
      the component count of a mat4 is 4, not 16.
    */
    int getComponentCount() const
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
        default: EN_CORE_ASSERT(false, "Unknown ShaderDataType!"); return 0;
      }
    }
  };



  class BufferLayout
  {
  public:
    BufferLayout() {}
    BufferLayout(const std::initializer_list<BufferElement>& elements)
    : m_Elements(elements) 
    {
      calculateOffsetsAndStride();
    }

    uint32_t stride() const { return m_Stride; }
    const std::vector<BufferElement>& elements() const { return m_Elements; }

    std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
    std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
    std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
    std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }

  private:
    std::vector<BufferElement> m_Elements;
    uint32_t m_Stride = 0;

    void calculateOffsetsAndStride()
    {
      m_Stride = 0;
      for (BufferElement& element : m_Elements)
      {
        element.offset = m_Stride;
        m_Stride += element.size;
      }
    }
  };
}