#pragma once

enum class ShaderDataType : uint8_t
{
  None,
  Bool,
  Uint32,
  Int, Int2, Int3, Int4,
  Float, Float2, Float3, Float4,
  Mat3, Mat4
};

namespace Engine
{
  static uint32_t shaderDataTypeSize(ShaderDataType type)
  {
    switch (type)
    {
      case ShaderDataType::Bool:        return 1 * sizeof(bool);
      case ShaderDataType::Int:         return 1 * sizeof(int);
      case ShaderDataType::Uint32:      return 1 * sizeof(uint32_t);
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

    uint32_t getComponentCount() const
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
        case ShaderDataType::Mat3:        return 3;   // 3x float3
        case ShaderDataType::Mat4:        return 4;   // 4x float4
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

    uint32_t getStride() const { return m_Stride; }
    const std::vector<BufferElement>& getElements() const { return m_Elements; }

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
      for (auto& element : m_Elements)
      {
        element.offset = m_Stride;
        m_Stride += element.size;
      }
    }
  };



  /*
    Abstract representation of a vertex buffer.
    Platform-specific implementation is determined by derived class.
  */
  class VertexBuffer
  {
  public:
    virtual ~VertexBuffer() {}

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual const BufferLayout& getLayout() const = 0;
    virtual void setLayout(const BufferLayout& layout) = 0;

    /*
      \param data Buffer of vertex data
      \param size Size of buffer in bytes
    */
    virtual void setData(const void* data, uintptr_t size) = 0;

    static Shared<VertexBuffer> Create(uint32_t size);
    static Shared<VertexBuffer> Create(float* vertices, uint32_t size);
  };



  /*
    Abstract representation of an index buffer.
    Platform-specific implementation is determined by derived class.

    NOTE: Currently Engine only supports 32-bit index buffers
  */
  class IndexBuffer
  {
  public:
    virtual ~IndexBuffer() {}

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual uint32_t getCount() const = 0;

    /*
      \param indices Buffer of vertex indices
      \param count   Number of indices
    */
    static Shared<IndexBuffer> Create(uint32_t* indices, uint32_t count);
  };
}