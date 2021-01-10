workspace "Velocity"
	architecture "x64"
	startproject "VelocityEditor"
	
	configurations
	{
		"Debug",
		"Release"
	}
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

vulkanpath = os.getenv("VK_SDK_PATH")

if (vulkanpath == nil)
then
	io.write("Vulkan SDK not found! Please install from LunarG")
	os.exit()
end

IncludeDir = {}
IncludeDir["GLFW"] = "Velocity/vendor/GLFW/include"
IncludeDir["glm"] = "Velocity/vendor/glm"
IncludeDir["vulkan"] = vulkanpath .. "/Include"
IncludeDir["stb"] = "Velocity/vendor/stb"
IncludeDir["assimp"] = "Velocity/vendor/assimp/include"
IncludeDir["imgui"] = "Velocity/vendor/imgui"
IncludeDir["entt"] = "Velocity/vendor/entt/src"

group "Dependencies"
	include "Velocity/vendor/GLFW"
	include "Velocity/vendor/imgui"
	
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
		"%{prj.name}/src/**.cpp",
		"%{IncludeDir.stb}/**.cpp"
	}
	
	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.vulkan}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.imgui}",
		"%{IncludeDir.entt}"
	}
	
	links
	{
		"GLFW",
		vulkanpath .. "/Lib/vulkan-1.lib",
		"imgui"
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
		links
		{
			"Velocity/vendor/assimp/Debug/assimp-vc142-mtd.lib"
		}

		
	filter "configurations:Release"
		defines "VEL_RELEASE"
		runtime "Release"
		optimize "on"
		links
		{
			"Velocity/vendor/assimp/Release/assimp-vc142-mt.lib"
		}
	
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
		"Velocity/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.vulkan}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.imgui}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.entt}"
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
		links
		{
			"Velocity/vendor/assimp/Debug/assimp-vc142-mtd.lib"
		}
		postbuildcommands
		{
			("{COPY} Velocity/vendor/assimp/Debug/assimp-vc142-mtd.dll %{cfg.buildtarget.relpath}/assimp.dll")
		}

		
	filter "configurations:Release"
		defines "VEL_RELEASE"
		runtime "Release"
		optimize "on"
		links
		{
			"Velocity/vendor/assimp/Release/assimp-vc142-mt.lib"
		}
		postbuildcommands
		{
			("{COPY} Velocity/vendor/assimp/Release/assimp-vc142-mt.dll %{cfg.buildtarget.relpath}/assimp.dll")
		}
		