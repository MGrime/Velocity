#include "EditorApplication.hpp"		// Our app

#include "Velocity/Core/EntryPoint.hpp"	// Defines main.cpp

using namespace Velocity;

// This is used in main.cpp
Application* Velocity::CreateApplication()
{
	return new EditorApplication();
}