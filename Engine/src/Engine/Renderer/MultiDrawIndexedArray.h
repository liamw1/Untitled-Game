#pragma once
#include "VertexArray.h"
#include "MemoryPool.h"

namespace Engine
{
  /*
    A CRTP class that represents a single multi-draw command.
    Derived classes can store arbitrary amounts of extra data
    as long as the layout of the first 20 bytes is preserved.
  */
  template<typename Identifier, typename Derived>
  class MultiDrawIndexedCommand
  {
  public:
    MultiDrawIndexedCommand(const Identifier& id, uint32_t indexCount)
      : m_IndexCount(indexCount),
        m_InstanceCount(1),
        m_FirstIndex(0),
        m_BaseVertex(0),
        m_BaseInstance(0),
        m_ID(id),
        m_CommandIndex(nullptr) {}

    uint32_t indexCount() const { return m_IndexCount; }
    uint32_t firstIndex() const { return m_FirstIndex; }
    int baseVertex() const { return m_BaseVertex; }

    const Identifier& id() const { return m_ID; }
    const std::shared_ptr<size_t>& commandIndex() const { return m_CommandIndex; }

    void setPlacement(uint32_t firstIndex, int baseVertex, size_t commandIndex)
    {
      m_FirstIndex = firstIndex;
      m_BaseVertex = baseVertex;
      m_CommandIndex = std::make_shared<size_t>(commandIndex);
    }

    int vertexCount() const { return static_cast<Derived*>(this)->vertexCount(); }
    const void* indexData() { return static_cast<Derived*>(this)->indexData(); }
    const void* vertexData() { return static_cast<Derived*>(this)->vertexData(); }
    void prune() { return static_cast<Derived*>(this)->prune(); }

  protected:
    uint32_t m_IndexCount;
    uint32_t m_InstanceCount;
    uint32_t m_FirstIndex;
    int m_BaseVertex;
    uint32_t m_BaseInstance;

    Identifier m_ID;
    std::shared_ptr<size_t> m_CommandIndex;
  };

  template<typename DrawCommand>
  class MultiDrawIndexedArray
  {
  public:
    using Identifier = std::decay_t<decltype(std::declval<DrawCommand>().id())>;

    MultiDrawIndexedArray(const BufferLayout& layout)
      : m_Stride(layout.stride()),
        m_IndexMemory(StorageBuffer::Type::IndexBuffer),
        m_VertexMemory(StorageBuffer::Type::VertexBuffer)
    {
      m_VertexArray = VertexArray::Create();
      m_VertexArray->setIndexBuffer(m_IndexMemory.buffer());
      m_VertexArray->setVertexBuffer(m_VertexMemory.buffer());
      m_VertexArray->setLayout(layout);
    }

    void bind() const { m_VertexArray->bind(); }
    void unBind() const { m_VertexArray->unBind(); }

    void add(DrawCommand&& drawCommand)
    {
      uint32_t indexCount = drawCommand.indexCount();
      int vertexCount = drawCommand.vertexCount();
      if (vertexCount == 0 || indexCount == 0)
        return;

      EN_CORE_ASSERT(m_DrawCommandIndices.find(drawCommand.id()) == m_DrawCommandIndices.end(), "Draw command with ID {0} has already been allocated!", drawCommand.id());

      auto [indexBufferResized, indexAllocationAddress] = m_IndexMemory.add(drawCommand.indexData(), indexCount * sizeof(uint32_t));
      auto [vertexBufferResized, vertexAllocationAddress] = m_VertexMemory.add(drawCommand.vertexData(), vertexCount * m_Stride);
      if (vertexBufferResized)
        m_VertexArray->setLayout(m_VertexArray->getLayout());
      drawCommand.prune();

      uint32_t firstIndex = static_cast<uint32_t>(indexAllocationAddress / sizeof(uint32_t));
      int baseVertex = static_cast<int>(vertexAllocationAddress / m_Stride);
      drawCommand.setPlacement(firstIndex, baseVertex, m_DrawCommands.size());

      m_DrawCommandIndices.emplace(drawCommand.id(), drawCommand.commandIndex());
      m_DrawCommands.push_back(std::move(drawCommand));
    }

