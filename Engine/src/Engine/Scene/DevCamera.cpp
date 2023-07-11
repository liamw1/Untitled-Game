#include "ENpch.h"
#include "DevCamera.h"
#include "Engine/Core/Input.h"
#include "Engine/Events/MouseEvent.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
	static constexpr Angle c_RotationSpeed = 45_deg;

	static Angle s_FOV = 45_deg;
	static float s_AspectRatio = 1.778f;
	static float s_NearClip = 0.1f;
	static float s_FarClip = 1000.0f;

	static Mat4 s_ViewMatrix(1.0);
	static Mat4 s_Projection(1.0);
	static Vec3 s_Position{};
	static Vec3 s_FocalPoint{};

	static Float2 s_InitialMousePosition{};

	static length_t s_Distance = 10.0;
	static Angle s_Pitch, s_Yaw;

	static uint32_t s_ViewportWidth = 1280, s_ViewportHeight = 720;

	static Vec3 calculatePosition()
	{
		return s_FocalPoint - DevCamera::ForwardDirection() * s_Distance;
	}

	static void updateProjection()
	{
		s_AspectRatio = static_cast<float>(s_ViewportWidth) / s_ViewportHeight;
		s_Projection = glm::perspective(s_FOV.radf(), s_AspectRatio, s_NearClip, s_FarClip);
	}

	static void updateView()
	{
		// s_Yaw = s_Pitch = 0.0f; // Lock the camera's rotation
		s_Position = calculatePosition();

		Quat orientation = DevCamera::Orientation();
		s_ViewMatrix = glm::translate(Mat4(1.0f), s_Position) * glm::toMat4(orientation);
		s_ViewMatrix = glm::inverse(s_ViewMatrix);
	}

	static Float2 panSpeed()
	{
		float x = std::min(s_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(s_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return Vec2(xFactor, yFactor);
	}

	static float zoomSpeed()
	{
		float distance = static_cast<float>(s_Distance) * 0.2f;
		distance = std::max(distance, 0.0f);

     float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	static void mousePan(const glm::vec2& delta)
	{
		Float2 pan = panSpeed();
		s_FocalPoint += -DevCamera::RightDirection() * delta.x * pan.x * s_Distance;
		s_FocalPoint += DevCamera::UpDirection() * delta.y * pan.y * s_Distance;
	}

	static void mouseRotate(const glm::vec2& delta)
	{
		float yawSign = DevCamera::UpDirection().y < 0 ? -1.0f : 1.0f;
		s_Yaw += Angle(yawSign * delta.x) * c_RotationSpeed;
		s_Pitch += Angle(delta.y) * c_RotationSpeed;
	}

	static void mouseZoom(float delta)
	{
		s_Distance -= delta * zoomSpeed();
		if (s_Distance < 1.0f)
		{
			s_FocalPoint += DevCamera::ForwardDirection();
			s_Distance = 1.0f;
		}
	}

	static bool onMouseScroll(MouseScrollEvent& event)
	{
		float delta = event.yOffset() * 0.1f;
		mouseZoom(delta);
		updateView();
		return false;
	}

	void DevCamera::OnUpdate(Timestep timestep)
	{
		if (Input::IsKeyPressed(Key::LeftAlt))
		{
			const Float2& mouse = Input::GetMousePosition();
			Float2 delta = (mouse - s_InitialMousePosition) * 0.003f;
			s_InitialMousePosition = mouse;

			if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
				mousePan(delta);
			else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
				mouseRotate(delta);
			else if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
				mouseZoom(delta.y);
		}

		updateView();
	}

	void DevCamera::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.dispatch<MouseScrollEvent>(onMouseScroll);
	}

	length_t DevCamera::Distance() { return s_Distance; }
	void DevCamera::SetDistance(length_t distance) { s_Distance = distance; }

	void DevCamera::SetViewportSize(uint32_t width, uint32_t height)
	{
		s_ViewportWidth = width;
		s_ViewportHeight = height;
		updateProjection();
	}

	const Mat4& DevCamera::ViewMatrix() { return s_ViewMatrix; }
	Mat4 DevCamera::ViewProjection() { return s_Projection * s_ViewMatrix; }

	Vec3 DevCamera::UpDirection() { return glm::rotate(Orientation(), Vec3(0.0, 1.0, 0.0)); }
	Vec3 DevCamera::RightDirection() { return glm::rotate(Orientation(), Vec3(1.0, 0.0, 0.0)); }
	Vec3 DevCamera::ForwardDirection() { return glm::rotate(Orientation(), Vec3(0.0, 0.0, -1.0)); }
	const Vec3& DevCamera::Position() { return s_Position; }
  Quat DevCamera::Orientation() { return Quat(Vec3(-s_Pitch.rad(), -s_Yaw.rad(), 0.0)); }

	Angle DevCamera::Pitch() { return s_Pitch; }
	Angle DevCamera::Yaw() { return s_Yaw; }
}