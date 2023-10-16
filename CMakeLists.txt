cmake_minimum_required(VERSION 3.7)

project(PizzaBox)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(IMGUI_VULKAN_DEBUG_REPORT 1)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:4096")

#set(GLFW_BUILD_X11 1)
#set(GLFW_BUILD_WAYLAND 1)

set(IMGUI_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui_demo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui_tables.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/backends/imgui_impl_glfw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/backends/imgui_impl_vulkan.cpp"
)

set(ENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/PizzaBoxRTEngine/PizzaBoxRTEngine/Panels/Viewport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PizzaBoxRTEngine/PizzaBoxRTEngine/Panels/Panel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PizzaBoxRTEngine/PizzaBoxRTEngine/Rendering/Renderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PizzaBoxRTEngine/PizzaBoxRTEngine/Rendering/RenderData/AccelerationStructure.cpp"
    #"${CMAKE_CURRENT_SOURCE_DIR}/PizzaBoxRTEngine/PizzaBoxRTEngine/VulkanHelp/Context.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PizzaBoxRTEngine/PizzaBoxRTEngine/app.cpp"
 "PizzaBoxRTEngine/PizzaBoxRTEngine/VulkanHelp/vk_common.h")

add_executable(PizzaBox
    "${CMAKE_CURRENT_SOURCE_DIR}/PizzaBoxRTEngine/PizzaBoxRTEngine/main.cpp"
    ${IMGUI_SOURCES}
    ${ENGINE_SOURCES}
 )

include_directories(PizzaBox PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/PizzaBoxRTEngine/PizzaBoxRTEngine"
    "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/External/glm/glm"
)

# Volk Linking
if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/External/volk")
target_link_libraries(PizzaBox PRIVATE volk)

# Vulkan Linking
find_package(Vulkan REQUIRED)
target_link_libraries(PizzaBox PRIVATE Vulkan::Vulkan)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Works on Linux
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/External/glfw")
target_link_libraries(PizzaBox PRIVATE glfw)

# find_package(glfw3 REQUIRED)
# target_link_libraries(PizzaBox PRIVATE glfw)

# Works on Linux
# find_package(glm REQUIRED)
# target_link_libraries(PizzaBox PRIVATE glm::glm)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/External/glm")
target_link_libraries(PizzaBox PUBLIC glm::glm)

# Define C++ version to be used for building the project
set_property(TARGET ${Recipe_Name} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${Recipe_Name} PROPERTY CXX_STANDARD_REQUIRED ON)
