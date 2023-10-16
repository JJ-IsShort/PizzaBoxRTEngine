#pragma once
#define VK_NO_PROTOTYPES
#include "app.h"
#include <glm/mat4x4.hpp>
#include "RenderData/AccelerationStructure.h"
#include "RenderData/TLAS.h"

namespace PBEngine
{
    class Backend {
    public:
        enum RendererBackendType {
            RendererBackendType_None,
            RendererBackendType_FullRT,
            RendererBackendType_Custom
        };
        virtual ~Backend();
        virtual bool Init(float *width, float *height);
        virtual bool Render();
        virtual bool CleanupBackend();
        const RendererBackendType backendType = RendererBackendType_None;
    };

    class Backend_FullRT : public Backend {
    public:
        ~Backend_FullRT() override;
        bool Init(float *width, float *height) override;
        bool ResizeViewImage();
        bool Render() override;
        bool CleanupBackend() override;
        const RendererBackendType backendType = RendererBackendType_FullRT;

        float *viewportWidth;
        float *viewportHeight;

        VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR  ray_tracing_pipeline_properties{};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups{};

        std::unique_ptr<Buffer> raygen_shader_binding_table;
        std::unique_ptr<Buffer> miss_shader_binding_table;
        std::unique_ptr<Buffer> hit_shader_binding_table;

        struct StorageImage
        {
            VkDeviceMemory memory;
            VkImage        image = VK_NULL_HANDLE;
            VkImageView    view;
            VkFormat       format;
            uint32_t       width;
            uint32_t       height;
        } storage_image;

        struct ViewImage
        {
            VkDeviceMemory memory;
            VkImage        image = VK_NULL_HANDLE;
            VkImageView    view;
            VkFormat       format;
            uint32_t       width;
            uint32_t       height;
            VkSampler      sampler;
        } viewImage;

        struct UniformData
        {
            glm::mat4 view_inverse;
            glm::mat4 proj_inverse;
        } uniform_data;
        std::unique_ptr<Buffer> ubo;

        VkPipeline            pipeline;
        VkPipelineLayout      pipeline_layout;
        VkDescriptorSet       descriptor_set;
        VkDescriptorSetLayout descriptor_set_layout;

        std::unique_ptr<TLAS> scene;
        std::vector<VkShaderModule> shaderModules;

        // Command buffers and pool used for rendering
        std::vector<VkCommandBuffer> draw_cmd_buffers;
        std::vector<VkFence> drawBuffersFences;
        VkCommandPool cmd_pool;

        uint16_t displayImage = UINT16_MAX;

    private:
        /*
            Set up a storage image that the ray generation shader will be writing to
        */
        void CreateStorageImage();

        /*
            Set up a view image that will be shown to the user
        */
        void CreateViewImage(bool resize);

        /*
            Create our ray tracing pipeline
        */
        void CreateRayTracingPipeline();

        /*
            Create the Shader Binding Tables that connects the ray tracing pipelines' programs and the  top-level acceleration structure

            SBT Layout used in this sample:

                /-----------\
                | raygen    |
                |-----------|
                | miss      |
                |-----------|
                | hit       |
                \-----------/
        */
        void CreateShaderBindingTables();

        /*
            Create the descriptor sets used for the ray tracing dispatch
        */
        void CreateDescriptorSets();

        /*
            Command buffer generation
        */
        void BuildCommandBuffers();

        void CreateCommandPool();
        void DestroyDrawBuffers();
    };

    class Renderer {
    public:
        Renderer();
        Renderer(float *width, float *height);
        ~Renderer();
        std::unique_ptr<Backend> renderingBackend; // Don't forget to keep an eye on the memory for
        // this. A memory leak here probably wouldn't be
        // too bad but still.
    };
}
