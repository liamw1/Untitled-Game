#pragma once
#include "Containers/UnorderedSet.h"
#include "Engine/Renderer/MultiDrawArray.h"

namespace eng::thread
{
  template<DrawCommandType T>
  class AsyncMultiDrawIndexedArray
  {
    using Identifier = T::IDType;

    UnorderedSet<T> m_CommandQueue;
    MultiDrawArray<T> m_MultiDrawArray;

  public:
    AsyncMultiDrawIndexedArray() = default;
    AsyncMultiDrawIndexedArray(const BufferLayout& layout)
      : m_MultiDrawArray(layout) {}

    MultiDrawArray<T>& multiDrawArray()
    {
      return m_MultiDrawArray;
    }

    void queueCommand(T&& drawCommand)
    {
      m_CommandQueue.insertOrReplace(std::move(drawCommand));
    }

    void removeCommand(const Identifier& id)
    {
      m_CommandQueue.insertOrReplace(T(id));
    }

    template<std::predicate<Identifier> P>
    void uploadQueuedCommandsIf(P&& predicate)
    {
      std::unordered_set<T> drawCommands = m_CommandQueue.removeAll();
      for (auto it = drawCommands.begin(); it != drawCommands.end();)
      {
        T drawCommand = std::move(drawCommands.extract(it++).value());

        m_MultiDrawArray.remove(drawCommand.id());
        if (predicate(drawCommand.id()))
          m_MultiDrawArray.add(std::move(drawCommand));
      }
    }
  };
}