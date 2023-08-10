#include "GMpch.h"
#include "Player.h"
#include "PlayerCamera.h"

// Hitbox dimensions
static constexpr length_t c_Width = 0.6_m * Block::Length();
static constexpr length_t c_Height = 1.8_m * Block::Length();

static bool s_FreeCamEnabled = false;
static constexpr length_t c_TranslationSpeed = 250 * Block::Length();

// Designates origin of coordinate system
static GlobalIndex s_OriginIndex;

// Position of center of the player hitbox relative to anchor of origin chunk
static Vec3 s_Velocity;

static Engine::Entity s_PlayerEntity;
static Engine::Entity s_CameraEntity;
static std::mutex s_Mutex;



void Player::Initialize(const GlobalIndex& chunkIndex, const Vec3& positionWithinChunk)
{
  EN_ASSERT(positionWithinChunk.x >= 0.0 && positionWithinChunk.x <= Chunk::Length() &&
            positionWithinChunk.y >= 0.0 && positionWithinChunk.y <= Chunk::Length() &&
            positionWithinChunk.z >= 0.0 && positionWithinChunk.z <= Chunk::Length(), "Local position is out of bounds!");

  s_OriginIndex = chunkIndex;
  s_Velocity = Vec3(0.0);

  s_PlayerEntity = Engine::Scene::CreateEntity(positionWithinChunk, "Player");

  s_CameraEntity = Engine::Scene::CreateEntity(positionWithinChunk + Height() / 2, "PlayerCamera");
  s_CameraEntity.add<Component::NativeScript>().bind<CameraController>();
  s_CameraEntity.add<Component::Camera>();
}

void Player::HandleDirectionalInput()
{
  std::lock_guard lock(s_Mutex);

  Vec3 viewDirection = s_CameraEntity.get<Component::Transform>().orientationDirection();
  Vec2 planarViewDirection = glm::normalize(Vec2(viewDirection));

  // Update player velocity
  Vec3 velocity{};
  if (Engine::Input::IsKeyPressed(Key::A))
    velocity += Vec3(c_TranslationSpeed * Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
  if (Engine::Input::IsKeyPressed(Key::D))
    velocity -= Vec3(c_TranslationSpeed * Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
  if (Engine::Input::IsKeyPressed(Key::W))
    velocity += Vec3(c_TranslationSpeed * planarViewDirection, 0.0);
  if (Engine::Input::IsKeyPressed(Key::S))
    velocity -= Vec3(c_TranslationSpeed * planarViewDirection, 0.0);
  if (Engine::Input::IsKeyPressed(Key::Space))
    velocity.z += c_TranslationSpeed;
  if (Engine::Input::IsKeyPressed(Key::LeftShift))
    velocity.z -= c_TranslationSpeed;
  s_Velocity = velocity;
}

void Player::UpdatePosition(Engine::Timestep timestep)
{
  const seconds dt = timestep.sec();
  Vec3 lastPosition = s_PlayerEntity.get<Component::Transform>().position;

  // Calculate new player position based on velocity
  Vec3 newPosition = lastPosition + s_Velocity * dt;

  // If player enters new chunk, set that chunk as the new origin and recalculate position relative to new origin
  GlobalIndex chunkIndexOffset = GlobalIndex::ToIndex(lastPosition / Chunk::Length());
  s_OriginIndex += chunkIndexOffset;
  newPosition -= Chunk::Length() * static_cast<Vec3>(chunkIndexOffset);

  // Update player position
  s_PlayerEntity.get<Component::Transform>().position = newPosition;

  // Update camera position
  const Vec3& viewDirection = s_CameraEntity.get<Component::Transform>().orientationDirection();
  Vec2 planarViewDirection = glm::normalize(Vec2(viewDirection));
  Vec3 eyesPosition = Vec3(0.125 * c_Width * planarViewDirection + Vec2(newPosition), newPosition.z + 0.4 * c_Height);
  s_CameraEntity.get<Component::Transform>().position = eyesPosition;
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

Vec3 Player::CameraPosition()
{
  std::lock_guard lock(s_Mutex);
  return s_CameraEntity.get<Component::Transform>().position;
}

Vec3 Player::ViewDirection()
{
  std::lock_guard lock(s_Mutex);
  return s_CameraEntity.get<Component::Transform>().orientationDirection();
}

GlobalIndex Player::OriginIndex()
{
  std::lock_guard lock(s_Mutex);
  return s_OriginIndex;
}

length_t Player::Width() { return c_Width; }
length_t Player::Height() { return c_Height; }
