project "Game"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/obj/" .. outputdir .. "/%{prj.name}")

	pchheader "GMpch.h"
	pchsource "src/GMpch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
		"%{IncludeDir.llvm}/**.h",
		"%{IncludeDir.llvm}/**.cpp"
	}

	includedirs
	{
		"src",
		"%{wks.location}/Engine/src"
	}

	sysincludedirs
	{
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.EnTT}",
		"%{IncludeDir.llvm}"
	}

	links
	{
		"Engine"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "EN_DEBUG"
		runtime "Debug"
		symbols "on"

		postbuildcommands
		{
      "{COPY} ../bin/" .. outputdir .. "/Engine/Engine.pdb" .. " ../bin/" .. outputdir .. "/%{prj.name}",
			"{COPYDIR} \"%{LibraryDir.VulkanSDK_DebugDLL}\" \"%{cfg.targetdir}\""
		}

	filter "configurations:Release"
		defines "EN_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "EN_DIST"
		runtime "Release"
		optimize "on"