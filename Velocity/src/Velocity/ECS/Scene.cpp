#include "velpch.h"

#include "Scene.hpp"

#include "Components.hpp"
#include "Entity.hpp"

namespace Velocity
{
	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "New Entity" : name;
		return entity;
	}
}
