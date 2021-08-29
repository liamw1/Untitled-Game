workspace "Engine"
	architecture "x64"
	startproject "Sandbox"

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
IncludeDir["GLad"] = "Engine/lib/GLad/include"
IncludeDir["ImGui"] = "Engine/lib/imgui"



group "Dependencies"
	include "Engine/lib/GLFW"
	include "Engine/lib/GLad"
	include "Engine/lib/imgui"
group ""

project "Engine"
	location "Engine"
	kind "SharedLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "Off"

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
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	includedirs
	{
		"%{prj.name}/src/",
		"%{prj.name}/lib/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.GLad}",
		"%{IncludeDir.ImGui}"
	}

	links
	{
		"GLFW",
		"GLad",
		"ImGui",
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
			("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Sandbox/\"")
		}

	filter "configurations:Debug"
		defines "EN_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "EN_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "EN_DIST"
		runtime "Release"
		optimize "On"



project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "Off"

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
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "EN_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "EN_DIST"
		runtime "Release"
		optimize "On"