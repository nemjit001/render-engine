#pragma once
#ifndef VULKAN_RENDER_TYPES_HPP
#define VULKAN_RENDER_TYPES_HPP

#include <volk.h>
#include <vk_mem_alloc.h>
#include "RenderManagers/RenderTypes.hpp"

class VulkanBuffer : public GPUBuffer
{
public:
    VulkanBuffer(VkBuffer buffer, VmaAllocation allocation);

    /// @brief Destroy buffer resources.
    /// @param device Vulkan device to use for resource destruction.
    /// @param allocator VMA allocator to use for resource destruction.
    void DestroyResources(VkDevice device, VmaAllocator allocator);

    [[nodiscard]] RenderBackend GetRenderBackend() const override { return RenderBackend::Vulkan; }

private:
    VkBuffer _buffer = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
};

class VulkanTexture : public GPUTexture
{
public:
    VulkanTexture(VkImage image, VkImageView view, VmaAllocation allocation);

    /// @brief Destroy texture resources.
    /// @param device Vulkan device to use for resource destruction.
    /// @param allocator VMA allocator to use for resource destruction.
    void DestroyResources(VkDevice device, VmaAllocator allocator);

    [[nodiscard]] RenderBackend GetRenderBackend() const override { return RenderBackend::Vulkan; }

private:
    VkImage _image = VK_NULL_HANDLE;
    VkImageView _view = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
};

#endif //VULKAN_RENDER_TYPES_HPP
