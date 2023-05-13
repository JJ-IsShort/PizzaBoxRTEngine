cmake_minimum_required(VERSION 3.7)

project(PizzaBox)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/PizzaBoxRTEngine/PizzaBoxRTEngine/Viewport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PizzaBoxRTEngine/PizzaBoxRTEngine/Renderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PizzaBoxRTEngine/PizzaBoxRTEngine/Panel.cpp"
)

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

find_package(Vulkan REQUIRED)
target_link_libraries(PizzaBox PRIVATE Vulkan::Vulkan)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/External/glfw")

target_link_libraries(PizzaBox PRIVATE glfw)

# Define C++ version to be used for building the project
set_property(TARGET ${Recipe_Name} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${Recipe_Name} PROPERTY CXX_STANDARD_REQUIRED ON)
