#pragma once
#include "VertexArray.h"
#include "MemoryPool.h"
#include "Engine/Core/Algorithm.h"
#include "Engine/Utilities/Constraints.h"

namespace eng
{
  /*
    A CRTP class that represents a single multi-draw command.
    Derived classes can store arbitrary amounts of extra data
    as long as the layout of the first 16 bytes is preserved.
  */
  template<Hashable Identifier, typename Derived>
  class MultiDrawCommand : private NonCopyable
  {
  protected:
    u32 m_VertexCount;
    u32 m_InstanceCount;
    u32 m_FirstVertex;
    u32 m_BaseInstance;

    Identifier m_ID;
    std::shared_ptr<size_t> m_CommandIndex;

  public:
    MultiDrawCommand(const Identifier& id, i32 vertexCount)
      : m_VertexCount(vertexCount),
        m_InstanceCount(1),
        m_FirstVertex(0),
        m_BaseInstance(0),
        m_ID(id),
        m_CommandIndex(nullptr) {}

    i32 vertexCount() const { return m_VertexCount; }
    i32 firstVertex() const { return m_FirstVertex; }

    const Identifier& id() const { return m_ID; }
    const std::shared_ptr<size_t>& commandIndex() const { return m_CommandIndex; }

    void setPlacement(i32 firstVertex, size_t commandIndex)
    {
      m_FirstVertex = firstVertex;
      m_CommandIndex = std::make_shared<size_t>(commandIndex);
    }

    const void* vertexData() const { return static_cast<Derived*>(this)->vertexData(); }
    void prune() { return static_cast<Derived*>(this)->prune(); }
  };

  /*
    A CRTP class that represents a single indexed multi-draw command.
    Derived classes can store arbitrary amounts of extra data as long
    as the layout of the first 20 bytes is preserved.
  */
  template<Hashable Identifier, typename Derived>
  class MultiDrawIndexedCommand : private NonCopyable
  {
  protected:
    u32 m_IndexCount;
    u32 m_InstanceCount;
    u32 m_FirstIndex;
    i32 m_BaseVertex;
    u32 m_BaseInstance;

    Identifier m_ID;
    std::shared_ptr<size_t> m_CommandIndex;

  public:
    MultiDrawIndexedCommand(const Identifier& id, u32 indexCount)
      : m_IndexCount(indexCount),
        m_InstanceCount(1),
        m_FirstIndex(0),
        m_BaseVertex(0),
        m_BaseInstance(0),
        m_ID(id),
        m_CommandIndex(nullptr) {}

    u32 indexCount() const { return m_IndexCount; }
    u32 firstIndex() const { return m_FirstIndex; }
    i32 baseVertex() const { return m_BaseVertex; }

    const Identifier& id() const { return m_ID; }
    const std::shared_ptr<size_t>& commandIndex() const { return m_CommandIndex; }

    void setPlacement(u32 firstIndex, i32 baseVertex, size_t commandIndex)
    {
      m_FirstIndex = firstIndex;
      m_BaseVertex = baseVertex;
      m_CommandIndex = std::make_shared<size_t>(commandIndex);
    }

    i32 vertexCount() const { return static_cast<Derived*>(this)->vertexCount(); }
    const void* indexData() { return static_cast<Derived*>(this)->indexData(); }
    const void* vertexData() { return static_cast<Derived*>(this)->vertexData(); }
    void prune() { return static_cast<Derived*>(this)->prune(); }
  };

  namespace detail
  {
    template<typename T>
    using IDType = std::decay_t<decltype(std::declval<T>().id())>;
  }

  template<typename T>
  concept DrawCommand = std::derived_from<T, MultiDrawCommand<detail::IDType<T>, T>>;

  template<typename T>
  concept DrawIndexedCommand = std::derived_from<T, MultiDrawIndexedCommand<detail::IDType<T>, T>>;



  /*
    Stores and manages a set of multi-draw commands. Commands are queried using their unique ID,
    the type of which is user-specified. Provides fast insertion and removal, while keeping
    draw commands tightly packed. Vertex data is stored in a single dynamically-resizing buffer
    on the gpu. Additionally, the class provides operations on draw commands, such as sorting.
  */
  template<DrawCommand DrawCommandType>
  class MultiDrawArray
  {
  public:
    using Identifier = detail::IDType<DrawCommandType>;

