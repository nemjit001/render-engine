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
        src/Engine.hpp
        src/FatalError.hpp
        src/RenderManager.cpp
        src/RenderManager.hpp
        src/UseVulkanLibs.cpp
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
