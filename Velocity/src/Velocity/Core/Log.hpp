#pragma once

#include "Base.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Velocity
{
	class Log
	{
	public:
		static void Init();

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;

	};
}

// Define functions to use logger

template <class... Args>
constexpr auto VEL_CORE_TRACE(Args&&... args)
{
	Velocity::Log::GetCoreLogger()->trace(std::forward<Args>(args)...);
}
template <class... Args>
constexpr auto VEL_CORE_INFO(Args&&... args)
{
	Velocity::Log::GetCoreLogger()->info(std::forward<Args>(args)...);
}
template <class... Args>
constexpr auto VEL_CORE_WARN(Args&&... args)
{
	Velocity::Log::GetCoreLogger()->warn(std::forward<Args>(args)...);
}

template <class... Args>
constexpr auto VEL_CORE_ERROR(Args&&... args)
{
	Velocity::Log::GetCoreLogger()->error(std::forward<Args>(args)...);
}

template <class... Args>
constexpr auto VEL_CORE_CRITICAL(Args&&... args)
{
	Velocity::Log::GetCoreLogger()->critical(std::forward<Args>(args)...);
}

template <class... Args>
constexpr auto VEL_CLIENT_TRACE(Args&&... args)
{
	Velocity::Log::GetClientLogger()->trace(std::forward<Args>(args)...);
}
template <class... Args>
constexpr auto VEL_CLIENT_INFO(Args&&... args)
{
	Velocity::Log::GetClientLogger()->info(std::forward<Args>(args)...);
}
template <class... Args>
constexpr auto VEL_CLIENT_WARN(Args&&... args)
{
	Velocity::Log::GetClientLogger()->warn(std::forward<Args>(args)...);
}

template <class... Args>
constexpr auto VEL_CLIENT_ERROR(Args&&... args)
{
	Velocity::Log::GetClientLogger()->error(std::forward<Args>(args)...);
}

template <class... Args>
constexpr auto VEL_CLIENT_CRITICAL(Args&&... args)
{
	Velocity::Log::GetClientLogger()->critical(std::forward<Args>(args)...);
}
