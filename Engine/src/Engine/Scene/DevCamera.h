#pragma once
#include "Engine/Events/Event.h"

namespace Engine
{
	namespace DevCamera
	{
		void OnUpdate(Timestep timestep);
		void OnEvent(Event& event);

		length_t Distance();
		void SetDistance(length_t distance);

		void SetViewportSize(uint32_t width, uint32_t height);

		const Mat4& ViewMatrix();
		Mat4 ViewProjection();

		Vec3 UpDirection();
		Vec3 RightDirection();
		Vec3 ForwardDirection();
		const Vec3& Position();
    Quat Orientation();

		Angle Pitch();
	  Angle Yaw();
	};

}