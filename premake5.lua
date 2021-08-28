workspace "Engine"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include system directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "Engine/lib/GLFW/include"

include "Engine/lib/GLFW"



project "Engine"
	location "Engine"
	kind "SharedLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "On"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("obj/" .. outputdir .. "/%{prj.name}")

	pchheader "ENpch.h"
	pchsource "Engine/src/ENpch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	includedirs
	{
		"%{prj.name}/src/",
		"%{IncludeDir.GLFW}",
		"%{prj.name}/lib/spdlog/include"
	}

	links
	{
		"GLFW",
		"opengl32.lib"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"EN_PLATFORM_WINDOWS",
			"EN_BUILD_DLL"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
		}

	filter "configurations:Debug"
		defines { "EN_DEBUG", "EN_ENABLE_ASSERTS" }
		symbols "On"

	filter "configurations:Release"
		defines "EN_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "EN_DIST"
		optimize "On"



project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "On"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("obj/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Engine/src",
		"Engine/lib/spdlog/include"
	}

	links
	{
		"Engine"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"EN_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "EN_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "EN_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "EN_DIST"
		optimize "On"