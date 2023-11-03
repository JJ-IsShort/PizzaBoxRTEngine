#pragma once

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <memory>
#include <functional>
//#include <variant>
#include <algorithm>
#include <string>
#include "../../../External/imgui/imgui.h"

namespace PBEngine
{
	class Scene
	{
	public:
        using ComponentID = uint32_t;

		Scene();
		~Scene();

		struct Entity {
			int id; // Use the index as the ID.
			int version; // To check for stale entities.
            
            bool operator==(const Entity& other) const
            {
                if (other.id == id && other.version == version)
                    return true;
                return false;
            }
		};

        class Component
        {
        public:
            std::string componentName = "Base Component";

            Component() {};
            virtual void Construct(Entity entity) {};
            //virtual std::string GetName(Entity entity) { return "Component"; };
            virtual void Show() {};
            virtual void Start() {};
            virtual void Update() {};
            virtual ~Component() {};

        private:

        };

        class Test_Component : public Component
        {
            int foo;
            bool bar;

            void Show() override
            {
                ImGui::SliderInt("Foo", &foo, -50, 50);
                ImGui::Checkbox("Bar", &bar);
            }
        };

        class Service
        {
        public:
            Service() = default;
            virtual void Construct() {};
            virtual void Update(Entity entity) {};
            virtual void ComponentAdded(Entity entity) {};
            virtual ~Service() = default;

            int id;
            int version;
        };

        template <typename T>
        std::shared_ptr<T> AddComponent(Entity entity)
        {
            auto manager = GetComponentManager<T>();
            if (manager) {
                std::shared_ptr<T> component = manager->CreateComponent(entity);
                for (size_t i = 0; i < services.size(); i++)
                {
                    services[i]->ComponentAdded(entity);
                }
                return component;
            }
            else {
                // Component manager for the given type doesn't exist.
                // You can handle this case by creating the manager or throwing an error.
                // For simplicity, let's create one if it doesn't exist.
                auto newManager = std::make_shared<ComponentManager<T>>();
                componentManagers[std::type_index(typeid(T))] = std::dynamic_pointer_cast<BaseComponentManager>(newManager);
                std::shared_ptr<T> component = newManager->CreateComponent(entity);
                for (size_t i = 0; i < services.size(); i++)
                {
                    services[i]->ComponentAdded(entity);
                }
                return component;
            }
        }

        std::vector<std::shared_ptr<Component>> GetComponents(Entity entity);

        template <typename T>
        std::shared_ptr<T> AddService()
        {
            std::shared_ptr<T> newService = std::make_shared<T>();
            std::shared_ptr<Service> baseService = std::dynamic_pointer_cast<Service>(newService);
            services.resize(services.size() + 1);
            services[services.size() - 1] = baseService;
            baseService->Construct();
            return newService;
        }

        void Update();

        uint32_t EntityCount()
        {
            return managedEntities.EntityCount();
        }

        std::shared_ptr<Entity> AddEntity()
        {
            return managedEntities.CreateEntity();
        }

        std::shared_ptr<Entity> GetEntityAtID(int id)
        {
            return managedEntities.GetEntityAtID(id);
        }

        std::vector<std::shared_ptr<Entity>> GetAllEntities()
        {
            return managedEntities.GetAllEntities();
        }
        
        template <typename T>
        std::shared_ptr<T> GetComponents(Entity entity, ComponentID index)
        {
            return GetComponentManager<T>()->GetComponent(entity, index);
        }

        // This is a version of GetComponents<T>(Scene::Entity entity)
        // that lets you use multiple comma seperated T values and returns
        // them in that order.
        template <typename FirstT, typename... RestT>
        std::vector<std::shared_ptr<Component>> GetComponents(Entity entity)
        {
            std::vector<std::shared_ptr<Component>> result;
            std::vector<std::shared_ptr<FirstT>> thisType = GetComponentsOneT<FirstT>(entity);
            for (size_t i = 0; i < thisType.size(); i++)
            {
                result.push_back(std::dynamic_pointer_cast<Component>(thisType[i]));
            }
            if constexpr (sizeof...(RestT) > 0) {
                std::vector<std::shared_ptr<Component>> nextType = GetComponents<RestT...>(entity);
                /*for (size_t i = 0; i < nextType.size(); i++)
                {
                    result.push_back(std::dynamic_pointer_cast<Component>(nextType[i]));
                }*/
                result.insert(result.end(), nextType.begin(), nextType.end());
            }
            return result;
        }

    private:
		class EntityManager
		{
		public:
            std::shared_ptr<Entity> CreateEntity() {
                std::shared_ptr<Entity> entity = std::make_unique<Entity>();
                if (freeIndices.empty()) {
                    // If there are no free indices, create a new entity.
                    entity->id = entities.size();
                    entity->version = 0;
                    entities.push_back(entity);
                }
                else {
                    // Reuse a previously freed index.
                    int index = freeIndices.back();
                    freeIndices.pop_back();
                    entity = entities[index];
                }
                return entity;
            }

            std::shared_ptr<Entity> GetEntityAtID(int id)
            {
                return entities[id];
            }

            std::vector<std::shared_ptr<Entity>> GetAllEntities()
            {
                return entities;
            }

            uint32_t EntityCount()
            {
                return entities.size();
            }

