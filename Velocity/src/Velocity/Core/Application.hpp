#pragma once

namespace Velocity 
{
	class Application
	{
	public:
		Application() = default;
		virtual ~Application() = default;

		void Run();

	};

	// To be defined in client
	Application* CreateApplication();

}


