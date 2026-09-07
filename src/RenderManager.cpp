#include "RenderManager.hpp"

#include "RenderManagers/Vulkan/VulkanRenderManager.hpp"

std::unique_ptr<IRenderManager> IRenderManager::TryCreate()
{
    return std::make_unique<VulkanRenderManager>();
}
