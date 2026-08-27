#pragma once
#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <memory>
#include <SDL3/SDL.h>

class IRenderManager;

/// @brief Enumeration of EngineResult result codes.
enum EngineResult : int
{
    EngineResult_Ok         = 0,
    EngineResult_BadInit    = 1,
};

/// @brief The Engine class handles subsystem lifetime and updates.
class Engine
{
private:
    /// @brief Initialize the engine.
    /// @return A boolean indicating success.
    [[nodiscard]] bool Init();

    /// @brief Shut down the engine.
    void Shutdown();

    /// @brief Pump the platform event loop.
    void PumpPlatformEvents();

    /// @brief Process a platform event.
    /// @param event Event to process.
    void ProcessEvent(SDL_Event const& event);

    /// @brief Render a frame for the engine.
    void Frame();

public:
    /// @brief Run the engine.
    /// @return An EngineResult.
    [[nodiscard]] EngineResult Run();

private:
    static bool gIsRunning;
    static std::shared_ptr<IRenderManager> gRenderManager;
};

#endif //ENGINE_HPP
