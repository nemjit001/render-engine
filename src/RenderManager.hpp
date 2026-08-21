#pragma once
#ifndef RENDER_MANAGER_HPP
#define RENDER_MANAGER_HPP

#include <vector>
#include <SDL3/SDL.h>
#include <volk.h>
#include <vk_mem_alloc.h>

#ifndef NDEBUG
static constexpr bool RENDERER_ENABLE_DEBUG = true;
#else
static constexpr bool RENDERER_ENABLE_DEBUG = false;
#endif //NDEBUG

/// @brief Initialization info for the render manager.
struct RenderManagerInitInfo
{
    char const* windowTitle;
    uint32_t windowWidth;
    uint32_t windowHeight;
};

/// @brief The RenderManager manages render resources and frame submission.
class RenderManager
{
private:
    RenderManager() = default;

public:
    ~RenderManager() = default;

    RenderManager(RenderManager const&) = delete;
    RenderManager& operator=(RenderManager const&) = delete;

    /// @brief Get the render manager instance.
    /// @return The render manager instance.
    [[nodiscard]] static RenderManager& Get();

    /// @brief Initialize the render manager.
    /// @param initInfo Initialization info.
    /// @return A boolean indicating success.
    [[nodiscard]] bool Init(RenderManagerInitInfo const& initInfo);

    /// @brief Shut down the render manager.
    void Shutdown();

    /// @brief Process a platform event.
    /// @param event Event to process.
    void ProcessEvent(SDL_Event const& event);

    /// @brief Render a frame.
    void Frame();

    /// @brief Target Vulkan api version against which the application is written.
    static constexpr uint32_t TARGET_VULKAN_VERSION = VK_API_VERSION_1_3;

private:
    /// @brief The VulkanPhysicalDeviceInfo struct is used to store a physical device and its related
    /// information required for device initialization.
    struct VulkanPhysicalDeviceInfo
    {
        VkPhysicalDevice physicalDevice;
        bool isSupported;
        VkPhysicalDeviceProperties2 deviceProperties;
        VkPhysicalDeviceFeatures2 enabledDeviceFeatures;
        VkPhysicalDeviceVulkan11Features enabledVulkan11Features;
        VkPhysicalDeviceVulkan12Features enabledVulkan12Features;
        VkPhysicalDeviceVulkan13Features enabledVulkan13Features;
        uint32_t directQueueFamily;
    };

private:
    /// @brief Create the Vulkan instance state.
    /// @return A boolean indicating success.
    bool CreateVulkanInstance();

    /// @brief Find all Vulkan physical devices.
    /// @param instance The Vulkan instance to use for the search.
    /// @return A list of VulkanPhysicalDeviceInfo structures.
    std::vector<VulkanPhysicalDeviceInfo> FindPhysicalDevices(VkInstance instance);

    /// @brief Create the Vulkan device state.
    /// @param physicalDeviceInfo Physical device info to use for device creation.
    /// @return A boolean indicating success.
    bool CreateVulkanDevice(VulkanPhysicalDeviceInfo const& physicalDeviceInfo);

    /// @brief Destroy the Vulkan instance state.
    void DestroyVulkanInstance();

    /// @brief Destroy the Vulkan device state.
    void DestroyVulkanDevice();

private:
    VkInstance _instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VulkanPhysicalDeviceInfo _physicalDeviceInfo = {};
    VkDevice _device = VK_NULL_HANDLE;
    VkQueue _directQueue = VK_NULL_HANDLE;
};

#endif //RENDER_MANAGER_HPP
