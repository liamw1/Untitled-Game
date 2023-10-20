#include "ENpch.h"
#include "Window.h"
#include "Platform/Windows/WindowsWindow.h"

namespace eng
{
  std::unique_ptr<Window> Window::Create(const WindowProps& props)
	{
#ifdef ENG_PLATFORM_WINDOWS
		return std::make_unique<WindowsWindow>(props);
#else
		static_assert(false, "Unknown platform!");
#endif
	}
}