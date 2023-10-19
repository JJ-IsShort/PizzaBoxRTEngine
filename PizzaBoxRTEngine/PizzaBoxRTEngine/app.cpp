#include "app.h"

#include <functional>
#include <list>

// My header files
#include "Panels/Panel.h"
#include "Panels/Viewport.h"
#include "Panels/Inspector.h"
#include <unordered_map>
#include <typeindex>

namespace PBEngine
{
    // Vulkan Data
    VkAllocationCallbacks* App::g_Allocator = nullptr;
    VkInstance App::g_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice App::g_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice App::g_Device = VK_NULL_HANDLE;
    uint32_t App::g_QueueFamily[2]{ (int32_t)-1, (int32_t)-1 };
    VkQueue App::g_RenderQueue[2]{VK_NULL_HANDLE};
    VkQueue App::g_ComputeQueue = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT App::g_DebugReport = VK_NULL_HANDLE;
    VkPipelineCache App::g_PipelineCache = VK_NULL_HANDLE;
    VkDescriptorPool App::g_DescriptorPool = VK_NULL_HANDLE;

    ImGui_ImplVulkanH_Window App::g_MainWindowData;
    int App::g_MinImageCount = 2;
    bool App::g_SwapChainRebuild = false;

    Scene scene;

    void App::SetupImGuiStyle() {
        // Photoshop style by Derydoca from ImThemes
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.6000000238418579f;
        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.WindowRounding = 4.0f;
        style.WindowBorderSize = 1.0f;
        style.WindowMinSize = ImVec2(32.0f, 32.0f);
        style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ChildRounding = 4.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupRounding = 2.0f;
        style.PopupBorderSize = 1.0f;
        style.FramePadding = ImVec2(4.0f, 3.0f);
        style.FrameRounding = 2.0f;
        style.FrameBorderSize = 1.0f;
        style.ItemSpacing = ImVec2(8.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style.CellPadding = ImVec2(4.0f, 2.0f);
        style.IndentSpacing = 21.0f;
        style.ColumnsMinSpacing = 6.0f;
        style.ScrollbarSize = 13.0f;
        style.ScrollbarRounding = 12.0f;
        style.GrabMinSize = 7.0f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 0.0f;
        style.TabBorderSize = 1.0f;
        style.TabMinWidthForCloseButton = 0.0f;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

        style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(
            0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(
            0.1764705926179886f, 0.1764705926179886f, 0.1764705926179886f, 1.0f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(
            0.2784313857555389f, 0.2784313857555389f, 0.2784313857555389f, 0.0f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(
            0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
        style.Colors[ImGuiCol_Border] = ImVec4(
            0.2627451121807098f, 0.2627451121807098f, 0.2627451121807098f, 1.0f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(
            0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(
            0.2000000029802322f, 0.2000000029802322f, 0.2000000029802322f, 1.0f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(
            0.2784313857555389f, 0.2784313857555389f, 0.2784313857555389f, 1.0f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(
            0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(
            0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(
            0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(
            0.1921568661928177f, 0.1921568661928177f, 0.1921568661928177f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(
            0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(
            0.2745098173618317f, 0.2745098173618317f, 0.2745098173618317f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(
            0.2980392277240753f, 0.2980392277240753f, 0.2980392277240753f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] =
            ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(
            0.3882353007793427f, 0.3882353007793427f, 0.3882353007793427f, 1.0f);
        style.Colors[ImGuiCol_SliderGrabActive] =
            ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
        style.Colors[ImGuiCol_ButtonHovered] =
            ImVec4(1.0f, 1.0f, 1.0f, 0.1560000032186508f);
        style.Colors[ImGuiCol_ButtonActive] =
            ImVec4(1.0f, 1.0f, 1.0f, 0.3910000026226044f);
        style.Colors[ImGuiCol_Header] = ImVec4(
            0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(
            0.4666666686534882f, 0.4666666686534882f, 0.4666666686534882f, 1.0f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(
            0.4666666686534882f, 0.4666666686534882f, 0.4666666686534882f, 1.0f);
        style.Colors[ImGuiCol_Separator] = ImVec4(
            0.2627451121807098f, 0.2627451121807098f, 0.2627451121807098f, 1.0f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(
            0.3882353007793427f, 0.3882353007793427f, 0.3882353007793427f, 1.0f);
        style.Colors[ImGuiCol_SeparatorActive] =
            ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.0f, 1.0f, 1.0f, 0.25f);
        style.Colors[ImGuiCol_ResizeGripHovered] =
            ImVec4(1.0f, 1.0f, 1.0f, 0.6700000166893005f);
        style.Colors[ImGuiCol_ResizeGripActive] =
            ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_Tab] = ImVec4(
            0.09411764889955521f, 0.09411764889955521f, 0.09411764889955521f, 1.0f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(
            0.3490196168422699f, 0.3490196168422699f, 0.3490196168422699f, 1.0f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(
            0.1921568661928177f, 0.1921568661928177f, 0.1921568661928177f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(
            0.09411764889955521f, 0.09411764889955521f, 0.09411764889955521f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(
            0.1921568661928177f, 0.1921568661928177f, 0.1921568661928177f, 1.0f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(
            0.4666666686534882f, 0.4666666686534882f, 0.4666666686534882f, 1.0f);
        style.Colors[ImGuiCol_PlotLinesHovered] =
            ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(
            0.5843137502670288f, 0.5843137502670288f, 0.5843137502670288f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogramHovered] =
            ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(
            0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(
            0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(
            0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        style.Colors[ImGuiCol_TableRowBgAlt] =
            ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
        style.Colors[ImGuiCol_TextSelectedBg] =
            ImVec4(1.0f, 1.0f, 1.0f, 0.1560000032186508f);
        style.Colors[ImGuiCol_DragDropTarget] =
            ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_NavHighlight] =
            ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_NavWindowingHighlight] =
            ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_NavWindowingDimBg] =
            ImVec4(0.0f, 0.0f, 0.0f, 0.5860000252723694f);
        style.Colors[ImGuiCol_ModalWindowDimBg] =
            ImVec4(0.0f, 0.0f, 0.0f, 0.5860000252723694f);
    }

    PFN_vkVoidFunction IMGuiLoaderFunction(const char* function_name, void*)
    {
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetInstanceProcAddr(nullptr, function_name));
    }

	int App::Start()
	{
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return 1;

        // Create window with Vulkan context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(
            1280, 720, "Pizza Box Ray Tracing Engine", nullptr, nullptr);
        if (!glfwVulkanSupported()) {
            printf("GLFW: Vulkan Not Supported\n");
            return 1;
        }

        ImVector<const char*> extensions;
        uint32_t extensions_count = 0;
        const char** glfw_extensions =
            glfwGetRequiredInstanceExtensions(&extensions_count);
        for (uint32_t i = 0; i < extensions_count; i++)
            extensions.push_back(glfw_extensions[i]);
        //extensions.push_back(
        //    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        //extensions.push_back(
        //    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        SetupVulkan(extensions);

        {
            bool func_loaded = ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void*) { return vkGetInstanceProcAddr(g_Instance, function_name); }, nullptr);
            if (!func_loaded)
                std::cerr << "ImGUI vulkan functions not loaded properly." << std::endl;
        }

        // Create Window Surface
        VkSurfaceKHR surface;
        VkResult err =
            glfwCreateWindowSurface(g_Instance, window, g_Allocator, &surface);
        check_vk_result(err);

        // Create Framebuffers
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
        SetupVulkanWindow(wd, surface, w, h);

        // Initialize ImGUI
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable viewports
        io.ConfigDockingWithShift = true;
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
        // std::cout << (io.BackendFlags & ImGuiBackendFlags_PlatformHasViewports) <<
        // std::endl; std::cout << (io.BackendFlags &
        // ImGuiBackendFlags_RendererHasViewports) << std::endl;
        ImGuiPlatformIO platformIO = ImGui::GetPlatformIO();

        SetupImGuiStyle();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = g_Instance;
        init_info.PhysicalDevice = g_PhysicalDevice;
        init_info.Device = g_Device;
        init_info.QueueFamily = g_QueueFamily[0];
        init_info.Queue = g_RenderQueue[0];
        init_info.PipelineCache = g_PipelineCache;
        init_info.DescriptorPool = g_DescriptorPool;
        init_info.Subpass = 0;
        init_info.MinImageCount = g_MinImageCount;
        init_info.ImageCount = wd->ImageCount;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = g_Allocator;
        init_info.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);

        // Upload Fonts
        {
            // Use any command queue
            VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;
            VkCommandBuffer command_buffer = wd->Frames[wd->FrameIndex].CommandBuffer;

            err = vkResetCommandPool(g_Device, command_pool, 0);
            check_vk_result(err);
            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            err = vkBeginCommandBuffer(command_buffer, &begin_info);
            check_vk_result(err);

            ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

            VkSubmitInfo end_info = {};
            end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            end_info.commandBufferCount = 1;
            end_info.pCommandBuffers = &command_buffer;
            err = vkEndCommandBuffer(command_buffer);
            check_vk_result(err);
            err = vkQueueSubmit(g_RenderQueue[0], 1, &end_info, VK_NULL_HANDLE);
            check_vk_result(err);

            err = vkDeviceWaitIdle(g_Device);
            check_vk_result(err);
            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }

        std::list<std::unique_ptr<Panel>> panels;

        std::unique_ptr<Viewport> viewport = std::make_unique<Viewport>();
        std::unique_ptr<Inspector> inspector = std::make_unique<Inspector>();
        panels.push_back(std::move(viewport));
        panels.push_back(std::move(inspector));

        std::unordered_map<std::type_index, std::shared_ptr<void>> test;

        for (auto& panelPtr : panels) {
            Panel& panel = *panelPtr;
            panel.Init();
        }

        // Main loop
        while (!glfwWindowShouldClose(window)) {
            // Poll events
            glfwPollEvents();

            for (auto& panelPtr : panels) {
                Panel& panel = *panelPtr;
                panel.PreRender();
            }

            // Resize swap chain?
            if (g_SwapChainRebuild) {
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);
                if (width > 0 && height > 0) {
                    ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
                    ImGui_ImplVulkanH_CreateOrResizeWindow(
                        g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData,
                        g_QueueFamily[0], g_Allocator, width, height, g_MinImageCount);
                    g_MainWindowData.FrameIndex = 0;
                    g_SwapChainRebuild = false;
                }
            }

            // Start the Dear ImGui frame
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

#pragma region Workspace Setup
            ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            ImVec2 window_pos =
                ImVec2((float)main_viewport->Pos.x, (float)main_viewport->Pos.y);
            ImVec2 window_size =
                ImVec2((float)main_viewport->Size.x, (float)main_viewport->Size.y);

            ImGui::SetNextWindowPos(window_pos);
            ImGui::SetNextWindowSize(window_size);
            ImGui::SetNextWindowViewport(main_viewport->ID);

            ImGuiWindowFlags window_flags =
                ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            if (ImGui::Begin("Full Screen Window", nullptr, window_flags)) {
                ImGui::PopStyleVar(3);
                // Window menu bar
                if (ImGui::BeginMenuBar()) {
                    if (ImGui::BeginMenu("File")) {
                        ImGui::MenuItem("New", nullptr);
                        ImGui::MenuItem("Open", nullptr);
                        ImGui::MenuItem("Save", nullptr);
                        ImGui::Separator();
                        if (ImGui::MenuItem("Exit"))
                            glfwSetWindowShouldClose(window, true);
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }

                // Dockspace
                ImGui::DockSpace(ImGui::GetID("FullViewportDockSpace"),
                    ImVec2(0.0f, 0.0f));
            }
            ImGui::End();
#pragma endregion

            // ImGUI code goes here
            ImGui::ShowStackToolWindow();
            for (auto& panelPtr : panels) {
                Panel& panel = *panelPtr;
                panel.Show();
            }
            ImGui::ShowDemoWindow();

            // Rendering
            ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
            ImGui::Render();
            ImDrawData* main_draw_data = ImGui::GetDrawData();
            const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f ||
                main_draw_data->DisplaySize.y <= 0.0f);
            wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
            wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
            wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
            wd->ClearValue.color.float32[3] = clear_color.w;
            if (!main_is_minimized)
                FrameRender(wd, main_draw_data);

            // Update and Render additional Platform Windows
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            // Present Main Platform Window
            if (!main_is_minimized)
                FramePresent(wd);
        }

        // Cleanup
        err = vkDeviceWaitIdle(g_Device);
        check_vk_result(err);

        // So I'm injecting some code here
        panels.clear();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        CleanupVulkanWindow();
        CleanupVulkan();

        glfwDestroyWindow(window);
        glfwTerminate();

        return 0;
    }
}
