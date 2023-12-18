#pragma once
#include "Data.h"

namespace eng::mem
{
  class StorageBuffer
  {
  public:
    enum class Type
    {
      Uniform,
      SSBO
    };

    virtual ~StorageBuffer();

    virtual Type type() const = 0;
    virtual uSize size() const = 0;

    virtual void write(const mem::RenderData& data) = 0;

    static std::unique_ptr<StorageBuffer> Create(Type type, u32 binding, uSize size);
  };
}