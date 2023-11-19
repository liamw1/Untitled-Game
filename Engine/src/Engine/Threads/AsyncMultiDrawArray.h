#pragma once
#include "Containers/UnorderedSet.h"
#include "Engine/Renderer/MultiDrawArray.h"

namespace eng::thread
{
  template<DrawCommandType T>
  class AsyncMultiDrawArray
  {
    using Identifier = T::IDType;

    std::mutex m_Mutex;
    UnorderedSet<T> m_CommandQueue;
    MultiDrawArray<T> m_MultiDrawArray;

  public:
    AsyncMultiDrawArray(const BufferLayout& layout)
      : m_MultiDrawArray(layout) {}

    template<std::invocable<eng::MultiDrawArray<T>&> F>
    void drawOperation(F&& operation)
    {
      std::lock_guard lock(m_Mutex);
      operation(m_MultiDrawArray);
    }

    void queueCommand(T&& drawCommand)
    {
      m_CommandQueue.insertOrReplace(std::move(drawCommand));
    }

    void removeCommand(const Identifier& id)
    {
      std::lock_guard lock(m_Mutex);
      m_MultiDrawArray.remove(id);
    }

    void uploadQueuedCommands()
    {
      std::lock_guard lock(m_Mutex);

      std::unordered_set<T> drawCommands = m_CommandQueue.removeAll();
      for (auto it = drawCommands.begin(); it != drawCommands.end();)
      {
        T drawCommand = std::move(drawCommands.extract(it++).value());

        m_MultiDrawArray.remove(drawCommand.id());
        m_MultiDrawArray.add(std::move(drawCommand));
      }
    }
  };
}