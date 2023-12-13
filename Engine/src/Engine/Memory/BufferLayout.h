#pragma once
#include "Engine/Core/FixedWidthTypes.h"
#include "Engine/Utilities/BoilerplateReduction.h"

namespace eng::mem
{
  /*
    Indicates underyling type of data to be send to shader.

    There are three basic categories of data: ints, floats, and mats.
    These are handled differently from each other, and so it is important
    that they are grouped together.

    Int types should be enumerated first, then floats, then mats.
    The beginning and ending of these groups are given their own identifier,
    which is used in the vertex array for differentiating between these types.
  */
  enum class ShaderDataType : i32
  {
    None,
    Bool,
    Unsigned,
    Int, Int2, Int3, Int4,
    Float, Float2, Float3, Float4,
    Mat3, Mat4,
  
    FirstInt = Bool,    LastInt = Int4,
    FirstFloat = Float, LastFloat = Float4,
    FirstMat = Mat3,    LastMat = Mat4
  };

  struct BufferElement
  {
    std::string name;
    ShaderDataType type;
    u32 size;
    u32 offset;
    bool normalized;

    BufferElement();
    BufferElement(ShaderDataType type, const std::string& name, bool normalized = false);

    /*
      Gets the number of components packed into the buffer element type.
      Note that for mats, the underlying component is a f32 vector, so
      the component count of a mat4 is 4, not 16.
    */
    i32 getComponentCount() const;
  };

  class BufferLayout
  {
    std::vector<BufferElement> m_Elements;
    u32 m_Stride;

  public:
    BufferLayout();
    BufferLayout(const std::initializer_list<BufferElement>& elements);

    u32 stride() const;
    const std::vector<BufferElement>& elements() const;

    ENG_DEFINE_CONSTEXPR_ITERATORS(m_Elements);

  private:
    void calculateOffsetsAndStride();
  };
}