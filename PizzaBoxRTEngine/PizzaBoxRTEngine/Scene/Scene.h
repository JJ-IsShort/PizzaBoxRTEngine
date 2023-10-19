#pragma once

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <memory>
#include <functional>
#include <variant>

namespace PBEngine
{
	class Scene
	{
	public:
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
            Component() { Construt(); };
            virtual void Construt() {};
            virtual void Show() {};
            virtual void Start() {};
            virtual void Update() {};
            virtual ~Component() {};

        private:

        };

        class Test_Component : Component
        {
            float foo;
            int ba;
        };

        template <typename T>
        std::shared_ptr<T> AddComponent(Entity entity)
        {
            auto manager = GetComponentManager<T>();
            if (manager) {
                return manager->CreateComponent(entity);
            }
            else {
                // Component manager for the given type doesn't exist.
                // You can handle this case by creating the manager or throwing an error.
                // For simplicity, let's create one if it doesn't exist.
                auto newManager = std::make_shared<ComponentManager<T>>();
                ComponentManagers[std::type_index(typeid(T))] = std::dynamic_pointer_cast<BaseComponentManager> (newManager);
                return newManager->CreateComponent(entity);
            }
        }

        std::vector<std::shared_ptr<Component>> GetComponents(Entity entity)
        {
            std::vector<std::shared_ptr<Component>> output;
            for (const auto& element : ComponentManagers)
            {
                //std::shared_ptr<ComponentManager<void>> manager = std::dynamic_pointer_cast<ComponentManager<void>>(element.second);

                // Call GetAllComponents on the appropriate ComponentManager
                std::vector<std::shared_ptr<Component>> components = element.second->GetAllComponents(entity);

                // Append the components to the result vector
                output.insert(output.end(), components.begin(), components.end());
            }
            return output;
        }

        /*std::vector<std::shared_ptr<Component>> GetAllComponentsForEntity(Entity entity) {
            std::vector<std::shared_ptr<Component>> resultComponents;

            for (const auto& [typeIndex, manager] : ComponentManagers) {
                // Since each manager is stored as a shared_ptr<void>, we'll need to cast it back.
                // But we don't know the exact type T, so we employ some type-erasure techniques.

                auto findFunc = [entity](const auto& managerPtr) -> std::shared_ptr<Component> {
                    using ManagerType = std::decay_t<decltype(*managerPtr)>;
                    using ComponentType = typename ManagerType::ComponentType;

                    return std::static_pointer_cast<Component>(managerPtr->getComponent(entity));
                };

                auto component = std::visit(findFunc, manager);
                if (component) {
                    resultComponents.push_back(component);
                }
            }

            return resultComponents;
        }*/

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

        class BaseComponentManager {
        public:
            virtual ~BaseComponentManager() = default;
            
            // Get all components of a specific type associated with an entity
            virtual std::vector<std::shared_ptr<Component>> GetAllComponents(Entity entity) = 0;
        };

        using ComponentID = uint32_t;

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

        // Define a map to store component managers for different component types.
        std::unordered_map<std::type_index, std::shared_ptr<BaseComponentManager>> ComponentManagers;

        template <typename T>
        std::shared_ptr<ComponentManager<T>> GetComponentManager() {
            auto typeIndex = std::type_index(typeid(T));
            if (ComponentManagers.find(typeIndex) != ComponentManagers.end()) {
                return std::dynamic_pointer_cast<ComponentManager<T>>(ComponentManagers[typeIndex]);
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
