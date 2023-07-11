#pragma once

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

namespace Engine
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
    uint32_t width;
    uint32_t height;
    uint32_t samples;
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
    virtual uint32_t getColorAttachmentRendererID(uint32_t index = 0) const = 0;

    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual int readPixel(uint32_t attachmentIndex, int x, int y) = 0;

    virtual void clearAttachment(uint32_t attachmentIndex, int value) = 0;
    virtual void clearAttachment(uint32_t attachmentIndex, float value) = 0;

    static std::unique_ptr<Framebuffer> Create(const FramebufferSpecification& specification);
  };
}