#include "velpch.h"

// This is the same as including the header here, but allows including in class header
#define CUTE_SOUND_IMPLEMENTATION
#include "AudioManager.hpp"

#include "Velocity/Core/Application.hpp"
#include "Velocity/Core/Log.hpp"

#ifdef VEL_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

std::shared_ptr<Velocity::AudioManager> Velocity::AudioManager::s_Manager = nullptr;

namespace Velocity
{
	AudioManager::AudioManager()
	{
		// Get native win32 handle
		auto glfwWin = Application::GetWindow()->GetNative();
		auto hwnd = glfwGetWin32Window(glfwWin);

		// Create cs context
		m_CSContext = cs_make_context(hwnd, 44100, 8192, 0, nullptr);

		if (!m_CSContext)
		{
			VEL_CORE_ERROR("Failed to create cute_sound context!");
			throw std::runtime_error("See console!");
		}
	}
	
	AudioManager::~AudioManager()
	{
		for (auto& sound : m_LoadedSounds)
		{
			cs_free_sound(&sound.second);
		}
	}

	void AudioManager::process()
	{
		if (!m_LoadedSounds.empty())
		{
			cs_mix(m_CSContext);
		}
	}

	AudioManager::SoundClip AudioManager::LoadSound(const std::string& filePath)
	{
		if (filePath.find(".wav") == std::string::npos)
		{
			VEL_CORE_ERROR("Only 16 bit .wav format sounds are supported!");
			throw std::runtime_error("See console!");
		}
		

		// Load and insert sound if not loaded
		if (m_LoadedSounds.find(filePath.c_str()) == m_LoadedSounds.end())
		{
			m_LoadedSounds.insert({
				filePath.c_str(),
				cs_load_wav(filePath.c_str())
			});

			if (m_LoadedSounds[filePath.c_str()].channels[0] == nullptr)
			{
				VEL_CORE_ERROR("Failed to load sound: " + std::string(cs_error_reason));
				throw std::runtime_error("See console!");			
			}
		}

		// Create playable clip
		return {
			cs_make_playing_sound(&m_LoadedSounds[filePath.c_str()])
		};
		
	}
}



