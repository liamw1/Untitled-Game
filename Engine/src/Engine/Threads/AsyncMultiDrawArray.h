#pragma once
#include "Containers/UnorderedSet.h"
#include "Engine/Renderer/MultiDrawArray.h"

// namespace eng::thread
// {
//   template<DrawIndexedCommand DrawCommandType>
//   class AsyncMultiDrawIndexedArray
//   {
//     using Identifier = detail::IDType<DrawCommandType>;
// 
//     UnorderedSet<DrawCommandType> m_CommandQueue;
//     MultiDrawIndexedArray<DrawCommandType> m_MultiDrawArray;
// 
//   public:
//     AsyncMultiDrawIndexedArray() = default;
//     AsyncMultiDrawIndexedArray(const BufferLayout& layout)
//       : m_MultiDrawArray(layout) {}
// 
//     MultiDrawIndexedArray<DrawCommandType>& multiDrawArray()
//     {
//       return m_MultiDrawArray;
//     }
// 
//     void add(DrawCommandType&& drawCommand)
//     {
//       m_CommandQueue.insertOrReplace(std::move(drawCommand));
//     }
// 
//     void remove(const Identifier& id)
//     {
//       m_CommandQueue.insertOrReplace(DrawCommandType(id));
//     }
// 
//     void update()
//     {
//       std::unordered_set<DrawCommandType> drawCommands = m_CommandQueue.removeAll();
//       for (auto it = drawCommands.begin(); it != drawCommands.end(); ++it)
//       {
//         DrawCommandType drawCommand = std::move(drawCommands.extract(it).value());
// 
// 
//       }
//     }
//   };
// }