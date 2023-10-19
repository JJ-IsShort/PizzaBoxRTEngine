#include "Viewport.h"
//#include "Panel.h"

namespace PBEngine
{
	VkDescriptorSet ImageDS;

	Viewport::Viewport() {}

	void Viewport::Show() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Viewport");

		height = ImGui::GetContentRegionAvail().x;
		width = ImGui::GetContentRegionAvail().y;

		if (renderer.get())
		{
			Backend_FullRT* derivedRenderer = dynamic_cast<Backend_FullRT*>(renderer.get()->renderingBackend.get());
			if (derivedRenderer != nullptr)
			{
				derivedRenderer->Render();
				if (derivedRenderer->draw_cmd_buffers.size() != 0)
				{
					ImGui::Image((ImTextureID)ImageDS, ImVec2(derivedRenderer->storage_image.height,
						derivedRenderer->storage_image.width));
				}
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();

		if (!renderer)
		{
			renderer = std::make_unique<Renderer>(&width, &height);
			Backend_FullRT* derivedRenderer = dynamic_cast<Backend_FullRT*>(renderer.get()->renderingBackend.get());
			if (derivedRenderer != nullptr)
			{
				ImageDS = ImGui_ImplVulkan_AddTexture(derivedRenderer->viewImage.sampler, derivedRenderer->viewImage.view,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}
	}

	void Viewport::PreRender()
	{
		if (renderer)
		{
			Backend_FullRT* derivedRenderer = dynamic_cast<Backend_FullRT*>(renderer.get()->renderingBackend.get());
			if (derivedRenderer != nullptr)
			{
				vkQueueWaitIdle(GetRenderQueue());
				ImGui_ImplVulkan_RemoveTexture(ImageDS);
				derivedRenderer->ResizeViewImage();
				ImageDS = ImGui_ImplVulkan_AddTexture(derivedRenderer->viewImage.sampler, derivedRenderer->viewImage.view,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}
	}
	
	void Viewport::Init() {

	}

	Viewport::~Viewport() {}
}
