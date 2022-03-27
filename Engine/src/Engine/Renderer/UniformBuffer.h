#pragma once

namespace Engine
{
	class UniformBuffer
	{
	public:
		virtual ~UniformBuffer() = default;

		virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		static Unique<UniformBuffer> Create(uint32_t size, uint32_t binding);
	};
}