#pragma once

namespace Engine
{
  struct FramebufferSpecification
  {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t samples = 1;

    bool swapChainTarget = false;
  };

  class Framebuffer
  {
  public:
    virtual ~Framebuffer() = default;

    virtual void bind() = 0;
    virtual void unbind() = 0;

    virtual const FramebufferSpecification& getSpecialization() const = 0;
    virtual uint32_t getColorAttachmentRendererID() const = 0;

    virtual void resize(uint32_t width, uint32_t height) = 0;

    static Shared<Framebuffer> Create(const FramebufferSpecification& specification);
  };
}