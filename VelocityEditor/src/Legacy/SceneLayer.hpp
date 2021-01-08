//#pragma once
//#include <Velocity.hpp>
//
//#pragma once
//class SceneLayer : public Velocity::Layer
//{
//public:
//	SceneLayer() : Layer("Scene Layer"){};
//
//	void OnAttach() override;
//
//	void OnUpdate(Velocity::Timestep deltaTime) override;
//
//	void OnEvent(Velocity::Event& event) override;
//
//	void OnGuiRender() override;
//
//private:
//	// Square with rainbow tint and standard uv
//	std::vector<Velocity::Vertex> m_Verts = {
//		{{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f,0.0f}},
//		{{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f,0.0f}},
//		{{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f,1.0f}},
//		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f,1.0f}}
//	};
//	std::vector<uint32_t> m_Indices = {
//		0u,1u,2u,2u,3u,0u
//	};
//	// Store renderer handle
//	std::shared_ptr<Velocity::Renderer> r_Renderer;
//
//	// Store loaded textures
//	std::array<uint32_t, 5> m_Textures;
//
//	// Use a velocity default camera
//	std::unique_ptr<Velocity::DefaultCameraController> m_CameraController;
//
//	// Store our scene
//	std::unique_ptr<Velocity::Scene> m_Scene;
//
//	// Store our entities
//	std::vector<Velocity::Entity> m_StaticEntities;
//
//	std::vector<Velocity::Entity> m_DynamicEntities;
//	
//};