#pragma once

namespace Engine
{
  enum class ShaderDataType : uint8_t
  {
    None,
    Bool,
    Int, Int2, Int3, Int4,
    Float, Float2, Float3, Float4,
    Mat3, Mat4
  };

  static uint32_t shaderDataTypeSize(ShaderDataType type)
  {
    switch (type)
    {
      case ShaderDataType::Bool:        return 1;
      case ShaderDataType::Int:         return 4;
      case ShaderDataType::Int2:        return 2 * 4;
      case ShaderDataType::Int3:        return 3 * 4;
      case ShaderDataType::Int4:        return 4 * 4;
      case ShaderDataType::Float:       return 4;
      case ShaderDataType::Float2:      return 2 * 4;
      case ShaderDataType::Float3:      return 3 * 4;
      case ShaderDataType::Float4:      return 4 * 4;
      case ShaderDataType::Mat3:        return 3 * 3 * 4;
      case ShaderDataType::Mat4:        return 4 * 4 * 4;
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

    BufferElement() {}
    BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
      : name(name), type(type), size(shaderDataTypeSize(type)), offset(0), normalized(normalized) {}

    uint32_t getComponentCount() const
    {
      switch (type)
      {
        case ShaderDataType::Bool:        return 1;
        case ShaderDataType::Int:         return 1;
        case ShaderDataType::Int2:        return 2;
        case ShaderDataType::Int3:        return 3;
        case ShaderDataType::Int4:        return 4;
        case ShaderDataType::Float:       return 1;
        case ShaderDataType::Float2:      return 2;
        case ShaderDataType::Float3:      return 3;
        case ShaderDataType::Float4:      return 4;
        case ShaderDataType::Mat3:        return 3 * 3;
        case ShaderDataType::Mat4:        return 4 * 4;
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

    inline uint32_t getStride() const { return m_Stride; }
    inline const std::vector<BufferElement>& getElements() const { return m_Elements; }

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

  class VertexBuffer
  {
  public:
    virtual ~VertexBuffer() {}

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual const BufferLayout& getLayout() const = 0;
    virtual void setLayout(const BufferLayout& layout) = 0;

    static VertexBuffer* Create(float* vertices, uint32_t size);
  };

  class IndexBuffer
  {
  public:
    virtual ~IndexBuffer() {}

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual uint32_t getCount() const = 0;

    static IndexBuffer* Create(uint32_t* indices, uint32_t count);
  };
}