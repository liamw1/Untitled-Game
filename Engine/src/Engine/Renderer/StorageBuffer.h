#pragma once

namespace eng
{
  class StorageBuffer
  {
  public:
    enum class Type
    {
      VertexBuffer,
      IndexBuffer,
      SSBO
    };

  public:
    virtual ~StorageBuffer();

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual uint32_t size() const = 0;
    virtual Type type() const = 0;

    virtual void set(const void* data, uint32_t size) = 0;
    virtual void update(const void* data, uint32_t offset, uint32_t size) = 0;
    virtual void resize(uint32_t newSize) = 0;

    static std::unique_ptr<StorageBuffer> Create(Type type, std::optional<uint32_t> binding = std::nullopt);
  };
}