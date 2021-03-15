#include "velpch.h"
#include "Log.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Velocity
{
	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	void Log::Init()
	{
		// Create sinks for logging
		std::array<spdlog::sink_ptr,2> logSinks;
		logSinks.at(0) = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		logSinks.at(1) = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Velocity.log",true);

		// Set patterns. These are horrible to explain
		logSinks.at(0)->set_pattern("%^[%T] %n: %v%$");
		logSinks.at(1)->set_pattern("[%T] [%l] %n: %v");

		// Create logger and register
		s_CoreLogger = std::make_shared<spdlog::logger>("VELOCITY", begin(logSinks), end(logSinks));
		register_logger(s_CoreLogger);
		s_CoreLogger->set_level(spdlog::level::trace);
		s_CoreLogger->flush_on(spdlog::level::trace);

		// Now create client logger
		s_ClientLogger = std::make_shared<spdlog::logger>("APPLICATION", begin(logSinks), end(logSinks));
		register_logger(s_ClientLogger);
		s_ClientLogger->set_level(spdlog::level::trace);
		s_ClientLogger->flush_on(spdlog::level::trace);

	}
}
