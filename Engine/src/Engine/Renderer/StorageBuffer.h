#pragma once

namespace Engine
{
  class StorageBuffer
  {
  public:
    enum class Type
    {
      VertexBuffer,
      SSBO
    };

  public:
    virtual ~StorageBuffer();

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual void set(const void* data, uint32_t size) = 0;
    virtual void update(const void* data, uint32_t offset, uint32_t size) = 0;
    virtual void resize(uint32_t newSize) = 0;

    void update(const void* data, uint64_t offset, uint64_t size);

    static std::unique_ptr<StorageBuffer> Create(Type type, std::optional<uint32_t> binding = std::nullopt);
  };
}