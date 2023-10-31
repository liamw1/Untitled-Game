#pragma once
#include "Engine/Core/FixedWidthTypes.h"

enum class FramebufferTextureFormat
{
  None,

  // Color
  RED_INTEGER,
  RGBA8,

  // Depth/stencil
  DEPTH24STENCIL8,

  // Defaults
  Depth = DEPTH24STENCIL8
};

namespace eng
{
  struct FramebufferTextureSpecification
  {
    FramebufferTextureFormat textureFormat;
    // TODO: Filtering/wrap

    FramebufferTextureSpecification();
    FramebufferTextureSpecification(FramebufferTextureFormat format);
  };

  struct FramebufferSpecification
  {
    u32 width;
    u32 height;
    u32 samples;
    bool swapChainTarget;

    std::vector<FramebufferTextureSpecification> attachments;

    FramebufferSpecification();
    FramebufferSpecification(const std::initializer_list<FramebufferTextureSpecification>& attachmentList);
  };

  class Framebuffer
  {
  public:
    virtual ~Framebuffer() = default;

    virtual void bind() = 0;
    virtual void unbind() = 0;

    virtual const FramebufferSpecification& specification() const = 0;
    virtual u32 getColorAttachmentRendererID(u32 index = 0) const = 0;

    virtual void resize(u32 width, u32 height) = 0;
    virtual i32 readPixel(u32 attachmentIndex, i32 x, i32 y) = 0;

    virtual void clearAttachment(u32 attachmentIndex, i32 value) = 0;
    virtual void clearAttachment(u32 attachmentIndex, f32 value) = 0;

    static std::unique_ptr<Framebuffer> Create(const FramebufferSpecification& specification);
  };
}