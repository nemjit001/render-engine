#include "Engine.hpp"

#include <spdlog/spdlog.h>
#include "RenderManager.hpp"

bool Engine::gIsRunning = false;
RenderManager* Engine::gRenderManager = nullptr;

bool Engine::Init()
{
    spdlog::set_level(spdlog::level::trace);
    spdlog::info("Initialized logger");

    spdlog::info("Initializing window system");
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        spdlog::error("Failed to initialize window system");
        return false;
    }

    spdlog::info("Initializing render manager");
    RenderManagerInitInfo renderManagerInitInfo{};
    renderManagerInitInfo.windowTitle = "Render Engine";
    renderManagerInitInfo.windowWidth = 1280;
    renderManagerInitInfo.windowHeight = 720;

    gRenderManager = &RenderManager::Get();
    if (!gRenderManager->Init(renderManagerInitInfo))
    {
        spdlog::error("Failed to initialize render manager");
        return false;
    }

    spdlog::info("Initialized!");
    gIsRunning = true;
    return true;
}

void Engine::Shutdown()
{
    spdlog::info("Shutting down...");

    spdlog::info("Shutting down render manager");
    gRenderManager->Shutdown();

    spdlog::info("Shutting down window system");
    SDL_Quit();

    spdlog::info("Clean shutdown, goodbye!");
}

void Engine::PumpPlatformEvents()
{
    SDL_Event event{};
    while (SDL_PollEvent(&event))
    {
        ProcessEvent(event);
        gRenderManager->ProcessEvent(event);
    }
}

void Engine::ProcessEvent(SDL_Event const& event)
{
    if (event.type == SDL_EVENT_QUIT) {
        gIsRunning = false;
    }
}

EngineResult Engine::Run()
{
    // Initialize engine
    if (!Init()) {
        return EngineResult_BadInit;
    }

    // Enter engine main loop
    while (gIsRunning)
    {
        PumpPlatformEvents();
        gRenderManager->Frame();
    }

    // Do cleanup
    Shutdown();
    return EngineResult_Ok;
}
