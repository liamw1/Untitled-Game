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
	include "Engine/lib/GLFW"
	include "Engine/lib/GLad"
	include "Engine/lib/imgui"
group ""

include "Engine"
include "GameDev"
include "Game"