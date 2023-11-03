#pragma once

#include "../Scene.h"

namespace PBEngine
{
	class Rendering : public Scene::Service
	{
	public:
		void Construct() override;
		void ComponentAdded(Scene::Entity entity) override;
		void Update(Scene::Entity entity) override;

	private:
		std::unordered_map<Scene::Entity, uint8_t> componentMasks;
	};
}