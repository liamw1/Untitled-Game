#pragma once
#include "VertexArray.h"
#include "MemoryPool.h"
#include "Engine/Core/Algorithm.h"
#include "Engine/Core/Log/Log.h"
#include "Engine/Utilities/Constraints.h"

namespace eng
{
  namespace detail
  {
    struct DrawCommandData
    {
      u32 vertexCount = 0;
      u32 instanceCount = 1;
      u32 firstVertex = 0;
      u32 baseInstance = 0;
    };
    struct IndexedDrawCommandData
    {
      u32 indexCount = 0;
      u32 instanceCount = 1;
      u32 firstIndex = 0;
      i32 baseVertex = 0;
      u32 baseInstance = 0;
    };
  }

  /*
    A CRTP class that represents a single multi-draw command.
    Derived classes must provided vertexData() and clearData() functions.
    Derived classes of the indexed variant must also provide vertexCount()
    and indexData() functions.
  */
  template<typename Derived, Hashable Identifier, bool IsIndexed>
  class GenericDrawCommand : private NonCopyable
  {
    using CommandData = std::conditional_t<IsIndexed, detail::IndexedDrawCommandData, detail::DrawCommandData>;

  protected:
    CommandData m_CommandData;
    Identifier m_ID;
    std::shared_ptr<uSize> m_CommandIndex;

  public:
    using IDType = Identifier;

    GenericDrawCommand(const Identifier& id)
      : m_ID(id) {}

    u32 elementCount() const
    {
      if constexpr (IsIndexed)
        return m_CommandData.indexCount;
      else
        return m_CommandData.vertexCount;
    }
    u32 firstElement() const
    {
      if constexpr (IsIndexed)
        return m_CommandData.firstIndex;
      else
        return m_CommandData.firstVertex;
    }

    u32 vertexCount() const
    {
      if constexpr (IsIndexed)
        return static_cast<const Derived*>(this)->vertexCount();
      else
        return m_CommandData.vertexCount;
    }
    i32 baseVertex() const
    {
      if constexpr (IsIndexed)
        return m_CommandData.baseVertex;
      else
        return 0;
    }

    const Identifier& id() const { return m_ID; }
    const std::shared_ptr<uSize>& commandIndex() const { return m_CommandIndex; }
    void setPlacement(u32 firstElement, i32 baseVertex, uSize commandIndex)
    {
      m_CommandIndex = std::make_shared<uSize>(commandIndex);
      if constexpr (IsIndexed)
      {
        m_CommandData.firstIndex = firstElement;
        m_CommandData.baseVertex = baseVertex;
      }
      else
        m_CommandData.firstVertex = firstElement;
    }

    const void* indexData() const
    {
      if constexpr (IsIndexed)
        return static_cast<const Derived*>(this)->indexData();
      else
        return nullptr;
    }
    const void* vertexData() const { return static_cast<const Derived*>(this)->vertexData(); }
    void clearData() { static_cast<Derived*>(this)->clearData(); }

    static constexpr bool Indexed() { return IsIndexed; }
  };

  template<typename Derived, Hashable Identifier>
  using DrawCommand = GenericDrawCommand<Derived, Identifier, false>;

  template<typename Derived, Hashable Identifier>
  using IndexedDrawCommand = GenericDrawCommand<Derived, Identifier, true>;

  template<typename T>
  concept DrawCommandType = std::derived_from<T, GenericDrawCommand<T, typename T::IDType, T::Indexed()>>;



  /*
    Stores and manages a set of multi-draw commands. Commands are queried using their unique ID,
    the type of which is user-specified. Provides fast insertion and removal, while keeping
    draw commands tightly packed. Vertex data is stored in a single dynamically-resizing buffer
    on the gpu. Additionally, the class provides operations on draw commands, such as sorting.
  */
  template<DrawCommandType T>
  class MultiDrawArray
  {
    using Identifier = T::IDType;
    using DrawCommandBaseType = GenericDrawCommand<T, Identifier, T::Indexed()>;
    static constexpr bool c_IsIndexed = T::Indexed();

    i32 m_Stride;
    MemoryPool m_IndexMemory;
    MemoryPool m_VertexMemory;
    std::unique_ptr<VertexArray> m_VertexArray;
    std::vector<T> m_DrawCommands;
    std::unordered_map<Identifier, std::shared_ptr<uSize>> m_DrawCommandIndices;

  public:
    MultiDrawArray(const BufferLayout& layout)
      : m_Stride(layout.stride()),
        m_IndexMemory(StorageBuffer::Type::IndexBuffer),
        m_VertexMemory(StorageBuffer::Type::VertexBuffer)
    {
      m_VertexArray = VertexArray::Create();
      m_VertexArray->setVertexBuffer(m_VertexMemory.buffer());
      m_VertexArray->setLayout(layout);

      if (c_IsIndexed)
        m_VertexArray->setIndexBuffer(m_IndexMemory.buffer());
    }

    void bind() const { m_VertexArray->bind(); }
    void unBind() const { m_VertexArray->unBind(); }

