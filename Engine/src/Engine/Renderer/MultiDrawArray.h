#pragma once
#include "VertexArray.h"
#include "MemoryPool.h"

namespace Engine
{
  /*
    A CRTP class that represents a single multi-draw command.
    Derived classes can store arbitrary amounts of extra data
    as long as the layout of the first 16 bytes is preserved.
  */
  template<typename Identifier, typename Derived>
  class MultiDrawCommand
  {
  public:
    MultiDrawCommand(const Identifier& id, int vertexCount)
      : m_VertexCount(vertexCount),
        m_InstanceCount(1),
        m_FirstVertex(0),
        m_BaseInstance(0),
        m_ID(id),
        m_CommandIndex(nullptr) {}

    int vertexCount() const { return m_VertexCount; }
    int firstVertex() const { return m_FirstVertex; }

    const Identifier& id() const { return m_ID; }
    const std::shared_ptr<size_t>& commandIndex() const { return m_CommandIndex; }

    void setPlacement(int firstVertex, size_t commandIndex)
    {
      m_FirstVertex = firstVertex;
      m_CommandIndex = std::make_shared<size_t>(commandIndex);
    }

    const void* vertexData() const { return static_cast<Derived*>(this)->vertexData(); }
    void prune() { return static_cast<Derived*>(this)->prune(); }

  protected:
    uint32_t m_VertexCount;
    uint32_t m_InstanceCount;
    uint32_t m_FirstVertex;
    uint32_t m_BaseInstance;

    Identifier m_ID;
    std::shared_ptr<size_t> m_CommandIndex;
  };

  /*
    Stores and manages a set of multi-draw commands. Commands are queried using their unique ID,
    the type of which is user-specified. Provides fast insertion and removal, while keeping
    draw commands tightly packed. Vertex data is stored in a single dynamically-resizing buffer
    on the gpu. Additionally, the class provides operations on draw commands, such as sorting.
  */
  template<typename DrawCommand>
  class MultiDrawArray
  {
  public:
    using Identifier = std::decay_t<decltype(std::declval<DrawCommand>().id())>;

    MultiDrawArray(const BufferLayout& layout)
      : m_Stride(layout.stride()),
        m_MemoryPool(StorageBuffer::Type::VertexBuffer)
    {
      m_VertexArray = VertexArray::Create();
      m_VertexArray->setVertexBuffer(m_MemoryPool.buffer());
      m_VertexArray->setLayout(layout);
    }

    void bind() const { m_VertexArray->bind(); }
    void unBind() const { m_VertexArray->unBind(); }

    void add(DrawCommand&& drawCommand)
    {
      int vertexCount = drawCommand.vertexCount();
      if (vertexCount == 0)
        return;

      EN_CORE_ASSERT(m_DrawCommandIndices.find(drawCommand.id()) == m_DrawCommandIndices.end(), "Draw command with ID {0} has already been allocated!", drawCommand.id());

      auto [triggeredResize, allocationAddress] = m_MemoryPool.add(drawCommand.vertexData(), vertexCount * m_Stride);
      if (triggeredResize)
        m_VertexArray->setLayout(m_VertexArray->getLayout());
      drawCommand.prune();

      int firstVertex = static_cast<int>(allocationAddress / m_Stride);
      drawCommand.setPlacement(firstVertex, m_DrawCommands.size());

      m_DrawCommandIndices.emplace(drawCommand.id(), drawCommand.commandIndex());
      m_DrawCommands.push_back(std::move(drawCommand));
    }

    void remove(const Identifier& id)
    {
      DrawCommandIndicesIterator drawCommandToRemove = m_DrawCommandIndices.find(id);
      if (drawCommandToRemove == m_DrawCommandIndices.end())
        return;

      size_t drawCommandIndex = *drawCommandToRemove->second;
      m_MemoryPool.remove(getDrawCommandAddress(m_DrawCommands[drawCommandIndex]));
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
        int oldVertexCount = it->vertexCount();
        if (function(*it, std::forward<Args>(args)...))
        {
          if (it->vertexCount() > oldVertexCount)
          {
            EN_CORE_ERROR("Ammended draw command added additional vertices! Discarding changes...");
            continue;
          }
          m_MemoryPool.amend(it->vertexData(), getDrawCommandAddress(*it));
        }
      }
    }

    const std::vector<DrawCommand>& getDrawCommandBuffer() const { return m_DrawCommands; }

  private:
    using DrawCommandIndicesIterator = std::unordered_map<Identifier, std::shared_ptr<size_t>>::iterator;

    int m_Stride;
    MemoryPool m_MemoryPool;
    std::unique_ptr<VertexArray> m_VertexArray;
    std::vector<DrawCommand> m_DrawCommands;
    std::unordered_map<Identifier, std::shared_ptr<size_t>> m_DrawCommandIndices;

    size_t getDrawCommandAddress(const DrawCommand& drawCommand)
    {
      return drawCommand.firstVertex() * m_Stride;
    }
  };
}