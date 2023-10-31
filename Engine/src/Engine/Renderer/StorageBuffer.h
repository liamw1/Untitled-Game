#pragma once
#include "Engine/Core/FixedWidthTypes.h"

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

    virtual ~StorageBuffer();

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual u32 size() const = 0;
    virtual Type type() const = 0;

    virtual void set(const void* data, u32 size) = 0;
    virtual void update(const void* data, u32 offset, u32 size) = 0;
    virtual void resize(u32 newSize) = 0;

    static std::unique_ptr<StorageBuffer> Create(Type type, std::optional<u32> binding = std::nullopt);
  };
}