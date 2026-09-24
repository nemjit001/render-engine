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

    /// @brief Get the buffer handle.
    /// @return 
    [[nodiscard]]
    VkBuffer GetBuffer() const { return _buffer; }
    
    /// @brief Get the buffer allocation.
    /// @return 
    [[nodiscard]]
    VmaAllocation GetAllocation() const { return _allocation; }

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

    /// @brief Get the image handle.
    /// @return 
    [[nodiscard]]
    VkImage GetImage() const { return _image; }

    /// @brief Get the image view handle.
    /// @return 
    [[nodiscard]]
    VkImageView GetImageView() const { return _view; }

    /// @brief Get the image allocation.
    /// @return 
    [[nodiscard]]
    VmaAllocation GetAllocation() const { return _allocation; }

    [[nodiscard]] RenderBackend GetRenderBackend() const override { return RenderBackend::Vulkan; }

private:
    VkImage _image = VK_NULL_HANDLE;
    VkImageView _view = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
};

#endif //VULKAN_RENDER_TYPES_HPP
