#pragma once
#include "Data.h"

namespace eng::mem
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

    virtual ~StorageBuffer();

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual u32 size() const = 0;
    virtual Type type() const = 0;

    virtual void set(const mem::Data& data) = 0;
    virtual void modify(u32 offset, const mem::Data& data) = 0;
    virtual void resize(u32 newSize) = 0;

    static std::unique_ptr<StorageBuffer> Create(Type type, std::optional<u32> binding = std::nullopt);
  };
}