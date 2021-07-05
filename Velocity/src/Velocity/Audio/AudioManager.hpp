#pragma once

#include "cute_sound.h"
#include "Velocity/Core/Log.hpp"

namespace Velocity
{
	class AudioManager
	{
	public:
		// Create wrapper structs for sounds
		struct SoundClip
		{
			SoundClip() { m_RawSoundClip.loaded_sound = nullptr; }
			SoundClip(cs_playing_sound_t sound) : m_RawSoundClip(sound) {}
			~SoundClip() = default;
			void Play()
			{
				const auto manager = GetManager();
				if (m_RawSoundClip.loaded_sound != nullptr)
				{
					cs_insert_sound(manager->m_CSContext, &m_RawSoundClip);
				}
			}

			// Wrappers to edit parameters
			void SetVolume(const float newVol)
			{
				if (newVol > 1.0f || newVol < 0.0f)
				{
					VEL_CORE_WARN("Ignoring volume change. Cannot be less than 0 or over 1");
					return;
				}
				cs_set_volume(&m_RawSoundClip, newVol, newVol);
			}

			// Get vol
			[[nodiscard]] float GetVolume() const
			{
				return m_RawSoundClip.volume0;
			}

			// Pan L/R (0.0f full left, 1.0f full right, 0.5f middle)
			void SetPan(const float pan)
			{
				if (pan > 1.0f || pan < 0.0f)
				{
					VEL_CORE_WARN("Ignoring pan change. Cannot be less than 0 or over 1");
					return;
				}
				cs_set_pan(&m_RawSoundClip, pan);
			}

			// Get pan amount
			[[nodiscard]] float GetPan() const
			{
				return m_RawSoundClip.pan0;
			}

			void SetLoop(bool loop)
			{
				cs_loop_sound(&m_RawSoundClip, loop);
			}

			[[nodiscard]] bool IsLooped() const
			{
				return m_RawSoundClip.looped;
			}

			// check play state
			bool IsPlaying()
			{
				return cs_is_active(&m_RawSoundClip);
			}
		private:
			cs_playing_sound_t m_RawSoundClip{};
		};

		// Sound clip needs lower access to the sound manager
		friend struct SoundClip;

		AudioManager();

		~AudioManager();

		void process();

		static std::shared_ptr<AudioManager>& GetManager()
		{
			if (!s_Manager)
			{
				s_Manager = std::make_shared<AudioManager>();
			}
			return s_Manager;
		}

		// Loads given sound and returns as a playable sound clip
		// Caches loaded sound to allow multiple clips to made from the same file without loading more than once
		SoundClip LoadSound(const std::string& filePath);

	private:
		// Singleton
		static std::shared_ptr<AudioManager> s_Manager;

		// Cute_sound things
		cs_context_t* m_CSContext;

		// Store a hashmap of files to loaded sounds
		std::unordered_map<const char*, cs_loaded_sound_t> m_LoadedSounds;
	};
}