#pragma once
#ifndef VULKAN_COMMAND_EXECUTOR_HPP
#define VULKAN_COMMAND_EXECUTOR_HPP

#include <volk.h>
#include "RenderManagers/RenderCommandList.hpp"

/// @brief Vulkan implementation of the command executor interface.
class VulkanCommandExecutor : public IRenderCommandExecutor
{
public:
    explicit constexpr VulkanCommandExecutor(VkCommandBuffer commandBuffer)
        : _commandBuffer(commandBuffer) {}

    void CopyBufferToBuffer(
        GPUBufferHandle src, GPUBufferHandle dst,
        size_t srcOffset, size_t dstOffset, size_t size
    ) override;

    void CopyBufferToTexture(
        GPUBufferHandle src, GPUTextureHandle dst,
        size_t bufferOffset, uint32_t rowPitch, uint32_t rowCount,
        TextureSubresource subresource, Offset3D textureOffset, Extent3D textureExtent
    ) override;
private:
    VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
};

#endif //VULKAN_COMMAND_EXECUTOR_HPP
