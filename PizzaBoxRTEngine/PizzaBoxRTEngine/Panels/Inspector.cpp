#include "Inspector.h"
#include <Scene/SceneManagement.h>
#include <glm/vec3.hpp>

#include <format>
#include <Scene/Components/Transform.h>
#include <Scene/Services/Rendering.h>

namespace PBEngine
{
	std::shared_ptr<Scene::Entity> selectedEntity;
	std::vector<std::shared_ptr<Scene::Component>> selectedComponents;
	uint32_t selectedEntityIndex = UINT32_MAX;
	
	void Inspector::Init()
	{
		SceneManager::GetCurrent()->AddService<Rendering>();
	}

	void Inspector::Show()
	{
		std::shared_ptr<Scene> scene = SceneManager::GetCurrent();

		ImGui::Begin("Inspector");

		ImGuiStyle style = ImGui::GetStyle();
		ImVec2 spacing = style.ItemSpacing;
		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
		auto buttonSize = [contentRegionAvailable, spacing](int buttonCount) { return ImVec2((contentRegionAvailable.x * (1.0f / buttonCount)) - 
			(spacing.x * 0.5f) * (buttonCount - 1), 0); };

		if (selectedEntityIndex != UINT32_MAX)
		{
			if (ImGui::Button("Previous Entity", buttonSize(2)))
			{
				if (selectedEntityIndex != (uint32_t)0)
				{
					selectedEntityIndex = std::max(selectedEntityIndex - (uint32_t)1, (uint32_t)0);
					selectedEntity = scene->GetEntityAtID(selectedEntityIndex);
					selectedComponents = scene->GetComponents(*selectedEntity);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Next Entity", buttonSize(2)))
			{
				selectedEntityIndex = std::min(selectedEntityIndex + 1, scene->EntityCount() - 1);
				selectedEntity = scene->GetEntityAtID(selectedEntityIndex);
				selectedComponents = scene->GetComponents(*selectedEntity);
			}

			ImGui::SeparatorText("Components");
			for (size_t i = 0; i < selectedComponents.size(); i++)
			{
				std::string componentName = selectedComponents[i]->componentName;
				if (ImGui::TreeNode(std::vformat("Component {}: {}", std::make_format_args(i, componentName)).c_str()))
				{
					selectedComponents[i]->Show();
					ImGui::TreePop();
				}
			}
		}

		if (ImGui::Button("Add Entity", buttonSize(3)))
		{
			selectedEntity = scene->AddEntity();
			selectedEntityIndex = selectedEntity->id;
			selectedComponents = scene->GetComponents(*selectedEntity);
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Transform Component", buttonSize(3)))
		{
			scene->AddComponent<Transform_Component>(*selectedEntity);
			selectedComponents = scene->GetComponents(*selectedEntity);
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Test Component", buttonSize(3)))
		{
			scene->AddComponent<Scene::Test_Component>(*selectedEntity);
			selectedComponents = scene->GetComponents(*selectedEntity);
		}

		ImGui::End();
	}
}