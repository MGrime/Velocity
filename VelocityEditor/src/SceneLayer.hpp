#pragma once
#include <Velocity.hpp>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#pragma once
class SceneLayer : public Velocity::Layer
{
public:
	SceneLayer() : Layer("Scene Layer"), m_View {}, m_Projection{} {}

	void OnAttach() override;

	void OnUpdate() override;

private:
	// Square with rainbow tint and standard uv
	std::vector<Velocity::Vertex> m_Verts = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f,0.0f}},
		{{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f,0.0f}},
		{{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f,1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f,1.0f}}
	};
	std::vector<uint32_t> m_Indices = {
		0u,1u,2u,2u,3u,0u
	};
	// Store renderer handle
	std::shared_ptr<Velocity::Renderer> r_Renderer;
	
	// Store the handle for the ^ square to be rendered
	Velocity::BufferManager::Renderable m_Square;

	std::array<uint32_t, 5> m_Textures;

	// "Fake" camera as we dont have one right now
	glm::mat4 m_View, m_Projection;

	// Reuse matrix for models
	glm::mat4 m_ModelMatrix;
	
};