#include "GMpch.h"
#include "Player.h"
#include "PlayerCamera.h"

// Hitbox dimensions
static constexpr length_t s_Width = 1 * Block::Length();
static constexpr length_t s_Height = 2 * Block::Length();

// Controller for player camera, which is placed at the eyes
// static Engine::CameraController s_CameraController;
static bool s_FreeCamEnabled = false;

// Designates origin of coordinate system
static GlobalIndex s_OriginIndex;

// Position of center of the player hitbox relative to anchor of origin chunk
static Vec3 s_Velocity;

static Engine::Entity s_PlayerEntity;
static std::mutex s_Mutex;



void Player::Initialize(const GlobalIndex& initialChunkIndex, const Vec3& initialLocalPosition)
{
  EN_ASSERT(initialLocalPosition.x >= 0.0 && initialLocalPosition.x <= Chunk::Length() &&
            initialLocalPosition.y >= 0.0 && initialLocalPosition.y <= Chunk::Length() &&
            initialLocalPosition.z >= 0.0 && initialLocalPosition.z <= Chunk::Length(), "Local position is out of bounds!");

  s_OriginIndex = initialChunkIndex;
  s_Velocity = Vec3(0.0);

  s_PlayerEntity = Engine::Scene::CreateEntity(initialLocalPosition);
  s_PlayerEntity.add<Component::NativeScript>().bind<CameraController>();
  s_PlayerEntity.add<Component::Camera>();
}

void Player::Update()
{
  std::lock_guard lock(s_Mutex);

  const Vec3 lastLocalPosition = s_PlayerEntity.get<Component::Transform>().position;

  // If player enters new chunk, set that chunk as the new origin and recalculate local position
  Vec3 newLocalPosition = lastLocalPosition;
  for (int i = 0; i < 3; ++i)
  {
    globalIndex_t chunkIndexOffset = static_cast<globalIndex_t>(floor(lastLocalPosition[i] / Chunk::Length()));
    s_OriginIndex[i] += chunkIndexOffset;
    newLocalPosition[i] -= chunkIndexOffset * Chunk::Length();
  }
  s_PlayerEntity.get<Component::Transform>().position = newLocalPosition;

  // const Vec3& viewDirection = s_CameraController.getViewDirection();
  // Vec2 planarViewDirection = glm::normalize(Vec2(viewDirection));
  // 
  // // Update camera position
  // Vec3 eyesPosition = Vec3(0.025 * s_Width * planarViewDirection + Vec2(s_LocalPosition), s_LocalPosition.z + 0.25 * s_Height);
  // s_CameraController.setPosition(eyesPosition);
  // 
  // s_CameraController.onUpdate(s_Timestep);
  // s_PlayerEntity.get<Component::Camera>().camera.setProjection(s_CameraController.getViewProjectionMatrix());
}

Vec3 Player::Position()
{
  std::lock_guard lock(s_Mutex);
  return s_PlayerEntity.get<Component::Transform>().position;
}

void Player::SetPosition(const Vec3& position)
{
  std::lock_guard lock(s_Mutex);
  s_PlayerEntity.get<Component::Transform>().position = position;
}

Vec3 Player::Velocity()
{
  std::lock_guard lock(s_Mutex);
  return s_Velocity;
}

void Player::SetVelocity(const Vec3& velocity)
{
  std::lock_guard lock(s_Mutex);
  s_Velocity = velocity;
}

Vec3 Player::ViewDirection()
{
  std::lock_guard lock(s_Mutex);
  return s_PlayerEntity.get<Component::Transform>().orientationDirection();
}

GlobalIndex Player::OriginIndex()
{
  std::lock_guard lock(s_Mutex);
  return s_OriginIndex;
}

length_t Player::Width() { return s_Width; }
length_t Player::Height() { return s_Height; }
