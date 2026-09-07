#include "Engine.hpp"

#include <spdlog/spdlog.h>

bool Engine::gIsRunning = false;
std::unique_ptr<IRenderManager> Engine::gRenderManager = nullptr;

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
    renderManagerInitInfo.windowWidth = 1280u;
    renderManagerInitInfo.windowHeight = 720u;
    renderManagerInitInfo.framesInFlight = 2u;
    renderManagerInitInfo.swapTextureFormat = TextureFormat_RGBA8_UNormSRGB;

    gRenderManager = IRenderManager::TryCreate();
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
    gRenderManager.reset();

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

void Engine::Frame()
{
    if (!gRenderManager->NewFrame()) {
        return;
    }

    gRenderManager->ExecuteFrame();
    gRenderManager->EndFrame();
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
        Frame();
    }

    // Do cleanup
    Shutdown();
    return EngineResult_Ok;
}
