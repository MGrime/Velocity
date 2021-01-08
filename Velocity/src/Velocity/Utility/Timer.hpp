#pragma once

#include <chrono>

// Timer using std::chrono for cross platform

namespace Velocity
{

	// Stores in seconds.
	struct Timestep
	{
		float Time;

		float GetSeconds() const { return Time; }
		float GetMilliseconds() const { return Time * 1000.0f; }

		operator float() const { return Time; }
	};

	class FrameTimer
	{
	public:
		FrameTimer()
		{
			m_LastTime = std::chrono::high_resolution_clock::now();
		};

		Timestep GetDeltaTime()
		{
			const auto now = std::chrono::high_resolution_clock::now();

			std::chrono::duration<float, std::milli> time = now - m_LastTime;

			const Timestep deltaTime = {
				static_cast<float>(time.count()) / 1000.0f
			};

			m_LastTime = now;

			return deltaTime;
		};

	private:
		std::chrono::high_resolution_clock::time_point m_LastTime;
	};

}