            void DestroyEntity(Entity entity) {
                if (entity.id < entities.size() && entities[entity.id]->version == entity.version) {
                    // Mark the entity as freed and increment its version.
                    freeIndices.push_back(entity.id);
                    entities[entity.id]->version++;
                }
            }

		private:
			std::vector<std::shared_ptr<Entity>> entities;
			std::vector<int> freeIndices;
		};

        /*class ServiceManager
        {
        public:
            template <typename T>
            std::shared_ptr<T> CreateService() {
                std::shared_ptr<T> newService = std::make_unique<T>();
                std::shared_ptr<Service> baseService = std::dynamic_pointer_cast<Service>(newService);
                if (freeIndices.empty()) {
                    // If there are no free indices, create a new entity.
                    newService->id = services.size();
                    newService->version = 0;
                    services.push_back(baseService);
                }
                else {
                    // Reuse a previously freed index.
                    int index = freeIndices.back();
                    freeIndices.pop_back();
                    baseService = entities[index];
                }
                return entity;
            }

            std::shared_ptr<Entity> GetEntityAtID(int id)
            {
                return entities[id];
            }

            std::vector<std::shared_ptr<Entity>> GetAllEntities()
            {
                return entities;
            }

            uint32_t EntityCount()
            {
                return entities.size();
            }

            void DestroyEntity(Entity entity) {
                if (entity.id < entities.size() && entities[entity.id]->version == entity.version) {
                    // Mark the entity as freed and increment its version.
                    freeIndices.push_back(entity.id);
                    entities[entity.id]->version++;
                }
            }

        private:
            std::vector<std::shared_ptr<Service>> services;
            std::vector<int> freeIndices;
        };*/

        class BaseComponentManager {
        public:
            virtual ~BaseComponentManager() = default;
            
            // Get all components of a specific type associated with an entity
            virtual std::vector<std::shared_ptr<Component>> GetAllComponents(Entity entity) = 0;
        };

        template <typename T>
        class ComponentManager : public BaseComponentManager {
        private:
            //static_assert(std::is_base_of<Component, T>::value, "T must inherit from BaseComponent");
            // Components are stored in an unordered map, where the key is the EntityID
            std::unordered_map<Entity, std::vector<std::shared_ptr<T>>> components;
            // Keep a separate ID for each component
            ComponentID nextComponentID = 0;

        public:
            // Create a new component for a given entity and return a shared pointer to it
            std::shared_ptr<T> CreateComponent(Entity entity) {
                std::shared_ptr<T> component = std::make_shared<T>();
                component->Construct(entity);
                components[entity].emplace_back(component);
                return component;
            }

            // Remove a specific component from an entity
            void RemoveComponent(Entity entity, std::shared_ptr<T> component) {
                if (components.find(entity) != components.end()) {
                    auto& entityComponents = components[entity];
                    auto it = std::remove(entityComponents.begin(), entityComponents.end(), component);
                    if (it != entityComponents.end()) {
                        entityComponents.erase(it);
                    }
                }
            }

            // Remove all components of a specific type from an entity
            void RemoveAllComponents(Entity entity) {
                components.erase(entity);
            }

            // Get a specific component associated with an entity
            std::shared_ptr<T> GetComponent(Entity entity, ComponentID componentID) {
                if (components.find(entity) != components.end() && componentID < components[entity].size()) {
                    return components[entity][componentID];
                }
                return nullptr;
            }

            // Get all components of a specific type associated with an entity
            std::vector<std::shared_ptr<T>> GetAllComponentsAsT(Entity entity) {
                if (components.find(entity) != components.end()) {
                    return components[entity];
                }
                return std::vector<std::shared_ptr<T>>();
            }

            // Get all components of a specific type associated with an entity
            std::vector<std::shared_ptr<Component>> GetAllComponents(Entity entity) override {
                std::vector<std::shared_ptr<Component>> result = std::vector<std::shared_ptr<Component>>();
                if (components.find(entity) != components.end()) {
                    //return dynamic_cast<std::vector<std::shared_ptr<Component>>>(components[entity]);
                    for (size_t i = 0; i < components[entity].size(); i++)
                    {
                        result.push_back(std::dynamic_pointer_cast<Component>(components[entity][i]));
                    }
                }
                return result;
            }
        };

        EntityManager managedEntities;

        // A map to store component managers for different component types.
        std::unordered_map<std::type_index, std::shared_ptr<BaseComponentManager>> componentManagers;

        public: std::vector<std::shared_ptr<Service>> services; private:

        template <typename T>
        std::vector<std::shared_ptr<T>> GetComponentsOneT(Entity entity)
        {
            std::shared_ptr<ComponentManager<T>> manager = GetComponentManager<T>();
            if (manager)
                return manager->GetAllComponentsAsT(entity);
            return std::vector<std::shared_ptr<T>>();
        }

        template <typename T>
        std::shared_ptr<ComponentManager<T>> GetComponentManager() {
            auto typeIndex = std::type_index(typeid(T));
            if (componentManagers.find(typeIndex) != componentManagers.end()) {
                return std::dynamic_pointer_cast<ComponentManager<T>>(componentManagers[typeIndex]);
            }
            return nullptr;
        }
	};
}

namespace std {
    template<>
    struct hash<PBEngine::Scene::Entity> {
        std::size_t operator()(const PBEngine::Scene::Entity& entity) const {
            // Hashing logic here
            return std::hash<int>()(entity.id) ^ (std::hash<int>()(entity.version) << 1);
        }
    };
}
