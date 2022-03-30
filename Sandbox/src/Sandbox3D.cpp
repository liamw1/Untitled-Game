#include "SBpch.h"
#include "Sandbox3D.h"
#include "Engine/Scene/Scene.h"

// ============================== Camera Controls ============================== //
static constexpr float s_CameraSensitivity = 0.1f;
static constexpr float s_CameraZoomSensitivity = 0.2f;

static constexpr Angle s_MinPitch(-89.99f);
static constexpr Angle s_MaxPitch(89.99f);
static constexpr Angle s_MinFOV(0.5f);
static constexpr Angle s_MaxFOV(80.0f);

static constexpr Vec3 s_UpDirection(0, 0, 1);

// Camera initialization
static constexpr Angle s_FOV(80.0f);
static constexpr float s_AspectRatio = 1280.0f / 720.0f;
static constexpr length_t s_NearClip = static_cast<length_t>(0.125);
static constexpr length_t s_FarClip = static_cast<length_t>(10000);

static Float2 s_LastMousePosition{};

static length_t s_TranslationSpeed = 8;

class CameraController : public Engine::ScriptableEntity
{
public:
  void onCreate() override
  {
    Component::Camera& cameraComponent = get<Component::Camera>();
    cameraComponent.isActive = true;
    cameraComponent.camera.setPerspective(s_AspectRatio, s_FOV, s_NearClip, s_FarClip);
  }

  void onUpdate(Timestep timestep) override
  {
    const seconds dt = timestep.sec();  // Time between frames in seconds
    Vec3 viewDirection = get<Component::Transform>().orientationDirection();
    Vec2 planarViewDirection = glm::normalize(Vec2(viewDirection));

    // Update player velocity
    Vec3 velocity{};
    if (Engine::Input::IsKeyPressed(Key::A))
      velocity += Vec3(s_TranslationSpeed * Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
    if (Engine::Input::IsKeyPressed(Key::D))
      velocity -= Vec3(s_TranslationSpeed * Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
    if (Engine::Input::IsKeyPressed(Key::W))
      velocity += Vec3(s_TranslationSpeed * planarViewDirection, 0.0);
    if (Engine::Input::IsKeyPressed(Key::S))
      velocity -= Vec3(s_TranslationSpeed * planarViewDirection, 0.0);
    if (Engine::Input::IsKeyPressed(Key::Space))
      velocity.z += s_TranslationSpeed;
    if (Engine::Input::IsKeyPressed(Key::LeftShift))
      velocity.z -= s_TranslationSpeed;

    // Update player position
    get<Component::Transform>().position += velocity * dt;
  }

  void onEvent(Engine::Event& event) override
  {
    Engine::EventDispatcher dispatcher(event);
    dispatcher.dispatch<Engine::MouseMoveEvent>(EN_BIND_EVENT_FN(onMouseMove));
    dispatcher.dispatch<Engine::MouseScrollEvent>(EN_BIND_EVENT_FN(onMouseScroll));
  }

private:
  bool onMouseMove(Engine::MouseMoveEvent& event)
  {
    Component::Transform& transformComponent = get<Component::Transform>();

    const Vec3& rotation = transformComponent.rotation;
    Angle roll = Angle::FromRad(rotation.x);
    Angle pitch = Angle::FromRad(rotation.y);
    Angle yaw = Angle::FromRad(rotation.z);

    // Adjust view angles based on mouse movement
    yaw += Angle((event.getX() - s_LastMousePosition.x) * s_CameraSensitivity);
    pitch += Angle((event.getY() - s_LastMousePosition.y) * s_CameraSensitivity);

    pitch = std::max(pitch, s_MinPitch);
    pitch = std::min(pitch, s_MaxPitch);

    transformComponent.rotation = Vec3(roll.rad(), pitch.rad(), yaw.rad());

    s_LastMousePosition.x = event.getX();
    s_LastMousePosition.y = event.getY();

    return true;
  }

  bool onMouseScroll(Engine::MouseScrollEvent& event)
  {
    Engine::Camera& camera = get<Component::Camera>().camera;

    Angle cameraFOV = camera.getFOV();
    cameraFOV -= s_CameraZoomSensitivity * event.getYOffset() * cameraFOV;
    cameraFOV = std::max(cameraFOV, s_MinFOV);
    cameraFOV = std::min(cameraFOV, s_MaxFOV);

    camera.changeFOV(cameraFOV);
    return true;
  }
};

Sandbox3D::Sandbox3D()
  : Layer("Sandbox3D")
{
  Engine::RenderCommand::Initialize();
  Engine::Renderer::Initialize();
}

void Sandbox3D::onAttach()
{
  EN_PROFILE_FUNCTION();

  m_CheckerboardTexture = Engine::Texture2D::Create("assets/textures/Checkerboard.png");

  m_CameraEntity = Engine::Scene::CreateEntity();
  m_CameraEntity.add<Component::Camera>().isActive = true;
  m_CameraEntity.get<Component::Camera>().camera.setPerspective(1.0f, Angle(90.0f), 0.2f, 100.0f);

  m_CameraEntity.add<Component::NativeScript>().bind<CameraController>();
}

void Sandbox3D::onDetach()
{
}

void Sandbox3D::onUpdate(Timestep timestep)
{
  EN_PROFILE_FUNCTION();

  Engine::RenderCommand::Clear(Float4(0.1f, 0.1f, 0.1f, 1.0f));

  Engine::Renderer::BeginScene(Engine::Scene::ActiveCameraViewProjection());
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      Engine::Renderer::DrawCube(Vec3(static_cast<length_t>((i - 1) * 1.25), 2.0, static_cast<length_t>((j + 1) * 1.25)), Vec3(1.0f), m_CheckerboardTexture.get());
  Engine::Renderer::EndScene();

  Engine::Scene::OnUpdate(timestep);
}

void Sandbox3D::onImGuiRender()
{
}

void Sandbox3D::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));

  Engine::Scene::OnEvent(event);
}

bool Sandbox3D::onKeyPressEvent(Engine::KeyPressEvent& event)
{
  static bool mouseEnabled = false;
  static bool wireFrameEnabled = false;
  static bool faceCullingEnabled = false;

  if (event.getKeyCode() == Key::Escape)
  {
    mouseEnabled = !mouseEnabled;
    mouseEnabled ? Engine::Application::Get().getWindow().enableCursor() : Engine::Application::Get().getWindow().disableCursor();
  }
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
