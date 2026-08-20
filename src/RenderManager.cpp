#include "RenderManager.hpp"

#include <spdlog/spdlog.h>
#include <SDL3/SDL_vulkan.h>

#define VK_SUCCEEDED(result)    (result == VK_SUCCESS)
#define VK_FAILED(result)       (result != VK_SUCCESS)

RenderManager& RenderManager::Get()
{
    static RenderManager instance{};
    return instance;
}

bool RenderManager::Init(RenderManagerInitInfo const& initInfo)
{
    spdlog::trace("Loading Vulkan symbols");
    {
        if (VK_FAILED(volkInitialize())
            || !SDL_Vulkan_LoadLibrary(nullptr)
        )
        {
            spdlog::error("Failed to load Vulkan symbols");
            return false;
        }

        uint32_t const vulkanApiVersion = volkGetInstanceVersion();
        spdlog::trace("Loaded Vulkan symbols ({}.{}.{})",
            VK_API_VERSION_MAJOR(vulkanApiVersion),
            VK_API_VERSION_MINOR(vulkanApiVersion),
            VK_API_VERSION_PATCH(vulkanApiVersion)
        );

        if (vulkanApiVersion < TARGET_VULKAN_VERSION)
        {
            spdlog::error("Unsupported Vulkan symbols found, the application requires at least Vulkan {}.{}.{}",
                VK_API_VERSION_MAJOR(TARGET_VULKAN_VERSION),
                VK_API_VERSION_MINOR(TARGET_VULKAN_VERSION),
                VK_API_VERSION_PATCH(TARGET_VULKAN_VERSION)
            );
            return false;
        }
    }

    return true;
}

void RenderManager::Shutdown()
{
    spdlog::trace("Unloading Vulkan symbols");
    SDL_Vulkan_UnloadLibrary();
    volkFinalize();
}

void RenderManager::ProcessEvent(SDL_Event const& event)
{
    //
}

void RenderManager::Frame()
{
    //
}
