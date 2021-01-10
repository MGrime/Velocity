#include "velpch.h"

#include "Scene.hpp"

#include "Components.hpp"
#include "Entity.hpp"

#include <Velocity/Renderer/Renderer.hpp>

namespace Velocity
{
	Scene::Scene()
	{
		m_Registry.on_construct<PointLightComponent>().connect<&Scene::OnPointLightChanged>(this);
		m_Registry.on_destroy<PointLightComponent>().connect<&Scene::OnPointLightChanged>(this);
		m_Registry.on_update<PointLightComponent>().connect<&Scene::OnPointLightChanged>(this);
	}
	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "New Entity" : name;
		m_Entities.push_back(entity);
		return entity;
	}

	void Scene::OnPointLightChanged(entt::registry& reg, entt::entity entity)
	{
		Renderer::GetRenderer()->UpdatePointlightArray();
	}

}
