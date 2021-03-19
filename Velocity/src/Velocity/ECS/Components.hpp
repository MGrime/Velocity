#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>

namespace Velocity
{
	struct TagComponent
	{
		std::string Tag = "";

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag) : Tag(tag){}
		// I am using seperate functions even though i could use serialize as you need to use one or the other
		// for all things used in the same serilisation
		// And some complex components will need a split
		
		template <class Archive>
		void save (Archive& ar) const
		{
			ar(Tag);
		}

		template <class Archive>
		void load(Archive& ar)
		{
			ar(Tag);
		}
		
	};

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f,0.0f,0.0f };
		glm::vec3 Rotation = { 0.0f,0.0f,0.0f };
		glm::vec3 Scale = { 1.0f,1.0f,1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;

		glm::mat4 GetTransform()
		{
			return translate(glm::mat4(1.0f), Translation) * toMat4(glm::quat(Rotation)) * scale(glm::mat4(1.0f),Scale);
		}

		template<class Archive>
		void save(Archive& ar) const
		{
			ar(Translation.x,Translation.y,Translation.z, 
				Rotation.x,Rotation.y,Rotation.z, 
				Scale.x,Scale.y,Scale.z);
		}

		template<class Archive>
		void load(Archive& ar)
		{
			ar(Translation.x, Translation.y, Translation.z,
				Rotation.x, Rotation.y, Rotation.z,
				Scale.x, Scale.y, Scale.z);
		}
		
	};

	// Submit this to the gpu to tell it you want to render this "mesh"
	struct MeshComponent
	{
		std::string MeshReference = "";

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;

		template <class Archive>
		void save(Archive& ar) const
		{
			ar(MeshReference);
		}

		template <class Archive>
		void load(Archive& ar)
		{
			ar(MeshReference);
		}
	};

	struct TextureComponent
	{
		uint32_t	TextureID = 0u;

		TextureComponent() = default;
		TextureComponent(const TextureComponent&) = default;
		TextureComponent(uint32_t id) { TextureID = id; }

		template <class Archive>
		void save(Archive& ar) const
		{
			ar(TextureID);
		}

		template <class Archive>
		void load(Archive& ar)
		{
			ar(TextureID);
		}
		
	};

	struct PointLightComponent
	{
		glm::vec3 Position = glm::vec3(0.0f,0.0f,0.0f);
		float	  Intensity = 1000.0f;
		glm::vec3 Color = glm::vec3(1.0f,1.0f,1.0f);

		PointLightComponent() = default;
		PointLightComponent(const PointLightComponent&) = default;
		PointLightComponent(const glm::vec3& position, const glm::vec3& color) : Position(position), Color(color) {}

		template <class Archive>
		void save(Archive& ar) const
		{
			ar(Position.x,Position.y,Position.z,Intensity,Color.x,Color.y,Color.z);
		}

		template <class Archive>
		void load(Archive& ar)
		{
			ar(Position.x, Position.y, Position.z, Color.x, Color.y, Color.z);
		}
	};

	struct PBRComponent
	{
		int32_t& AlbedoID() { return TextureIDs[0]; }
		int32_t& NormalID() { return TextureIDs[1]; }
		int32_t& HeightID() { return TextureIDs[2]; }
		int32_t& MetallicID() { return TextureIDs[3]; }
		int32_t& RoughnessID() { return TextureIDs[4]; }

		PBRComponent() = default;
		PBRComponent(const PBRComponent&) = default;

		std::string				MaterialName = "";
		std::array<int32_t, 5> TextureIDs = { 0,0,0,0,0 };

		uint32_t GetSize()
		{
			return sizeof(int32_t) * static_cast<int32_t>(TextureIDs.size());
		}

		// This works for a single component as structs are contiguous
		int32_t* GetPointer()
		{
			return TextureIDs.data();
		}

		template <class Archive>
		void save(Archive& ar) const
		{
			ar(MaterialName,TextureIDs);
		}

		template <class Archive>
		void load(Archive& ar)
		{
			ar(MaterialName, TextureIDs);
		}
		
	};

	// This is really bad but idk
	#define COMPONENT_LIST TagComponent,TransformComponent,MeshComponent,TextureComponent,PointLightComponent,PBRComponent
	
}
