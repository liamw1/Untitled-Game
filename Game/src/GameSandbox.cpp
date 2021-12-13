#include "GMpch.h"
#include "GameSandbox.h"
#include "World/ChunkRenderer.h"
#include "World/World.h"
#include "World/LOD.h"

GameSandbox::GameSandbox()
  : Layer("GameSandbox")
{
  Block::Initialize();
  Player::Initialize({ 0, 0, 1 }, Block::Length() * Vec3(16.0));
  Engine::RenderCommand::Initialize();
  Engine::Renderer::Initialize();
  ChunkRenderer::Initialize();
  m_World.initialize();
}

void GameSandbox::onAttach()
{
}

void GameSandbox::onDetach()
{
}

void GameSandbox::onUpdate(std::chrono::duration<seconds> timestep)
{
  EN_PROFILE_FUNCTION();

  // EN_TRACE("fps: {0}", static_cast<int>(1.0f / timestep.count()));

#if 0
  EN_TRACE("({0}, {1}, {2})", static_cast<int>(m_Player.getPosition()[0] / Block::Length()),
                              static_cast<int>(m_Player.getPosition()[1] / Block::Length()),
                              static_cast<int>(m_Player.getPosition()[2] / Block::Length()));
#endif

#if 0
  LocalIndex chunk = Chunk::ChunkIndexFromPos(m_Player.getPosition());
  EN_TRACE("({0}, {1}, {2})", chunk.i, chunk.j, chunk.k);
#endif

  Engine::RenderCommand::Clear({ 0.788f, 0.949f, 0.949f, 1.0f });

  m_World.onUpdate(timestep);

  // EN_INFO("{0}, {1}, {2}", m_Player.getPosition().x, m_Player.getPosition().y, m_Player.getPosition().z);

  static LOD::Octree tree{};

  const GlobalIndex& origin = Player::OriginIndex();

  Engine::Renderer::BeginScene(Player::Camera());
  std::vector<LOD::Octree::Node*> leaves = tree.getLeaves();
  for (int n = 0; n < leaves.size(); ++n)
  {
    LOD::Octree::Node* leaf = leaves[n];
    const uint64_t lodSize = bit(leaf->LODLevel());

    // EN_INFO("Depth: {0}, Anchor: {1}, {2}, {3}", leaf->depth, leaf->anchor.i, leaf->anchor.j, leaf->anchor.k);

    GlobalIndex lodCenter = leaf->anchor + static_cast<globalIndex_t>(lodSize / 2) * GlobalIndex({ 1, 1, 1 });
    LocalIndex lodCenterLoc = Chunk::CalcRelativeIndex(lodCenter, Player::OriginIndex());

    Vec3 lodDimensions = lodSize * Chunk::Length() * Vec3(1.0);
    Vec3 lodCenterLocPos = Chunk::Length() * Vec3(lodCenterLoc.i, lodCenterLoc.j, lodCenterLoc.k);
    if (lodSize == 1)
      lodCenterLocPos += Chunk::Length() / 2 * Vec3(1.0);

    Engine::Renderer::DrawCubeFrame(lodCenterLocPos, lodDimensions, { 0.1, 0.1, 0.1, 1.0 });

    // Player global position in units of chunks
    Vec3 playerGlobalPos = Vec3(Player::OriginIndex().i, Player::OriginIndex().j, Player::OriginIndex().k) + Player::Position() / Chunk::Length();

    // Split nodes
    if (leaf->LODLevel() > 0)
    {
      // All lengths in units of chunks
      Vec3 lodCenterPos = Vec3(lodCenter.i, lodCenter.j, lodCenter.k);
      Vec3 chunksFromCenter = lodCenterPos - playerGlobalPos;
      length_t lodLength = static_cast<length_t>(lodSize);

      if (glm::dot(chunksFromCenter, chunksFromCenter) < 1.0001 * 0.75 * lodLength * lodLength)
      {
        tree.splitNode(leaves[n]);
        continue;
      }
    }

    // Combine nodes
    if (leaf->depth != 0)
    {
      GlobalIndex parentCenter = leaf->parent->anchor + static_cast<globalIndex_t>(lodSize) * GlobalIndex({ 1, 1, 1 });

      // All lengths in units of chunks
      Vec3 parentCenterPos = Vec3(parentCenter.i, parentCenter.j, parentCenter.k);
      Vec3 chunksFromCenter = parentCenterPos - playerGlobalPos;
      length_t lodLength = static_cast<length_t>(2.0) * static_cast<length_t>(lodSize);

      if (glm::dot(chunksFromCenter, chunksFromCenter) > 1.0001 * 0.75 * lodLength * lodLength)
      {
        tree.combineChildren(leaf->parent);
        goto endLOD;
      }
    }
  }
  Engine::Renderer::EndScene();

endLOD:;

  // EN_WARN("Frame end");
  EN_INFO("{0}", leaves.size());
}

void GameSandbox::onImGuiRender()
{
}

void GameSandbox::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));

  Player::OnEvent(event);
  m_World.onEvent(event);
}

bool GameSandbox::onKeyPressEvent(Engine::KeyPressEvent& event)
{
  static bool wireFrameEnabled = false;
  static bool faceCullingEnabled = false;

  if (event.getKeyCode() == Key::F1)
  {
    wireFrameEnabled = !wireFrameEnabled;
    Engine::RenderCommand::WireFrameToggle(wireFrameEnabled);
  }
  if (event.getKeyCode() == Key::F2)
  {
    faceCullingEnabled = !faceCullingEnabled;
    Engine::RenderCommand::FaceCullToggle(faceCullingEnabled);
  }

  return false;
}
