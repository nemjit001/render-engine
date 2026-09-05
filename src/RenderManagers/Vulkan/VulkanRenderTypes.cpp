#include "VulkanRenderTypes.hpp"

VulkanBuffer::VulkanBuffer(VkBuffer buffer, VmaAllocation allocation)
    :
    _buffer(buffer),
    _allocation(allocation)
{
    //
}

void VulkanBuffer::DestroyResources([[maybe_unused]] VkDevice device, VmaAllocator allocator)
{
    vmaDestroyBuffer(allocator, _buffer, _allocation);
}

VulkanTexture::VulkanTexture(VkImage image, VkImageView view, VmaAllocation allocation)
    :
    _image(image),
    _view(view),
    _allocation(allocation)
{
    //
}

void VulkanTexture::DestroyResources(VkDevice device, VmaAllocator allocator)
{
    vkDestroyImageView(device, _view, nullptr);
    vmaDestroyImage(allocator, _image, _allocation);
}
