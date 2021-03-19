#pragma once

#include "Scene.hpp"

#include "entt/entt.hpp"

#include <Velocity/Core/Base.hpp>
#include <Velocity/Core/Log.hpp>

namespace Velocity
{
	class Entity
	{
		friend class Scene;
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene) : m_EntityHandle(handle),m_Scene(scene){};
		Entity(const Entity& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			VEL_CORE_ASSERT(!HasComponent<T>(), "Entity already has that component!");
			return  m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			VEL_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			VEL_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}


		template<typename T>
		bool HasComponent()
		{
			VEL_CORE_ASSERT(m_Scene != nullptr, "Entity doesn't have a valid scene handle!");
			return m_Scene->m_Registry.has<T>(m_EntityHandle);
		}

		operator bool() const { return m_EntityHandle != entt::null; }
	private:
		entt::entity m_EntityHandle{ entt::null };
		Scene* m_Scene = nullptr;
	};
}