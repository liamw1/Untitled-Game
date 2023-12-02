#include "GMpch.h"
#include "Player.h"
#include "PlayerCamera.h"
#include "World/Chunk/Chunk.h"

// Hitbox dimensions
static constexpr length_t c_Width = 0.6_m * block::length();
static constexpr length_t c_Height = 1.8_m * block::length();

static bool s_FreeCamEnabled = false;
static constexpr length_t c_TranslationSpeed = 500 * block::length();

// Designates origin of coordinate system
static GlobalIndex s_OriginIndex;

// Position of center of the player hitbox relative to anchor of origin chunk
static eng::math::Vec3 s_Velocity;

static eng::Entity s_PlayerEntity;
static eng::Entity s_CameraEntity;
static std::mutex s_Mutex;



void player::initialize(const GlobalIndex& chunkIndex, const eng::math::Vec3& positionWithinChunk)
{
  ENG_ASSERT(positionWithinChunk.x >= 0.0 && positionWithinChunk.x <= Chunk::Length() &&
             positionWithinChunk.y >= 0.0 && positionWithinChunk.y <= Chunk::Length() &&
             positionWithinChunk.z >= 0.0 && positionWithinChunk.z <= Chunk::Length(), "Local position is out of bounds!");

  s_OriginIndex = chunkIndex;
  s_Velocity = eng::math::Vec3(0.0);

  s_PlayerEntity = eng::scene::CreateEntity(positionWithinChunk, "Player");

  s_CameraEntity = eng::scene::CreateEntity(positionWithinChunk + height() / 2, "PlayerCamera");
  s_CameraEntity.add<eng::component::NativeScript>().bind<CameraController>();
  s_CameraEntity.add<eng::component::Camera>();
}

void player::handleDirectionalInput()
{
  std::lock_guard lock(s_Mutex);

  eng::math::Vec3 viewDirection = s_CameraEntity.get<eng::component::Transform>().orientationDirection();
  eng::math::Vec2 planarViewDirection = glm::normalize(eng::math::Vec2(viewDirection));

  // Update player velocity
  eng::math::Vec3 velocity{};
  if (eng::input::isKeyPressed(eng::input::Key::A))
    velocity += eng::math::Vec3(c_TranslationSpeed * eng::math::Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
  if (eng::input::isKeyPressed(eng::input::Key::D))
    velocity -= eng::math::Vec3(c_TranslationSpeed * eng::math::Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
  if (eng::input::isKeyPressed(eng::input::Key::W))
    velocity += eng::math::Vec3(c_TranslationSpeed * planarViewDirection, 0.0);
  if (eng::input::isKeyPressed(eng::input::Key::S))
    velocity -= eng::math::Vec3(c_TranslationSpeed * planarViewDirection, 0.0);
  if (eng::input::isKeyPressed(eng::input::Key::Space))
    velocity.z += c_TranslationSpeed;
  if (eng::input::isKeyPressed(eng::input::Key::LeftShift))
    velocity.z -= c_TranslationSpeed;
  s_Velocity = velocity;
}

void player::updatePosition(eng::Timestep timestep)
{
  std::lock_guard lock(s_Mutex);
  const seconds dt = timestep.sec();

  // Calculate new player position based on velocity
  eng::math::Vec3 previousPosition = s_PlayerEntity.get<eng::component::Transform>().position;
  eng::math::Vec3 newPosition = previousPosition + s_Velocity * dt;

  // Calculate new camera position
  const eng::math::Vec3& viewDirection = s_CameraEntity.get<eng::component::Transform>().orientationDirection();
  eng::math::Vec2 planarViewDirection = glm::normalize(eng::math::Vec2(viewDirection));
  eng::math::Vec3 eyesPosition = eng::math::Vec3(0.125 * c_Width * planarViewDirection + eng::math::Vec2(newPosition), newPosition.z + 0.4 * c_Height);

  // If player camera enters new chunk, set that chunk as the new origin and recalculate positions relative to new origin
  GlobalIndex chunkIndexOffset = GlobalIndex::ToIndex(eyesPosition / Chunk::Length());
  s_OriginIndex += chunkIndexOffset;
  newPosition -= Chunk::Length() * static_cast<eng::math::Vec3>(chunkIndexOffset);
  eyesPosition -= Chunk::Length() * static_cast<eng::math::Vec3>(chunkIndexOffset);

  // Update player and camera positions
  s_PlayerEntity.get<eng::component::Transform>().position = newPosition;
  s_CameraEntity.get<eng::component::Transform>().position = eyesPosition;
}

eng::math::Vec3 player::position()
{
  std::lock_guard lock(s_Mutex);
  return s_PlayerEntity.get<eng::component::Transform>().position;
}

void player::setPosition(const eng::math::Vec3& position)
{
  std::lock_guard lock(s_Mutex);
  s_PlayerEntity.get<eng::component::Transform>().position = position;
}

eng::math::Vec3 player::velocity()
{
  std::lock_guard lock(s_Mutex);
  return s_Velocity;
}

void player::setVelocity(const eng::math::Vec3& velocity)
{
  std::lock_guard lock(s_Mutex);
  s_Velocity = velocity;
}

eng::math::Vec3 player::cameraPosition()
{
  std::lock_guard lock(s_Mutex);
  return s_CameraEntity.get<eng::component::Transform>().position;
}

eng::math::Vec3 player::viewDirection()
{
  std::lock_guard lock(s_Mutex);
  return s_CameraEntity.get<eng::component::Transform>().orientationDirection();
}

GlobalIndex player::originIndex()
{
  std::lock_guard lock(s_Mutex);
  return s_OriginIndex;
}

length_t player::width() { return c_Width; }
length_t player::height() { return c_Height; }
