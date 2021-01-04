#pragma once

#include "Application.hpp"
#include "Log.hpp"


#ifdef VEL_PLATFORM_WINDOWS

	extern Velocity::Application* Velocity::CreateApplication();

	int main(int argc, char** argv)
	{
		Velocity::Log::Init();
		auto app = Velocity::CreateApplication();
		app->Run();
		delete app;
	}

#endif