    void remove(const Identifier& id)
    {
      DrawCommandIndicesIterator drawCommandToRemove = m_DrawCommandIndices.find(id);
      if (drawCommandToRemove == m_DrawCommandIndices.end())
        return;

      size_t drawCommandIndex = *drawCommandToRemove->second;
      m_IndexMemory.remove(getDrawCommandIndicesAddress(m_DrawCommands[drawCommandIndex]));
      m_VertexMemory.remove(getDrawCommandVerticesAddress(m_DrawCommands[drawCommandIndex]));
      m_DrawCommandIndices.erase(drawCommandToRemove);

      // Update draw command container
      std::swap(m_DrawCommands[drawCommandIndex], m_DrawCommands.back());
      *m_DrawCommands[drawCommandIndex].commandIndex() = drawCommandIndex;
      m_DrawCommands.pop_back();
    }

    template<typename F, typename... Args>
    int mask(F condition, Args&&... args)
    {
      int leftIndex = 0;
      int rightIndex = static_cast<int>(m_DrawCommands.size() - 1);

      while (leftIndex < rightIndex)
      {
        while (condition(m_DrawCommands[leftIndex].id(), std::forward<Args>(args)...) && leftIndex < rightIndex)
          leftIndex++;
        while (!condition(m_DrawCommands[rightIndex].id(), std::forward<Args>(args)...) && leftIndex < rightIndex)
          rightIndex--;

        if (leftIndex != rightIndex)
        {
          std::swap(m_DrawCommands[leftIndex], m_DrawCommands[rightIndex]);
          std::swap(*m_DrawCommands[leftIndex].commandIndex(), *m_DrawCommands[rightIndex].commandIndex());
        }
      }
      return leftIndex;
    }

    template<typename F, typename... Args>
    void sort(int drawCount, F comparision, Args&&... args)
    {
      std::sort(m_DrawCommands.begin(), m_DrawCommands.begin() + drawCount, [&](const DrawCommand& drawA, const DrawCommand& drawB)
        {
          return comparision(drawA.id(), drawB.id(), std::forward<Args>(args)...);
        });

      for (int i = 0; i < drawCount; ++i)
        *m_DrawCommands[i].commandIndex() = i;
    }

    template<typename F, typename... Args>
    void amend(int drawCount, F function, Args&&... args)
    {
      for (auto it = m_DrawCommands.begin(); it != m_DrawCommands.begin() + drawCount; ++it)
      {
        uint32_t oldIndexCount = it->indexCount();
        if (function(*it, std::forward<Args>(args)...))
        {
          if (it->indexCount() > oldIndexCount)
          {
            EN_CORE_ERROR("Ammended draw command added additional vertices! Discarding changes...");
            continue;
          }
          m_IndexMemory.amend(it->indexData(), getDrawCommandIndicesAddress(*it));
        }
      }
    }

    const std::vector<DrawCommand>& getDrawCommandBuffer() const { return m_DrawCommands; }

  private:
    using DrawCommandIndicesIterator = std::unordered_map<Identifier, std::shared_ptr<size_t>>::iterator;

    int m_Stride;
    MemoryPool m_IndexMemory;
    MemoryPool m_VertexMemory;
    std::unique_ptr<VertexArray> m_VertexArray;
    std::vector<DrawCommand> m_DrawCommands;
    std::unordered_map<Identifier, std::shared_ptr<size_t>> m_DrawCommandIndices;

    size_t getDrawCommandIndicesAddress(const DrawCommand& drawCommand)
    {
      return drawCommand.firstIndex() * sizeof(uint32_t);
    }

    size_t getDrawCommandVerticesAddress(const DrawCommand& drawCommand)
    {
      return drawCommand.baseVertex() * m_Stride;
    }
  };
}
