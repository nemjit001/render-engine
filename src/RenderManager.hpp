#pragma once
#ifndef RENDER_MANAGER_HPP
#define RENDER_MANAGER_HPP

#include <cstdint>
#include <SDL3/SDL.h>

#ifndef NDEBUG
static constexpr bool RENDERER_ENABLE_DEBUG = true;
#else
static constexpr bool RENDERER_ENABLE_DEBUG = false;
#endif //NDEBUG

/// @brief Initialization info for the render manager.
struct RenderManagerInitInfo
{
    char const* windowTitle = "App";    //< Default window title.
    uint32_t windowWidth = 1280u;       //< Initial window width.
    uint32_t windowHeight = 720u;       //< Initial window height.
    uint32_t framesInFlight = 2u;       //< Number of frames that may be recorded simultaneously, lower values means lower frame latency, values in the range [1, 3] are recommended.
};

/// @brief The RenderManager interface for managing render resources and frame submission can be implemented to support different render backends.
class IRenderManager
{
public:
    IRenderManager() = default;
    virtual ~IRenderManager() = default;

    IRenderManager(IRenderManager const&) = delete;
    IRenderManager& operator=(IRenderManager const&) = delete;

    /// @brief Initialize the render manager.
    /// @param initInfo Initialization info.
    /// @return A boolean indicating success.
    [[nodiscard]]
    virtual bool Init(RenderManagerInitInfo const& initInfo) = 0;

    /// @brief Shut down the render manager.
    virtual void Shutdown() = 0;

    /// @brief Process a platform event.
    /// @param event Event to process.
    virtual void ProcessEvent(SDL_Event const& event) = 0;

    /// @brief Start a new frame.
    /// @return A boolean indicating successful frame start.
    [[nodiscard]]
    virtual bool NewFrame() = 0;

    /// @brief End the current frame.
    virtual void EndFrame() = 0;

    /// @brief Execute the frame commands for the current frame.
    virtual void ExecuteFrame() const = 0;

    /// @brief Wait for the graphics device to be idle.
    virtual void WaitIdle() const = 0;

    /// @brief Get the current frame index.
    /// @return The current frame index.
    [[nodiscard]]
    virtual uint64_t GetCurrentFrameIndex() const = 0;
    
    /// @brief Get the current frame in flight index in the range [0, frames in flight].
    /// @return The frame in flight index.
    [[nodiscard]]
    virtual uint64_t GetCurrentFrameInFlightIndex() const = 0;
};

#endif //RENDER_MANAGER_HPP
