#include "VulkanCommandExecutor.hpp"

#include "RenderManagers/RenderTypes.hpp"
#include "RenderManagers/Vulkan/VulkanRenderTypes.hpp"

void VulkanCommandExecutor::CopyBufferToBuffer(
    GPUBufferHandle src, GPUBufferHandle dst,
    size_t srcOffset, size_t dstOffset, size_t size
)
{
    if (src->GetRenderBackend() != RenderBackend::Vulkan || dst->GetRenderBackend() != RenderBackend::Vulkan) {
        return;
    }

    auto* vulkanSrcBuffer = static_cast<VulkanBuffer*>(src);
    auto* vulkanDstBuffer = static_cast<VulkanBuffer*>(dst);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = static_cast<VkDeviceSize>(srcOffset);
    copyRegion.dstOffset = static_cast<VkDeviceSize>(dstOffset);
    copyRegion.size = static_cast<VkDeviceSize>(size);

    vkCmdCopyBuffer(
        _commandBuffer,
        vulkanSrcBuffer->GetBuffer(),
        vulkanDstBuffer->GetBuffer(),
        1,
        &copyRegion
    );
}

void VulkanCommandExecutor::CopyBufferToTexture(
    GPUBufferHandle src, GPUTextureHandle dst,
    size_t bufferOffset, uint32_t rowPitch, uint32_t rowCount,
    TextureSubresource subresource, Offset3D textureOffset, Extent3D textureExtent
)
{
    if (src->GetRenderBackend() != RenderBackend::Vulkan || dst->GetRenderBackend() != RenderBackend::Vulkan) {
        return;
    }

    auto* vulkanSrcBuffer = static_cast<VulkanBuffer*>(src);
    auto* vulkanDstTexture = static_cast<VulkanTexture*>(dst);

    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset = static_cast<VkDeviceSize>(bufferOffset);
    copyRegion.bufferRowLength = static_cast<uint32_t>(rowPitch);
    copyRegion.bufferImageHeight = static_cast<uint32_t>(rowCount);
    copyRegion.imageSubresource = {}; // FIXME(nemjit001): Set subresource
    copyRegion.imageOffset = { textureOffset.x, textureOffset.y, textureOffset.z };
    copyRegion.imageExtent = { textureExtent.width, textureExtent.height, textureExtent.depth };

    vkCmdCopyBufferToImage(
        _commandBuffer,
        vulkanSrcBuffer->GetBuffer(),
        vulkanDstTexture->GetImage(),
        VK_IMAGE_LAYOUT_UNDEFINED, // FIXME(nemjit001): Set image layout
        1,
        &copyRegion
    );
}
