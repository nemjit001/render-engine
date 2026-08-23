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
    char const* windowTitle = "App";    //< Default window title.
    uint32_t windowWidth = 1280u;       //< Initial window width.
    uint32_t windowHeight = 720u;       //< Initial window height.
    uint32_t framesInFlight = 2u;       //< Number of frames that may be recorded simultaneously, lower values means lower frame latency, values in the range [1, 3] are recommended.
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

    /// @brief Handle a window resize event.
    void OnWindowResize();

    /// @brief Start a new frame.
    /// @return A boolean indicating successful frame start.
    bool NewFrame();

    /// @brief End the current frame.
    void EndFrame();

    /// @brief Wait for the graphics device to be idle.
    void WaitIdle() const;

    /// @brief Get the current frame index.
    /// @return The current frame index.
    uint64_t GetCurrentFrameIndex() const { return _currentFrameIndex; }

    /// @brief Get the current frame in flight index in the range [0, frames in flight].
    /// @return The frame in flight index.
    uint64_t GetFrameInFlightIndex() const { return GetCurrentFrameIndex() % _framesInFlight; }

    /// @brief Target Vulkan api version against which the application is written.
    static constexpr uint32_t TARGET_VULKAN_VERSION = VK_API_VERSION_1_3;
    /// @brief Preferred Vulkan swap surface format.
    static constexpr VkFormat PREFERRED_SWAP_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;

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

    /// @brief The VulkanFrameState struct contains per-frame data that is used to record frame commands.
    struct VulkanFrameState
    {
        VkFence frameReadyFence;
        VkCommandPool directCommandPool;
        VkCommandBuffer directCommandBuffer;
    };

    /// @brief The VulkanSwapchainConfig struct contains swapchain configuration data that
    /// is queried from the physical device and render surface.
    struct VulkanSwapchainConfig
    {
        uint32_t width;
        uint32_t height;
        uint32_t imageCount;
        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR presentMode;
        VkSurfaceTransformFlagBitsKHR surfaceTransform;
    };

    /// @brief The VulkanWindowState struct contains state related to the render surface and swap chain.
    struct VulkanWindowState
    {
        SDL_Window* window = nullptr;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VulkanSwapchainConfig swapchainConfig;
        std::vector<VkImage> swapImages;
        std::vector<VkImageView> swapImageViews;
        std::vector<VkSemaphore> swapImageAcquiredSemaphores; //< Sized on frames in flight
        std::vector<VkSemaphore> swapImageReleasedSemaphores; //< Sized on swap image count
        bool reconfigureSwapchain;
        uint32_t currentSwapImageIdx;
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

    /// @brief Create the Vulkan frame state.
    /// @param framesInFlight Number of frames in flight.
    /// @return A boolean indicating success.
    bool CreateVulkanFrameState(uint32_t framesInFlight);

    /// @brief Create the Vulkan window state.
    /// @param title Window title.
    /// @param width Window width.
    /// @param height window height.
    /// @return A boolean indicating success.
    bool CreateVulkanWindowState(char const* title, uint32_t width, uint32_t height);

    /// @brief Destroy the Vulkan instance state.
    void DestroyVulkanInstance();

    /// @brief Destroy the Vulkan device state.
    void DestroyVulkanDevice();

    /// @brief Destroy the Vulkan frame state.
    void DestroyVulkanFrameState();

    /// @brief Destroy the Vulkan window state.
    void DestroyVulkanWindowState();

    /// @brief Get the Vulkan swapchain configuration for a window and surface combination.
    /// @param window Window to query config for.
    /// @param surface Surface to query config for.
    /// @param preferredSurfaceFormat Preferred swap surface format.
    /// @param preferredPresentMode Preferred swap present mode.
    /// @return The swapchain configuration.
    VulkanSwapchainConfig GetVulkanSwapchainConfiguration(
        SDL_Window* window,
        VkSurfaceKHR surface,
        VkFormat preferredSurfaceFormat,
        VkPresentModeKHR preferredPresentMode
    ) const;

    /// @brief Configure the Vulkan swapchain for a Vulkan window state.
    /// @param windowState WindowState to configure swapchain for.
    /// @param preferredFormat Preferred surface format.
    /// @param preferredPresentMode Preferred present mode.
    /// @return A boolean indicating success.
    bool ConfigureSwapchain(VulkanWindowState& windowState, VkFormat preferredFormat, VkPresentModeKHR preferredPresentMode) const;

    /// @brief Destroy the Vulkan swapchain image state.
    /// @param windowState WindowState to destroy image state for.
    void DestroySwapchainImageState(VulkanWindowState& windowState) const;

    /// @brief Acquire the next swapchain image for a window state, may fatally exit if an unrecoverable error is encountered.
    /// @param windowState Window state to acquire swap image for.
    /// @return A boolean indicating success.
    bool AcquireNextSwapchainImage(VulkanWindowState& windowState) const;

    /// @brief Present the last acquired swapchain image for a window state, may fatally exit if an unrecoverable error is encountered.
    /// @param windowState Window state to present for.
    void Present(VulkanWindowState& windowState) const;

private:
    // Instance state
    VkInstance _instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;

    // Device state
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VulkanPhysicalDeviceInfo _physicalDeviceInfo = {};
    VkDevice _device = VK_NULL_HANDLE;
    VkQueue _directQueue = VK_NULL_HANDLE;
    
    // Frame state
    uint64_t _framesInFlight = 0;
    uint64_t _currentFrameIndex = 0;
    std::vector<VulkanFrameState> _frameStates = {};

    // Window state
    VulkanWindowState _windowState = {};
};

#endif //RENDER_MANAGER_HPP
