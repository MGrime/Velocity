#pragma once
// Velocity.hpp //

// This is the primarily include file for Velocity! //
// This is the ONLY file you need to include to use the engine //

// Base file - Contains macros for project operation
#include "Velocity/Core/Base.hpp"

// Logging
#include "Velocity/Core/Log.hpp"

// Core includes

#include "Velocity/Core/Application.hpp"	// Application base class

#include "Velocity/Core/Window.hpp"

// Layer includes

#include "Velocity/Core/Layers/Layer.hpp"

// Renderer Includes

#include "Velocity/Renderer/Renderer.hpp"
#include "Velocity/Renderer/Texture.hpp"
#include "Velocity/Renderer/Skybox.hpp"
#include "Velocity/Renderer/IBLMap.hpp"

#include "imgui/imgui.h"

// Utility includes
#include "Velocity/Utility/DefaultCameraController.hpp"

// ECS
#include "Velocity/ECS/Scene.hpp"
#include "Velocity/ECS/Entity.hpp"
#include "Velocity/ECS/Components.hpp"

// Math includes
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>