cmake_minimum_required(VERSION 3.28)
project("RenderEngine" VERSION 0.1.0)
enable_testing()

# Configure compilation settings
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set up dependencies
include(dependencies.cmake)
find_package(SDL3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# Set up RenderEngine target
add_executable(RenderEngine WIN32)
target_sources(RenderEngine
    PRIVATE
        src/main.cpp
        src/Engine.cpp
        src/RenderManagers/RenderTypes.hpp
        src/RenderManagers/Vulkan/UseVulkanLibs.cpp
        src/RenderManagers/Vulkan/VulkanRenderManager.cpp
        src/RenderManagers/Vulkan/VulkanRenderTypes.cpp
        src/RenderManagers/Vulkan/VulkanRenderTypes.hpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS src/
    FILES
        src/Engine.hpp
        src/FatalError.hpp
        src/RenderManager.hpp
        src/RenderManagers/Vulkan/VulkanRenderManager.hpp
)
target_compile_features(RenderEngine
    PRIVATE
        cxx_std_17
)
target_link_libraries(RenderEngine
    PRIVATE
        GPUOpen::VulkanMemoryAllocator
        SDL3::SDL3
        spdlog::spdlog
        volk::volk_headers
)

# Set up target warnings
if (MSVC)
    target_compile_options(RenderEngine PRIVATE /W4 /permissive-)
else()
    target_compile_options(RenderEngine PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
endif()

# Copy runtime dll dependencies
add_custom_command(TARGET RenderEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:RenderEngine> $<TARGET_FILE_DIR:RenderEngine>
    VERBATIM
)