    void add(T&& drawCommand)
    {
      DrawCommandBaseType& baseCommand = drawCommand;

      u32 elementCount = baseCommand.elementCount();
      i32 vertexCount = baseCommand.vertexCount();
      if (vertexCount == 0 || elementCount == 0)
        return;

      ENG_CORE_ASSERT(m_DrawCommandIndices.find(baseCommand.id()) == m_DrawCommandIndices.end(), "Draw command with ID {0} has already been allocated!", baseCommand.id());

      // Add draw command data to memory pools. If a vertex buffer resize is triggered, vertex array needs to have layout set again.
      auto [indexBufferResized, indexAllocationAddress] = m_IndexMemory.add(baseCommand.indexData(), elementCount * sizeof(u32));
      auto [vertexBufferResized, vertexAllocationAddress] = m_VertexMemory.add(baseCommand.vertexData(), vertexCount * m_Stride);
      if (vertexBufferResized)
        m_VertexArray->setLayout(m_VertexArray->getLayout());
      baseCommand.clearData();

      if constexpr (c_IsIndexed)
      {
        u32 firstIndex = arithmeticCast<u32>(indexAllocationAddress / sizeof(u32));
        i32 baseVertex = arithmeticCast<i32>(vertexAllocationAddress / m_Stride);
        baseCommand.setPlacement(firstIndex, baseVertex, m_DrawCommands.size());
      }
      else
      {
        u32 firstVertex = arithmetic_cast<u32>(vertexAllocationAddress / m_Stride);
        baseCommand.setPlacement(firstVertex, 0, m_DrawCommands.size());
      }

      m_DrawCommandIndices.emplace(baseCommand.id(), baseCommand.commandIndex());
      m_DrawCommands.push_back(std::move(drawCommand));
    }

    void remove(const Identifier& id)
    {
      DrawCommandIndicesIterator drawCommandToRemove = m_DrawCommandIndices.find(id);
      if (drawCommandToRemove == m_DrawCommandIndices.end())
        return;

      uSize drawCommandIndex = *drawCommandToRemove->second;
      if constexpr (c_IsIndexed)
        m_IndexMemory.remove(getDrawCommandIndicesAddress(m_DrawCommands[drawCommandIndex]));
      m_VertexMemory.remove(getDrawCommandVerticesAddress(m_DrawCommands[drawCommandIndex]));
      m_DrawCommandIndices.erase(drawCommandToRemove);

      // Update draw command container
      std::swap(m_DrawCommands[drawCommandIndex], m_DrawCommands.back());
      *m_DrawCommands[drawCommandIndex].commandIndex() = drawCommandIndex;
      m_DrawCommands.pop_back();
    }

    template<std::predicate<Identifier> P>
    i32 partition(P&& predicate)
    {
      DrawCommandIterator partitionEnd = algo::partition(m_DrawCommands, [&predicate](const T& draw) { return predicate(draw.id()); });
      setDrawCommandIndices(0, m_DrawCommands.size());
      return arithmeticCast<i32>(partitionEnd - m_DrawCommands.begin());
    }

    template<TransformToComparable<Identifier> F>
    void sort(i32 drawCount, F&& transform, SortPolicy sortPolicy)
    {
      algo::sort(m_DrawCommands.begin(), m_DrawCommands.begin() + drawCount, [&transform](const T& draw) { return transform(draw.id()); }, sortPolicy);
      setDrawCommandIndices(0, drawCount);
    }

    template<BinaryRelation<Identifier> F>
    void sort(i32 drawCount, F&& comparison)
    {
      std::sort(m_DrawCommands.begin(), m_DrawCommands.begin() + drawCount,
                [&comparison](const T& drawA, const T& drawB) { return comparison(drawA.id(), drawB.id()); });
      setDrawCommandIndices(0, drawCount);
    }

    template<InvocableWithReturnType<bool, T&> F>
    void amend(i32 drawCount, F&& function)
    {
      std::for_each_n(m_DrawCommands.begin(), drawCount, [this, &function](T& drawCommand)
      {
        DrawCommandBaseType& baseCommand = drawCommand;

        u32 oldElementCount = baseCommand.elementCount();
        if (!function(drawCommand))
          return;

        if (baseCommand.elementCount() > oldElementCount)
        {
          ENG_CORE_ERROR("Ammended draw command added additional elements! Discarding changes...");
          return;
        }

        if constexpr (c_IsIndexed)
          m_IndexMemory.amend(baseCommand.indexData(), getDrawCommandIndicesAddress(baseCommand));
        else
          m_VertexMemory.amend(baseCommand.vertexData(), getDrawCommandVerticesAddress(baseCommand));
      });
    }

    const std::vector<T>& getDrawCommandBuffer() const { return m_DrawCommands; }

  private:
    using DrawCommandIterator = std::vector<T>::iterator;
    using DrawCommandIndicesIterator = std::unordered_map<Identifier, std::shared_ptr<uSize>>::iterator;

    MemoryPool::address_t getDrawCommandIndicesAddress(const DrawCommandBaseType& baseCommand)
    {
      if constexpr (c_IsIndexed)
        return baseCommand.firstElement() * sizeof(u32);
      else
        throw std::runtime_error("Vertex-based commands do not have an index address!");
    }

    MemoryPool::address_t getDrawCommandVerticesAddress(const DrawCommandBaseType& baseCommand)
    {
      if constexpr (c_IsIndexed)
        return baseCommand.baseVertex() * m_Stride;
      else
        return baseCommand.firstElement() * m_Stride;
    }

    void setDrawCommandIndices(uSize begin, uSize end)
    {
      for (uSize i = begin; i < end; ++i)
        *m_DrawCommands[i].DrawCommandBaseType::commandIndex() = i;
    }
  };
}



// Specialize std::hash for draw command types
namespace std
{
  template<eng::DrawCommandType T>
  struct hash<T>
  {
    uSize operator()(const T& drawCommand) const
    {
      return std::hash<T::IDType>()(drawCommand.id());
    }
  };
}