#pragma once
#include "VertexArray.h"

namespace Engine
{
  struct DrawElementsIndirectCommand
  {
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int baseVertex;
    uint32_t baseInstance;
  };

  class MultiDrawArray
  {
  public:
    MultiDrawArray(const BufferLayout& layout);

    void bind() const;
    void unBind() const;

    void add(int ID, const void* data, uint32_t size);
    void remove(int ID);

    uint32_t getSize(int ID);

    bool queue(int ID, uint32_t indexCount = 0);
    void clear();

    int size() const;
    float usage() const;
    float fragmentation() const;

    const std::vector<DrawElementsIndirectCommand>& getQueuedDrawCommands() const;

    /*
      Sets an array of indices that represent the order in which vertices will be drawn.
      The count of this array should be a multiple of 3 (for drawing triangles).
      OpenGL expects vertices to be in counter-clockwise orientation.
    */
    void setIndexBuffer(const std::shared_ptr<const IndexBuffer>& indexBuffer);

  private:
    struct MemoryRegion
    {
      uint32_t offset;
      uint32_t size;
      bool free;
    };

    std::unique_ptr<VertexArray> m_VertexArray;
    std::unordered_map<int, uint32_t> m_AllocatedMemory;
    std::vector<MemoryRegion> m_MemoryPool;
    std::vector<DrawElementsIndirectCommand> m_QueuedDrawCommands;
    uint32_t m_Stride;
  };
}