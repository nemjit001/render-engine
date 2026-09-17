cmake_minimum_required(VERSION 3.28)
project("RenderEngine" VERSION 0.1.0)
enable_testing()

# Configure compilation settings
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set up dependencies
include(cmake/dependencies.cmake)
include(cmake/utils.cmake)
find_package(GTest CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# Set up RenderEngineLib library target
add_library(RenderEngineLib STATIC)
add_library(RenderEngine::Lib ALIAS RenderEngineLib)
target_sources(RenderEngineLib
    PRIVATE
        src/Engine.cpp
        src/RenderManager.cpp
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
target_compile_features(RenderEngineLib
    PRIVATE
        cxx_std_17
)
target_link_libraries(RenderEngineLib
    PUBLIC
        GPUOpen::VulkanMemoryAllocator
        SDL3::SDL3
        spdlog::spdlog
        volk::volk_headers
)
target_enable_extended_warnings(RenderEngineLib)

# Set up RenderEngine executable target
add_executable(RenderEngine WIN32)
add_executable(RenderEngine::Bin ALIAS RenderEngine)
target_sources(RenderEngine
    PRIVATE
        src/main.cpp
)
target_compile_features(RenderEngine
    PRIVATE
        cxx_std_17
)
target_link_libraries(RenderEngine
    PRIVATE
        RenderEngine::Lib
)
target_enable_extended_warnings(RenderEngine)
target_copy_runtime_dlls(RenderEngine)
