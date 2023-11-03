#pragma once

#include <Scene/Scene.h>
#include <glm/vec3.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <string>

namespace PBEngine
{
	class Transform_Component : public Scene::Component
	{
	public:
		glm::vec3 position;
		glm::vec3 scale;
		glm::quat rotation;

		void Construct(Scene::Entity entity) override;
		void Show() override;
		//std::string GetName(Scene::Entity entity) override;
		void Rotate(glm::vec3 euler);
	};
}