workspace "Velocity"
	architecture "x64"
	startproject "VelocityEditor"
	
	configurations
	{
		"Debug",
		"Release"
	}
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Velocity/vendor/GLFW/include"

group "Dependencies"
	include "Velocity/vendor/GLFW"
	
group ""

project "Velocity"
	location "Velocity"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	
	targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-init/" .. outputdir .. "/%{prj.name}")

	pchheader "velpch.h"
	pchsource "Velocity/src/velpch.cpp"
	
	files 
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp"
	}
	
	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}"
	}
	
	links
	{
		"GLFW"
	}
	
	filter "system:windows"
		systemversion "latest"
	
		defines
		{
			"VEL_PLATFORM_WINDOWS",
			"_CRT_SECURE_NO_WARNINGS"
		}
	
	filter "configurations:Debug"
		defines "VEL_DEBUG"
		runtime "Debug"
		symbols "on"
		
	filter "configurations:Release"
		defines "VEL_RELEASE"
		runtime "Release"
		optimize "on"
	
project "VelocityEditor"
	location "VelocityEditor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	
	targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-init/" .. outputdir .. "/%{prj.name}")
	
	files 
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp"
	}
	
	includedirs
	{
		"Velocity/vendor/spdlog/include",
		"Velocity/src",
		"Velocity/vendor"
	}
	
	links
	{
		"Velocity"
	}
	
	filter "system:windows"
		systemversion "latest"
		defines "VEL_PLATFORM_WINDOWS"
	
	filter "configurations:Debug"
		defines "VEL_DEBUG"
		runtime "Debug"
		symbols "on"
		
	filter "configurations:Release"
		defines "VEL_RELEASE"
		runtime "Release"
		optimize "on"
		
		