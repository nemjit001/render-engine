#include "Engine.hpp"

#include <spdlog/spdlog.h>

bool Engine::gIsRunning = false;
std::unique_ptr<IRenderManager> Engine::gRenderManager = nullptr;

static GPUBufferHandle gVertexBufferHandle = {};
static GPUBufferHandle gIndexBufferHandle = {};

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

    spdlog::info("Uploading render resources");
    {
        static constexpr float sVertexData[] = {
            -0.5F, -0.5F, 0.0F,
            -0.5F,  0.5F, 0.0F,
             0.5F,  0.5F, 0.0F,
             0.5F, -0.5F, 0.0F,
        };
        static constexpr uint32_t sIndexData[] = {
            0, 1, 2,
            2, 3, 0
        };

        // Create vertex buffer
        GPUBufferDesc vertexBufferDeviceDesc{};
        vertexBufferDeviceDesc.heapType = GPUHeapType_Default;
        vertexBufferDeviceDesc.size = sizeof(sVertexData);
        vertexBufferDeviceDesc.usage = BufferUsage_TransferDst | BufferUsage_VertexBuffer;

        GPUBufferHandle vertexBufferDevice = gRenderManager->CreateGPUBuffer(vertexBufferDeviceDesc);
        
        // Create index buffer
        GPUBufferDesc indexBufferDeviceDesc{};
        indexBufferDeviceDesc.heapType = GPUHeapType_Default;
        indexBufferDeviceDesc.size = sizeof(sIndexData);
        indexBufferDeviceDesc.usage = BufferUsage_TransferDst | BufferUsage_IndexBuffer;

        GPUBufferHandle indexBufferDevice = gRenderManager->CreateGPUBuffer(indexBufferDeviceDesc);

        // Write buffers
        gRenderManager->WriteBuffer(vertexBufferDevice, sVertexData, sizeof(sVertexData), 0);
        gRenderManager->WriteBuffer(indexBufferDevice, sIndexData, sizeof(sIndexData), 0);

        gVertexBufferHandle = vertexBufferDevice;
        gIndexBufferHandle = indexBufferDevice;
    }

    spdlog::info("Initialized!");
    gIsRunning = true;
    return true;
}

void Engine::Shutdown()
{
    spdlog::info("Shutting down...");

    spdlog::info("Cleaning up render resources");
    {
        gRenderManager->WaitIdle();
        gRenderManager->DestroyGPUBuffer(gIndexBufferHandle);
        gRenderManager->DestroyGPUBuffer(gVertexBufferHandle);
    }

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

    IRenderCommandList* frameCommandList = gRenderManager->CreateRenderCommandList();
    gRenderManager->ExecuteFrame(frameCommandList);
    gRenderManager->DestroyRenderCommandList(frameCommandList);

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
