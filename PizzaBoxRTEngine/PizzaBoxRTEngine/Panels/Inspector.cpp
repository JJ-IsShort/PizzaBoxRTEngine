#include "Inspector.h"
#include <Scene/Scene.h>
#include <glm/vec3.hpp>

#include <format>

namespace PBEngine
{
	extern Scene scene;

	class Transform_Component : public Scene::Component
	{
	public:
		glm::vec3 position;
		glm::vec3 scale;
		glm::vec3 rotate;

		void Show() override
		{
			ImGuiStyle style = ImGui::GetStyle();
			ImVec2 spacing = style.ItemSpacing;
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
			auto elementSize = [contentRegionAvailable, spacing](int elementCount) { return ImVec2((contentRegionAvailable.x * (1.0f / elementCount)) -
				(spacing.x * 0.5f) * (elementCount - 1), 0); };
			ImGui::DragFloat3("Position", (float*)&position);
			ImGui::DragFloat3("Scale", (float*)&scale);
			ImGui::DragFloat3("Rotate", (float*)&rotate);
		}
	};

	std::shared_ptr<Scene::Entity> selectedEntity;
	std::vector<std::shared_ptr<Scene::Component>> selectedComponents;
	uint32_t selectedEntityIndex = UINT32_MAX;

	void Inspector::Show()
	{
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
					selectedEntity = scene.GetEntityAtID(selectedEntityIndex);
					selectedComponents = scene.GetComponents(*selectedEntity);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Next Entity", buttonSize(2)))
			{
				selectedEntityIndex = std::min(selectedEntityIndex + 1, scene.EntityCount() - 1);
				selectedEntity = scene.GetEntityAtID(selectedEntityIndex);
				selectedComponents = scene.GetComponents(*selectedEntity);
			}

			ImGui::SeparatorText("Components");
			for (size_t i = 0; i < selectedComponents.size(); i++)
			{
				if (ImGui::TreeNode(std::vformat("{} {}", std::make_format_args("Component", i)).c_str()))
				{
					selectedComponents[i]->Show();
					ImGui::TreePop();
				}
			}
		}

		if (ImGui::Button("Add Test Entity", buttonSize(2)))
		{
			selectedEntity = scene.AddEntity();
			selectedEntityIndex = selectedEntity->id;
			selectedComponents = scene.GetComponents(*selectedEntity);
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Test Component", buttonSize(2)))
		{
			scene.AddComponent<Transform_Component>(*selectedEntity);
			selectedComponents = scene.GetComponents(*selectedEntity);
		}

		ImGui::End();
	}
}