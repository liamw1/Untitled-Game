#pragma once
#include "NewLOD.h"
#include "LODHelpers.h"

namespace newLod
{
  class LODManager
  {
    std::shared_ptr<eng::thread::AsyncMultiDrawArray<DrawCommand>> m_MultiDrawArray;

    // Multi-threading
    std::shared_ptr<eng::thread::ThreadPool> m_ThreadPool;
    eng::thread::WorkSet<GlobalIndex, void> m_MeshingWork;

    // LOD data
    Octree m_LODs;

  public:
    LODManager();

    void render();

  private:
    void meshLOD(const NodeID& nodeID);
  };
}