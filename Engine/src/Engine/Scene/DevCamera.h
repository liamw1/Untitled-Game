#pragma once
#include "Engine/Core/Time.h"
#include "Engine/Events/Event.h"

namespace eng::DevCamera
{
	void OnUpdate(Timestep timestep);
	void OnEvent(event::Event& event);

	length_t Distance();
	void SetDistance(length_t distance);

	void SetViewportSize(u32 width, u32 height);

	const math::Mat4& ViewMatrix();
  math::Mat4 ViewProjection();

	math::Vec3 UpDirection();
	math::Vec3 RightDirection();
	math::Vec3 ForwardDirection();
	const math::Vec3& Position();
  math::Quat Orientation();

	math::Angle Pitch();
  math::Angle Yaw();
}