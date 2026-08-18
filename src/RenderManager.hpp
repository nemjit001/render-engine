#pragma once
#ifndef RENDER_MANAGER_HPP
#define RENDER_MANAGER_HPP

#include <SDL3/SDL.h>

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
    /// @return A boolean indicating success.
    [[nodiscard]] bool Init();

    /// @brief Shut down the render manager.
    void Shutdown();

    /// @brief Process a platform event.
    /// @param event Event to process.
    void ProcessEvent(SDL_Event const& event);

    /// @brief Render a frame.
    void Frame();
};

#endif //RENDER_MANAGER_HPP
