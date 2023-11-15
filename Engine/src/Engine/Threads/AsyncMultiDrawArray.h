#pragma once
#include "Containers/UnorderedSet.h"
#include "Engine/Renderer/MultiDrawArray.h"

namespace eng::thread
{
  template<DrawIndexedCommand DrawCommandType>
  class AsyncMultiDrawIndexedArray
  {
    using Identifier = detail::IDType<DrawCommandType>;

    UnorderedSet<DrawCommandType> m_CommandQueue;
    MultiDrawIndexedArray<DrawCommandType> m_MultiDrawArray;

  public:
    AsyncMultiDrawIndexedArray() = default;
    AsyncMultiDrawIndexedArray(const BufferLayout& layout)
      : m_MultiDrawArray(layout) {}

    MultiDrawIndexedArray<DrawCommandType>& multiDrawArray()
    {
      return m_MultiDrawArray;
    }

    void queueCommand(DrawCommandType&& drawCommand)
    {
      m_CommandQueue.insertOrReplace(std::move(drawCommand));
    }

    void removeCommand(const Identifier& id)
    {
      m_CommandQueue.insertOrReplace(DrawCommandType(id));
    }

    template<std::predicate<Identifier> P>
    void uploadQueuedCommandsIf(P&& predicate)
    {
      std::unordered_set<DrawCommandType> drawCommands = m_CommandQueue.removeAll();
      for (auto it = drawCommands.begin(); it != drawCommands.end();)
      {
        DrawCommandType drawCommand = std::move(drawCommands.extract(it++).value());

        m_MultiDrawArray.remove(drawCommand.id());
        if (predicate(drawCommand.id()))
          m_MultiDrawArray.add(std::move(drawCommand));
      }
    }
  };
}