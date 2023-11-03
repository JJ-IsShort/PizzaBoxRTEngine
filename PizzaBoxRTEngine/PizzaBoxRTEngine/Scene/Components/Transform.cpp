#include "Transform.h"
#include <imgui.h>
#include <glm/gtc/quaternion.hpp>
#include <Scene/SceneManagement.h>

namespace PBEngine
{
	void Transform_Component::Show()
	{
		ImGuiStyle style = ImGui::GetStyle();
		ImVec2 spacing = style.ItemSpacing;
		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
		auto elementSize = [contentRegionAvailable, spacing](int elementCount) { return ImVec2((contentRegionAvailable.x * (1.0f / elementCount)) -
			(spacing.x * 0.5f) * (elementCount - 1), 0); };
		ImGui::DragFloat3("Position", (float*)&position, 0.1f);
		ImGui::DragFloat3("Scale", (float*)&scale, 0.1f);

		glm::vec3 eulerAngles = glm::eulerAngles(rotation);
		eulerAngles = glm::degrees(eulerAngles);
#if 1
		if (ImGui::DragFloat3("Rotation", (float*)&eulerAngles, 0.1f, -180, 180))
			rotation = glm::quat(glm::radians(eulerAngles));
#else
		if (ImGui::SliderFloat("Pitch", &eulerAngles.x, -PI, PI) ||
			ImGui::SliderFloat("Yaw", &eulerAngles.y, -PI, PI) ||
			ImGui::SliderFloat("Roll", &eulerAngles.z, -PI, PI))
		{
			rotation = glm::quat(eulerAngles);
		}
#endif
	}

	/*std::string Transform_Component::GetName(Scene::Entity entity)
	{
		if (scene.GetComponents<Transform_Component>(entity, 1))
		{
			return "Transform (Duplicate)";
		}
		return "Transform";
	}*/

	void Transform_Component::Construct(Scene::Entity entity)
	{
		componentName = "Transform";
		{
			if (SceneManager::GetCurrent()->GetComponents<Transform_Component>(entity).size() > 0)
			{
				componentName = "Transform (Extra)";
			}
		}
	}

	void Transform_Component::Rotate(glm::vec3 euler)
	{
		glm::vec3 eulerAngles = glm::eulerAngles(rotation);
		rotation = glm::quat(eulerAngles + glm::degrees(euler));
	}
}