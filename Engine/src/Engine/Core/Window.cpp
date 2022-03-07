#include "ENpch.h"
#include "Window.h"
#include "Platform/Windows/WindowsWindow.h"

namespace Engine
{
	Unique<Window> Window::Create(const WindowProps& props)
	{
#ifdef EN_PLATFORM_WINDOWS
		return CreateUnique<WindowsWindow>(props);
#else
		EN_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
#endif
	}
}