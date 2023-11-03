#include "Rendering.h"
#include <Scene/SceneManagement.h>
#include <Scene/Components/Transform.h>
#include <Scene/Helpers/GeneralHelper.h>

#include <memory>

namespace PBEngine
{
	void Rendering::Construct()
	{
		/*std::vector<std::shared_ptr<Scene::Entity>> entities = scene.GetAllEntities();

		for (size_t i = 0; i < entities.size(); i++)
		{
			componentMasks[*entities[i]] = 0;
		}*/
	}

	void Rendering::ComponentAdded(Scene::Entity entity)
	{
		std::vector<std::shared_ptr<Scene::Component>> components = SceneManager::GetCurrent()->GetComponents<Transform_Component, Scene::Test_Component>(entity);
		uint8_t componentMask = componentMasks[entity];

		for (size_t i = 0; i < components.size(); i++)
		{
			if (!((componentMask >> 0) & 1))
			{
				SceneHelpers::RunIfDerivedPointerAs<Transform_Component>(components[i],
					[&](std::shared_ptr<Transform_Component> comp) {
						componentMask |= 1 << 0;
					});
			}
			if (!((componentMask >> 1) & 1))
			{
				SceneHelpers::RunIfDerivedPointerAs<Scene::Test_Component>(components[i],
					[&](std::shared_ptr<Scene::Test_Component> comp) {
						componentMask |= 1 << 1;
					});
			}
		}
		componentMasks[entity] = componentMask;
	}

	void Rendering::Update(Scene::Entity entity)
	{
		//std::printf("Transform Component Count: [%i]\n", SceneManager::GetCurrent()->GetComponents<Transform_Component>(entity).size());

		if (componentMasks[entity] == 3)
		{
			SceneManager::GetCurrent()->GetComponents<Transform_Component>(entity, 0)->Rotate(glm::vec3(0,0,1));
		}
	}
}