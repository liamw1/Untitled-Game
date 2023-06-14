#include "ENpch.h"
#include "Window.h"
#include "Platform/Windows/WindowsWindow.h"

namespace Engine
{
  std::unique_ptr<Window> Window::Create(const WindowProps& props)
	{
#ifdef EN_PLATFORM_WINDOWS
		return std::make_unique<WindowsWindow>(props);
#else
		EN_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
#endif
	}
}