cmake_minimum_required(VERSION 3.28)
project("RenderEngine" VERSION 0.1.0)
enable_testing()

# Configure compilation settings
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set up dependencies
include(dependencies.cmake)
find_package(SDL3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Set up RenderEngine target
add_executable(RenderEngine WIN32)
target_sources(RenderEngine
    PRIVATE
        src/main.cpp
)
target_link_libraries(RenderEngine
    PRIVATE
        SDL3::SDL3
        spdlog::spdlog
)

# Copy runtime dll dependencies
add_custom_command(TARGET RenderEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:RenderEngine> $<TARGET_FILE_DIR:RenderEngine>
    VERBATIM
)
