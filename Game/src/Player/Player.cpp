#include "GMpch.h"
#include "Player.h"
#include "Engine/Core/Input.h"

// ================================ Player Data ================================ //
// Camera initialization
static constexpr radians s_FOV = static_cast<radians>(glm::radians(80.0));
static constexpr float s_AspectRatio = 1280.0f / 720.0f;
static constexpr length_t s_NearPlaneDistance = static_cast<length_t>(0.125 * Block::Length());
static constexpr length_t s_FarPlaneDistance = static_cast<length_t>(1000 * Block::Length());

// Time between current frame and previous frame
static std::chrono::duration<seconds> s_Timestep;

// Hitbox dimensions
static constexpr length_t s_Width = 1 * Block::Length();
static constexpr length_t s_Height = 2 * Block::Length();

// Controller for player camera, which is placed at the eyes
static Engine::CameraController s_CameraController;
static bool s_FreeCamEnabled = false;

// Designates origin of coordinate system
static GlobalIndex s_OriginIndex;

// Position of center of the player hitbox relative to anchor of origin chunk
static Vec3 s_LocalPosition;
static Vec3 s_Velocity;

static length_t s_TranslationSpeed = 32 * Block::Length();



void Player::Initialize(const GlobalIndex& initialChunkIndex, const Vec3& initialLocalPosition)
{
  EN_ASSERT(initialLocalPosition.x >= 0.0 && initialLocalPosition.x <= Chunk::Length() &&
            initialLocalPosition.y >= 0.0 && initialLocalPosition.y <= Chunk::Length() &&
            initialLocalPosition.z >= 0.0 && initialLocalPosition.z <= Chunk::Length(), "Local position is out of bounds!");

  s_CameraController = Engine::CameraController(s_FOV, s_AspectRatio, s_NearPlaneDistance, s_FarPlaneDistance);
  s_OriginIndex = initialChunkIndex;
  s_LocalPosition = initialLocalPosition;
  s_Velocity = Vec3(0.0);
}

void Player::UpdateBegin(std::chrono::duration<seconds> timestep)
{
  s_Timestep = timestep;
  const seconds dt = s_Timestep.count();  // Time between frames in seconds

  if (!s_FreeCamEnabled)
  {
    const Vec3& viewDirection = s_CameraController.getViewDirection();
    Vec2 planarViewDirection = glm::normalize(Vec2(viewDirection));

    // Update player velocity
    s_Velocity = Vec3(0.0);
    if (Engine::Input::IsKeyPressed(Key::A))
      s_Velocity += Vec3(s_TranslationSpeed * Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
    if (Engine::Input::IsKeyPressed(Key::D))
      s_Velocity -= Vec3(s_TranslationSpeed * Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
    if (Engine::Input::IsKeyPressed(Key::W))
      s_Velocity += Vec3(s_TranslationSpeed * planarViewDirection, 0.0);
    if (Engine::Input::IsKeyPressed(Key::S))
      s_Velocity -= Vec3(s_TranslationSpeed * planarViewDirection, 0.0);
    if (Engine::Input::IsKeyPressed(Key::Space))
      s_Velocity.z += s_TranslationSpeed;
    if (Engine::Input::IsKeyPressed(Key::LeftShift))
      s_Velocity.z -= s_TranslationSpeed;

    // Update player position
    s_LocalPosition += s_Velocity * dt;
  }
}

void Player::UpdateEnd()
{
  // If player enters new chunk, set that chunk as the new origin and recalculate local position
  for (int i = 0; i < 3; ++i)
  {
    globalIndex_t chunkIndexOffset = static_cast<globalIndex_t>(floor(s_LocalPosition[i] / Chunk::Length()));
    s_OriginIndex[i] += chunkIndexOffset;
    s_LocalPosition[i] -= chunkIndexOffset * Chunk::Length();
  }

  const Vec3& viewDirection = s_CameraController.getViewDirection();
  Vec2 planarViewDirection = glm::normalize(Vec2(viewDirection));

  // Update camera position
  Vec3 eyesPosition = Vec3(0.05 * s_Width * planarViewDirection + Vec2(s_LocalPosition), s_LocalPosition.z + 0.25 * s_Height);
  s_CameraController.setPosition(eyesPosition);

  s_CameraController.onUpdate(s_Timestep);
}

void Player::OnEvent(Engine::Event& event)
{
  s_CameraController.onEvent(event);
}

AABB Player::BoundingBox()
{
  Vec3 boundingBoxMin = s_LocalPosition - 0.5 * Vec3(s_Width, s_Width, s_Height);
  Vec3 boundingBoxMax = s_LocalPosition + 0.5 * Vec3(s_Width, s_Width, s_Height);

  return { boundingBoxMin, boundingBoxMax };
}

const Vec3& Player::ViewDirection() { return s_CameraController.getViewDirection(); }
const Engine::Camera& Player::Camera() { return s_CameraController.getCamera(); }

const Vec3& Player::Position() { return s_LocalPosition; }
void Player::SetPosition(const Vec3& position) { s_LocalPosition = position; }

const Vec3& Player::Velocity() { return s_Velocity; }
void Player::SetVelocity(const Vec3& velocity) { s_Velocity = velocity; }

const GlobalIndex& Player::OriginIndex() { return s_OriginIndex; }

length_t Player::Width() { return s_Width; }
length_t Player::Height() { return s_Height; }
