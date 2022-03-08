#include "GMpch.h"
#include "Player.h"
#include "Engine/Core/Input.h"
#include <glm/gtc/matrix_transform.hpp>

// ============================== Camera Controls ============================== //
static constexpr float s_CameraSensitivity = 0.1f;
static constexpr float s_CameraZoomSensitivity = 0.2f;

static constexpr Angle s_MinPitch = Angle(-89.99f);
static constexpr Angle s_MaxPitch = Angle(89.99f);
static constexpr Angle s_MinFOV = Angle(0.5f);
static constexpr Angle s_MaxFOV = Angle(80.0f);

static constexpr Vec3 s_UpDirection = { 0.0, 0.0, 1.0 };

// Camera initialization
static constexpr Angle s_FOV = Angle(80.0f);
static constexpr float s_AspectRatio = 1280.0f / 720.0f;
static constexpr length_t s_NearClip = static_cast<length_t>(0.125 * Block::Length());
static constexpr length_t s_FarClip = static_cast<length_t>(10000 * Block::Length());

static Float2 s_LastMousePosition{};

class CameraController : public Engine::ScriptableEntity
{
public:
  void onCreate() override
  {
    add<Component::Transform>();
    add<Component::Orientation>();
    auto& cameraComponent = add<Component::Camera>();
    cameraComponent.isActive = true;
    cameraComponent.camera.setPerspective(s_AspectRatio, s_FOV, s_NearClip, s_FarClip);
  }

  void onEvent(Engine::Event& event) override
  {
    Engine::EventDispatcher dispatcher(event);
    dispatcher.dispatch<Engine::MouseMoveEvent>(EN_BIND_EVENT_FN(onMouseMove));
  }

private:
  bool onMouseMove(Engine::MouseMoveEvent& event)
  {
    Mat4& transform = get<Component::Transform>().transform;
    const Vec3& position = transform[3];

    auto& cameraComponent = get<Component::Camera>();
    auto& orientationComponent = get<Component::Orientation>();
    Angle& roll = orientationComponent.roll;
    Angle& pitch = orientationComponent.pitch;
    Angle& yaw = orientationComponent.yaw;

    // Adjust view angles based on mouse movement
    yaw += Angle((event.getX() - s_LastMousePosition.x) * s_CameraSensitivity);
    pitch += Angle((event.getY() - s_LastMousePosition.y) * s_CameraSensitivity);

    pitch = std::max(pitch, s_MinPitch);
    pitch = std::min(pitch, s_MaxPitch);

    // Convert from spherical coordinates to Cartesian coordinates
    Vec3 viewDirection{};

    viewDirection.x = cos(yaw.rad()) * cos(pitch.rad());
    viewDirection.y = -sin(yaw.rad()) * cos(pitch.rad());
    viewDirection.z = -sin(pitch.rad());

    transform = glm::lookAt(position, position + viewDirection, s_UpDirection);

    s_LastMousePosition.x = event.getX();
    s_LastMousePosition.y = event.getY();

    return true;
  }
};



// ================================ Player Data ================================ //
// Time between current frame and previous frame
static Timestep s_Timestep;

// Hitbox dimensions
static constexpr length_t s_Width = 1 * Block::Length();
static constexpr length_t s_Height = 2 * Block::Length();

// Controller for player camera, which is placed at the eyes
// static Engine::CameraController s_CameraController;
static bool s_FreeCamEnabled = false;

// Designates origin of coordinate system
static GlobalIndex s_OriginIndex;

// Position of center of the player hitbox relative to anchor of origin chunk
static Vec3 s_LocalPosition;
static Vec3 s_Velocity;

static length_t s_TranslationSpeed = 32 * Block::Length();

static Engine::Entity s_PlayerEntity;



void Player::Initialize(const GlobalIndex& initialChunkIndex, const Vec3& initialLocalPosition)
{
  EN_ASSERT(initialLocalPosition.x >= 0.0 && initialLocalPosition.x <= Chunk::Length() &&
            initialLocalPosition.y >= 0.0 && initialLocalPosition.y <= Chunk::Length() &&
            initialLocalPosition.z >= 0.0 && initialLocalPosition.z <= Chunk::Length(), "Local position is out of bounds!");

  // s_CameraController = Engine::CameraController(s_FOV, s_AspectRatio, s_NearPlaneDistance, s_FarPlaneDistance);
  s_OriginIndex = initialChunkIndex;
  s_LocalPosition = initialLocalPosition;
  s_Velocity = Vec3(0.0);

  s_PlayerEntity = Engine::Scene::CreateEntity();
  s_PlayerEntity.add<Component::NativeScript>().bind<CameraController>();
  auto& nsc = s_PlayerEntity.get<Component::NativeScript>();
}

void Player::UpdateBegin(Timestep timestep)
{
  s_Timestep = timestep;
  const seconds dt = s_Timestep.sec();  // Time between frames in seconds

  // if (!s_FreeCamEnabled)
  // {
  //   const Vec3& viewDirection = s_CameraController.getViewDirection();
  //   Vec2 planarViewDirection = glm::normalize(Vec2(viewDirection));
  // 
  //   // Update player velocity
  //   s_Velocity = Vec3(0.0);
  //   if (Engine::Input::IsKeyPressed(Key::A))
  //     s_Velocity += Vec3(s_TranslationSpeed * Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
  //   if (Engine::Input::IsKeyPressed(Key::D))
  //     s_Velocity -= Vec3(s_TranslationSpeed * Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
  //   if (Engine::Input::IsKeyPressed(Key::W))
  //     s_Velocity += Vec3(s_TranslationSpeed * planarViewDirection, 0.0);
  //   if (Engine::Input::IsKeyPressed(Key::S))
  //     s_Velocity -= Vec3(s_TranslationSpeed * planarViewDirection, 0.0);
  //   if (Engine::Input::IsKeyPressed(Key::Space))
  //     s_Velocity.z += s_TranslationSpeed;
  //   if (Engine::Input::IsKeyPressed(Key::LeftShift))
  //     s_Velocity.z -= s_TranslationSpeed;
  // 
  //   // Update player position
  //   s_LocalPosition += s_Velocity * dt;
  // }
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

  if (s_PlayerEntity.has<Component::Transform>())
    s_PlayerEntity.get<Component::Transform>().transform[3] = Vec4(s_LocalPosition, 0);

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

const Vec3& Player::Position() { return s_LocalPosition; }
void Player::SetPosition(const Vec3& position) { s_LocalPosition = position; }

const Vec3& Player::Velocity() { return s_Velocity; }
void Player::SetVelocity(const Vec3& velocity) { s_Velocity = velocity; }

const GlobalIndex& Player::OriginIndex() { return s_OriginIndex; }

length_t Player::Width() { return s_Width; }
length_t Player::Height() { return s_Height; }
