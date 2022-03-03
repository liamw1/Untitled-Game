#pragma once

namespace Engine
{
  struct FramebufferSpecification
  {
    uint32_t width;
    uint32_t height;
    uint32_t samples = 1;

    bool swapChainTarget = false;
  };

  class Framebuffer
  {
  public:
    virtual void bind() = 0;
    virtual void unbind() = 0;

    virtual const FramebufferSpecification& getSpecialization() const = 0;
    virtual uint32_t getColorAttachmentRendererID() const = 0;

    static Shared<Framebuffer> Create(const FramebufferSpecification& specification);
  };
}