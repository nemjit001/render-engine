#pragma once
#ifndef RENDER_MANAGER_HPP
#define RENDER_MANAGER_HPP

#include <SDL3/SDL.h>
#include <volk.h>
#include <vk_mem_alloc.h>

/// @brief Initialization info for the render manager.
struct RenderManagerInitInfo
{
    char const* windowTitle;
    uint32_t windowWidth;
    uint32_t windowHeight;
};

/// @brief The RenderManager manages render resources and frame submission.
class RenderManager
{
private:
    RenderManager() = default;

public:
    ~RenderManager() = default;

    RenderManager(RenderManager const&) = delete;
    RenderManager& operator=(RenderManager const&) = delete;

    /// @brief Get the render manager instance.
    /// @return The render manager instance.
    [[nodiscard]] static RenderManager& Get();

    /// @brief Initialize the render manager.
    /// @param initInfo Initialization info.
    /// @return A boolean indicating success.
    [[nodiscard]] bool Init(RenderManagerInitInfo const& initInfo);

    /// @brief Shut down the render manager.
    void Shutdown();

    /// @brief Process a platform event.
    /// @param event Event to process.
    void ProcessEvent(SDL_Event const& event);

    /// @brief Render a frame.
    void Frame();

    /// @brief Target Vulkan api version against which the application is written.
    static constexpr uint32_t TARGET_VULKAN_VERSION = VK_API_VERSION_1_3;
};

#endif //RENDER_MANAGER_HPP