  private:
    i32 m_Stride;
    MemoryPool m_MemoryPool;
    std::unique_ptr<VertexArray> m_VertexArray;
    std::vector<DrawCommandType> m_DrawCommands;
    std::unordered_map<Identifier, std::shared_ptr<size_t>> m_DrawCommandIndices;

  public:
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

    void add(DrawCommandType&& drawCommand)
    {
      i32 vertexCount = drawCommand.vertexCount();
      if (vertexCount == 0)
        return;

      ENG_CORE_ASSERT(m_DrawCommandIndices.find(drawCommand.id()) == m_DrawCommandIndices.end(), "Draw command with ID {0} has already been allocated!", drawCommand.id());

      // Add draw command data to memory pool. If a resize is triggered, vertex array needs to have layout set again.
      auto [triggeredResize, allocationAddress] = m_MemoryPool.add(drawCommand.vertexData(), vertexCount * m_Stride);
      if (triggeredResize)
        m_VertexArray->setLayout(m_VertexArray->getLayout());
      drawCommand.prune();

      i32 firstVertex = static_cast<i32>(allocationAddress / m_Stride);
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

    template<Predicate<Identifier> P>
    i32 partition(P&& predicate)
    {
      DrawCommandIterator partitionEnd = partitionContainer(m_DrawCommands, [&predicate](const DrawCommandType& draw) { return predicate(draw.id()); });
      setDrawCommandIndices(0, m_DrawCommands.size());
      return static_cast<i32>(partitionEnd - m_DrawCommands.begin());
    }

    template<TransformToComarable<Identifier> F>
    void sort(i32 drawCount, F&& transform, SortPolicy sortPolicy)
    {
      algo::sort(m_DrawCommands.begin(), m_DrawCommands.begin() + drawCount, [&transform](const DrawCommandType& draw) { return transform(draw.id()); }, sortPolicy);
      setDrawCommandIndices(0, drawCount);
    }

    template<BinaryComparison F>
    void sort(i32 drawCount, F&& comparison)
    {
      std::sort(m_DrawCommands.begin(), m_DrawCommands.begin() + drawCount,
                [&comparison](const DrawCommandType& drawA, const DrawCommandType& drawB) { return comparison(drawA.id(), drawB.id()); });
      setDrawCommandIndices(0, drawCount);
    }

    template<InvocableWithReturnType<bool, DrawCommandType&> F>
    void amend(i32 drawCount, F&& function)
    {
      std::for_each_n(m_DrawCommands.begin(), drawCount, [this, &function](DrawCommandType& draw)
      {
        u32 oldVertexCount = draw.vertexCount();
        if (!function(draw))
          return;

        if (draw.vertexCount() > oldVertexCount)
        {
          ENG_CORE_ERROR("Ammended draw command added additional vertices! Discarding changes...");
          return;
        }

        m_MemoryPool.amend(draw.vertexData(), getDrawCommandAddress(draw));
      });
    }

    const std::vector<DrawCommandType>& getDrawCommandBuffer() const { return m_DrawCommands; }

  private:
    using DrawCommandIterator = std::vector<DrawCommandType>::iterator;
    using DrawCommandIndicesIterator = std::unordered_map<Identifier, std::shared_ptr<size_t>>::iterator;

    MemoryPool::address_t getDrawCommandAddress(const DrawCommandType& drawCommand)
    {
      return drawCommand.firstVertex() * m_Stride;
    }

    void setDrawCommandIndices(size_t begin, size_t end)
    {
      for (size_t i = begin; i < end; ++i)
        *m_DrawCommands[i].commandIndex() = i;
    }
  };

  /*
    Stores and manages a set of multi-draw commands. Commands are queried using their unique ID,
    the type of which is user-specified. Provides fast insertion and removal, while keeping
    draw commands tightly packed. Vertex data is stored in a single dynamically-resizing buffer
    on the gpu. Additionally, the class provides operations on draw commands, such as sorting.
  */
  template<DrawIndexedCommand DrawCommandType>
  class MultiDrawIndexedArray
  {
  public:
    using Identifier = detail::IDType<DrawCommandType>;

