#include "ENpch.h"
#include "OpenGLFramebuffer.h"
#include "Engine/Threads/Threads.h"
#include <glad/glad.h>

namespace eng
{
  static constexpr u32 c_MaxFrameBufferSize = 8192;

  static GLenum textureTarget(bool multiSampled) { return multiSampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D; }
  static void createTextures(bool multiSampled, u32* outID, u32 count) { glCreateTextures(textureTarget(multiSampled), count, outID); }
  static void bindTexture(bool multiSampled, u32 id) { glBindTexture(textureTarget(multiSampled), id); }

  static void attachColorTexture(u32 id, const FramebufferSpecification& fbSpecs, GLenum internalFormat, GLenum format, i32 index)
  {
    const u32& samples = fbSpecs.samples;
    const u32& width = fbSpecs.width;
    const u32& height = fbSpecs.height;

    bool multisampled = samples > 1;
    if (multisampled)
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
    else
    {
      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, textureTarget(multisampled), id, 0);
  }

  static void attachDepthTexture(u32 id, const FramebufferSpecification& fbSpecs, GLenum format, GLenum attachmentType)
  {
    const u32& samples = fbSpecs.samples;
    const u32& width = fbSpecs.width;
    const u32& height = fbSpecs.height;

    bool multisampled = samples > 1;
    if (multisampled)
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
    else
    {
      glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, textureTarget(multisampled), id, 0);
  }

  static bool isDepthFormat(FramebufferTextureFormat format)
  {
    switch (format)
    {
      case FramebufferTextureFormat::DEPTH24STENCIL8: return true;
      default: return false;
    }
  }

  static GLenum openGLTextureFormat(FramebufferTextureFormat format)
  {
    switch (format)
    {
      case FramebufferTextureFormat::RED_INTEGER: return GL_RED_INTEGER;
      case FramebufferTextureFormat::RGBA8:       return GL_RGBA8;
      default: ENG_CORE_ERROR("Invalid texture format!");  return 0;
    }
  }



  OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& specification)
    : m_RendererID(0),
      m_Specification(specification),
      m_DepthAttachmentSpecification(FramebufferTextureFormat::None),
      m_DepthAttachment(0)
  {
    for (const auto& spec : m_Specification.attachments)
    {
      if (!isDepthFormat(spec.textureFormat))
        m_ColorAttachmentSpecifications.push_back(spec);
      else
        m_DepthAttachmentSpecification = spec;
    }

    invalidate();
  }

  OpenGLFramebuffer::~OpenGLFramebuffer()
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    glDeleteFramebuffers(1, &m_RendererID);
    glDeleteTextures(static_cast<GLsizei>(m_ColorAttachments.size()), m_ColorAttachments.data());
    glDeleteTextures(1, &m_DepthAttachment);
  }

  void OpenGLFramebuffer::bind()
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    glViewport(0, 0, m_Specification.width, m_Specification.height);
  }

  void OpenGLFramebuffer::unbind()
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  const FramebufferSpecification& OpenGLFramebuffer::specification() const
  {
    return m_Specification;
  }

  u32 OpenGLFramebuffer::getColorAttachmentRendererID(u32 index) const
  {
    ENG_CORE_ASSERT(index < m_ColorAttachments.size(), "Color attachment index is out of bounds!");
    return m_ColorAttachments[index];
  }

  void OpenGLFramebuffer::invalidate()
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    if (m_RendererID != 0)
    {
      glDeleteFramebuffers(1, &m_RendererID);
      glDeleteTextures(static_cast<GLsizei>(m_ColorAttachments.size()), m_ColorAttachments.data());
      glDeleteTextures(1, &m_DepthAttachment);

      m_ColorAttachments.clear();
      m_DepthAttachment = 0;
    }

    glCreateFramebuffers(1, &m_RendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

    bool multiSample = m_Specification.samples > 1;

    // Attachments
    if (m_ColorAttachmentSpecifications.size() > 0)
    {
      m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size());
      createTextures(multiSample, m_ColorAttachments.data(), static_cast<u32>(m_ColorAttachments.size()));

      for (i32 i = 0; i < m_ColorAttachments.size(); ++i)
      {
        bindTexture(multiSample, m_ColorAttachments[i]);
        switch (m_ColorAttachmentSpecifications[i].textureFormat)
        {
          case FramebufferTextureFormat::RED_INTEGER:
            attachColorTexture(m_ColorAttachments[i], m_Specification, GL_R32I, GL_RED_INTEGER, i);
            break;
          case FramebufferTextureFormat::RGBA8: 
            attachColorTexture(m_ColorAttachments[i], m_Specification, GL_RGBA8, GL_RGBA, i);
            break;
        }
      }
    }

    if (m_DepthAttachmentSpecification.textureFormat != FramebufferTextureFormat::None)
    {
      createTextures(multiSample, &m_DepthAttachment, 1);
      bindTexture(multiSample, m_DepthAttachment);

      switch (m_DepthAttachmentSpecification.textureFormat)
      {
        case FramebufferTextureFormat::DEPTH24STENCIL8:
          attachDepthTexture(m_DepthAttachment, m_Specification, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT);
          break;
      }
    }

    if (m_ColorAttachments.size() > 1)
    {
      ENG_CORE_ASSERT(m_ColorAttachments.size() <= 4, "Engine only supports up to 4 color attachments");
      static constexpr GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
      glDrawBuffers(static_cast<GLsizei>(m_ColorAttachments.size()), buffers);
    }
    else if (m_ColorAttachments.empty())
      glDrawBuffer(GL_NONE);  // Only depth-pass

    ENG_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void OpenGLFramebuffer::resize(u32 width, u32 height)
  {
    if (width == 0 || width > c_MaxFrameBufferSize || height == 0 || height > c_MaxFrameBufferSize)
    {
      ENG_CORE_WARN("Attempted to resize framebuffer to {0}, {1}", width, height);
      return;
    }

    m_Specification.width = width;
    m_Specification.height = height;
    invalidate();
  }

  i32 OpenGLFramebuffer::readPixel(u32 attachmentIndex, i32 x, i32 y)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    ENG_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "Color attachment index is out of bounds!");
    glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
    i32 pixelData;
    glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
    return pixelData;
  }

  void OpenGLFramebuffer::clearAttachment(u32 attachmentIndex, i32 value)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    ENG_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "Color attachment index is out of bounds!");

    const FramebufferTextureSpecification& spec = m_ColorAttachmentSpecifications[attachmentIndex];
    u32 attachment = m_ColorAttachments[attachmentIndex];
    GLenum textureFormat = openGLTextureFormat(spec.textureFormat);

    glClearTexImage(attachment, 0, textureFormat, GL_INT, &value);
  }

  void OpenGLFramebuffer::clearAttachment(u32 attachmentIndex, f32 value)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    ENG_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "Color attachment index is out of bounds!");

    const FramebufferTextureSpecification& spec = m_ColorAttachmentSpecifications[attachmentIndex];
    u32 attachment = m_ColorAttachments[attachmentIndex];
    GLenum textureFormat = openGLTextureFormat(spec.textureFormat);

    glClearTexImage(attachment, 0, textureFormat, GL_FLOAT, &value);
  }
}