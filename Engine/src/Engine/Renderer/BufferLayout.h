#pragma once

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
  struct BufferElement
  {
    std::string name;
    ShaderDataType type;
    uint32_t size;
    uint32_t offset;
    bool normalized;

    BufferElement();
    BufferElement(ShaderDataType type, const std::string& name, bool normalized = false);

    /*
      Gets the number of components packed into the buffer element type.
      Note that for mats, the underlying component is a float vector, so
      the component count of a mat4 is 4, not 16.
    */
    int getComponentCount() const;
  };

  class BufferLayout
  {
  public:
    BufferLayout();
    BufferLayout(const std::initializer_list<BufferElement>& elements);

    uint32_t stride() const;
    const std::vector<BufferElement>& elements() const;

    std::vector<BufferElement>::iterator begin();
    std::vector<BufferElement>::iterator end();
    std::vector<BufferElement>::const_iterator begin() const;
    std::vector<BufferElement>::const_iterator end() const;

  private:
    std::vector<BufferElement> m_Elements;
    uint32_t m_Stride;

    void calculateOffsetsAndStride();
  };
}