  private:
    i32 m_Stride;
    MemoryPool m_IndexMemory;
    MemoryPool m_VertexMemory;
    std::unique_ptr<VertexArray> m_VertexArray;
    std::vector<DrawCommandType> m_DrawCommands;
    std::unordered_map<Identifier, std::shared_ptr<size_t>> m_DrawCommandIndices;

  public:
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

    void add(DrawCommandType&& drawCommand)
    {
      u32 indexCount = drawCommand.indexCount();
      i32 vertexCount = drawCommand.vertexCount();
      if (vertexCount == 0 || indexCount == 0)
        return;

      ENG_CORE_ASSERT(m_DrawCommandIndices.find(drawCommand.id()) == m_DrawCommandIndices.end(), "Draw command with ID {0} has already been allocated!", drawCommand.id());

      // Add draw command data to memory pools. If a vertex buffer resize is triggered, vertex array needs to have layout set again.
      auto [indexBufferResized, indexAllocationAddress] = m_IndexMemory.add(drawCommand.indexData(), indexCount * sizeof(u32));
      auto [vertexBufferResized, vertexAllocationAddress] = m_VertexMemory.add(drawCommand.vertexData(), vertexCount * m_Stride);
      if (vertexBufferResized)
        m_VertexArray->setLayout(m_VertexArray->getLayout());
      drawCommand.prune();

      u32 firstIndex = static_cast<u32>(indexAllocationAddress / sizeof(u32));
      i32 baseVertex = static_cast<i32>(vertexAllocationAddress / m_Stride);
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

    template<Predicate<Identifier> P>
    i32 partition(P&& predicate)
    {
      DrawCommandIterator partitionEnd = algo::partition(m_DrawCommands, [&predicate](const DrawCommandType& draw) { return predicate(draw.id()); });
      setDrawCommandIndices(0, m_DrawCommands.size());
      return static_cast<i32>(partitionEnd - m_DrawCommands.begin());
    }

    template<TransformToComarable<Identifier> F>
    void sort(i32 drawCount, F&& transform, SortPolicy sortPolicy)
    {
      algo::sort(m_DrawCommands.begin(), m_DrawCommands.begin() + drawCount, [&transform](const DrawCommandType& draw) { return transform(draw.id()); }, sortPolicy);
      setDrawCommandIndices(0, drawCount);
    }

    template<BinaryComparison<Identifier> F>
    void sort(i32 drawCount, F&& comparison)
    {
      std::sort(m_DrawCommands.begin(), m_DrawCommands.begin() + drawCount,
                [&comparison](const DrawCommandType& drawA, const DrawCommandType& drawB) { return comparison(drawA.id(), drawB.id()); });
      setDrawCommandIndices(0, drawCount);
    }

    template<InvocableWithReturnType<bool, DrawCommandType&> F>
    void amend(i32 drawCount, F&& function)
    {
      std::for_each_n(m_DrawCommands.begin(), drawCount, [this, &function](DrawCommandType& draw)
      {
        u32 oldIndexCount = draw.indexCount();
        if (!function(draw))
          return;

        if (draw.indexCount() > oldIndexCount)
        {
          ENG_CORE_ERROR("Ammended draw command added additional indices! Discarding changes...");
          return;
        }

        m_IndexMemory.amend(draw.indexData(), getDrawCommandIndicesAddress(draw));
      });
    }

    const std::vector<DrawCommandType>& getDrawCommandBuffer() const { return m_DrawCommands; }

  private:
    using DrawCommandIterator = std::vector<DrawCommandType>::iterator;
    using DrawCommandIndicesIterator = std::unordered_map<Identifier, std::shared_ptr<size_t>>::iterator;

    MemoryPool::address_t getDrawCommandIndicesAddress(const DrawCommandType& drawCommand)
    {
      return drawCommand.firstIndex() * sizeof(u32);
    }

    MemoryPool::address_t getDrawCommandVerticesAddress(const DrawCommandType& drawCommand)
    {
      return drawCommand.baseVertex() * m_Stride;
    }

    void setDrawCommandIndices(size_t begin, size_t end)
    {
      for (size_t i = begin; i < end; ++i)
        *m_DrawCommands[i].commandIndex() = i;
    }
  };
}