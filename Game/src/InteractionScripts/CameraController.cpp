#include "GMpch.h"
#include <glm/gtc/matrix_transform.hpp>

static constexpr float s_CameraSensitivity = 0.1f;
static constexpr float s_CameraZoomSensitivity = 0.2f;

static constexpr Angle s_MinPitch(-89.99f);
static constexpr Angle s_MaxPitch(89.99f);
static constexpr Angle s_MinFOV(0.5f);
static constexpr Angle s_MaxFOV(80.0f);

static constexpr Vec3 s_UpDirection(0, 0, 1);

static Float2 s_LastMousePosition{};

class CameraController : public Engine::ScriptableEntity
{
public:
  void onEvent(Engine::Event& event) override
  {
    Engine::EventDispatcher dispatcher(event);
    dispatcher.dispatch<Engine::MouseMoveEvent>(EN_BIND_EVENT_FN(onMouseMove));
  }

private:
  bool onMouseMove(Engine::MouseMoveEvent& event)
  {
    Component::Transform& transformComponent = get<Component::Transform>();
    Mat4& transform = transformComponent.transform;
    Vec3 position = transformComponent.getPosition();

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

    EN_INFO("{0}, {1}, {2}", viewDirection.x, viewDirection.y, viewDirection.z);

    transform = glm::lookAt(position, position + viewDirection, s_UpDirection);

    s_LastMousePosition.x = event.getX();
    s_LastMousePosition.y = event.getY();

    return true;
  }
};