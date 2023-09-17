include "./premake/customization/solution_items.lua"
include "dependencies.lua"

workspace "Engine"
	architecture "x86_64"
	startproject "Game"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	solution_items
	{
		".editorconfig"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"



group "Dependencies"
	include "Engine/lib/premake/GLFW"
	include "Engine/lib/premake/GLad"
	include "Engine/lib/premake/imgui"
group ""

include "Engine"
include "Game"