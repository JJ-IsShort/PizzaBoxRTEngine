#include "Scene.h"

namespace PBEngine
{
	Scene::Scene()
	{

	}

	Scene::~Scene()
	{

	}

    std::vector<std::shared_ptr<Scene::Component>> Scene::GetComponents(Entity entity)
    {
        std::vector<std::shared_ptr<Component>> output;
        for (const auto& element : componentManagers)
        {
            // Call GetAllComponents on the appropriate ComponentManager
            std::vector<std::shared_ptr<Component>> components = element.second->GetAllComponents(entity);

            // Append the components to the result vector
            output.insert(output.end(), components.begin(), components.end());
        }
        return output;
    }

    void Scene::Update()
    {
        for (auto& service : services)
        {
            for (size_t i = 0; i < managedEntities.EntityCount(); i++)
            {
                service->Update(*managedEntities.GetEntityAtID(i));
            }
        }
        /*std::for_each(services.begin(), services.end(),
            [&](std::shared_ptr<Service> service){
                for (size_t i = 0; i < managedEntities.EntityCount(); i++)
                {
                    service->Update(*managedEntities.GetEntityAtID(i));
                }
            });*/
    }
}