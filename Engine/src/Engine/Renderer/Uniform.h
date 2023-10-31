#pragma once
#include "Engine/Core/FixedWidthTypes.h"

namespace eng
{
  class Uniform
  {
  public:
    virtual ~Uniform();

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual void set(const void* data, u32 size) = 0;

    static std::unique_ptr<Uniform> Create(u32 binding, u32 size);
  };
}