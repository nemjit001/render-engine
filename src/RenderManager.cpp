#include "RenderManager.hpp"

#include <spdlog/spdlog.h>
#include <SDL3/SDL_vulkan.h>
#include "FatalError.hpp"

#define VK_SUCCEEDED(result)    (result == VK_SUCCESS)
#define VK_FAILED(result)       (result != VK_SUCCESS)

/// @brief Callback for handling Vulkan debug messages.
/// @param messageSeverity 
/// @param messageType 
/// @param callbackData 
/// @param userData 
/// @return 
[[maybe_unused]] static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const* callbackData,
    [[maybe_unused]] void* userData
)
{
    spdlog::trace("{}", callbackData->pMessage);
    return VK_FALSE;
}

/// @brief Verify a layer name exists in a list of layers.
/// @param layers Layer list to search.
/// @param layerName Name of the layer to search for.
/// @return true if the layer name exists in the layer list.
[[maybe_unused]] static bool IsLayerAvailable(std::vector<VkLayerProperties> const& layers, char const* layerName)
{
    for (auto const& layer : layers)
    {
        if (strcmp(layer.layerName, layerName) == 0) {
            return true;
        }
    }

    return false;
}

/// @brief Verify an extension name exists in a list of extensions.
/// @param extensions Extension list to search.
/// @param extensionName Name of the extension to search for.
/// @return true if the extension name exists in the extension list.
static bool IsExtensionAvailable(std::vector<VkExtensionProperties> const& extensions, char const* extensionName)
{
    for (auto const& ext : extensions)
    {
        if (strcmp(ext.extensionName, extensionName) == 0) {
            return true;
        }
    }

    return false;
}

/// @brief Try to enable a layer name.
/// @param enabledLayerNames Enabled layer names list to store enabled layer name in.
/// @param availableLayers Available layer list to use for availability search.
/// @param layerName Layer name to add.
/// @return A boolean indicating whether the name was added to the list.
[[maybe_unused]] static bool TryEnableLayer(std::vector<char const*>& enabledLayerNames, std::vector<VkLayerProperties> const& availableLayers, char const* layerName)
{
    if (IsLayerAvailable(availableLayers, layerName))
    {
        spdlog::trace("Enabled layer: {}", layerName);
        enabledLayerNames.push_back(layerName);
        return true;
    }

    return false;
}

/// @brief Try to enable an extension name.
/// @param enabledExtensionNames Enabled extension names list to store enabled extension name in.
/// @param availableExtensions Available extension list to use for availability search.
/// @param extensionName Extension name to add.
/// @return A boolean indicating whether the name was added to the list.
static bool TryEnableExtension(std::vector<char const*>& enabledExtensionNames, std::vector<VkExtensionProperties> const& availableExtensions, char const* extensionName)
{
    if (IsExtensionAvailable(availableExtensions, extensionName))
    {
        spdlog::trace("Enabled extension: {}", extensionName);
        enabledExtensionNames.push_back(extensionName);
        return true;
    }
    
    return false;
}

RenderManager& RenderManager::Get()
{
    static RenderManager instance{};
    return instance;
}

bool RenderManager::Init(RenderManagerInitInfo const& initInfo)
{
    // Load Vulkan symbols
    {
        spdlog::trace("Loading Vulkan symbols");
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

    // Create the Vulkan instance
    if (!CreateVulkanInstance()) {
        return false;
    }
    
    // Search for first supported physical device
    std::vector<VulkanPhysicalDeviceInfo> const availablePhysicalDevices = FindPhysicalDevices(_instance);
    VulkanPhysicalDeviceInfo chosenDevice{};
    for (auto const& deviceInfo : availablePhysicalDevices)
    {
        // Pick first supported device
        if (deviceInfo.isSupported)
        {
            chosenDevice = deviceInfo;
            break;
        }
    }

    if (!chosenDevice.isSupported)
    {
        spdlog::error("Failed to find a supported Vulkan device");
        return false;
    }

    // Create the Vulkan device
    if (!CreateVulkanDevice(chosenDevice)) {
        return false;
    }

    // Create the Vulkan frame state
    if (!CreateVulkanFrameState(initInfo.framesInFlight)) {
        return false;
    }

    // Create the Vulkan window state
    if (!CreateVulkanWindowState(initInfo.windowTitle, initInfo.windowWidth, initInfo.windowHeight)) {
        return false;
    }

    return true;
}

void RenderManager::Shutdown()
{
    WaitIdle();

    DestroyVulkanWindowState();
    DestroyVulkanFrameState();
    DestroyVulkanDevice();
    DestroyVulkanInstance();

    spdlog::trace("Unloading Vulkan symbols");
    SDL_Vulkan_UnloadLibrary();
    volkFinalize();
}

void RenderManager::ProcessEvent(SDL_Event const& event)
{
    if (event.type == SDL_EVENT_WINDOW_RESIZED && event.window.windowID == SDL_GetWindowID(_windowState.window)) {
        OnWindowResize();
    }
}

void RenderManager::OnWindowResize()
{
    // Ensure all queued work is finished, ensuring any swapchain resources in-flight are no longer in use
    WaitIdle();

    // Reconfigure swapchain
    if (!ConfigureSwapchain(_windowState, PREFERRED_SWAP_FORMAT, VK_PRESENT_MODE_FIFO_KHR)) {
        spdlog::error("Failed to reconfigure Vulkan swapchain, continuing with outdated swapchain");
    }
}

bool RenderManager::NewFrame()
{
    // Wait for frame ready
    VulkanFrameState const& frameState = _frameStates[GetFrameInFlightIndex()];
    if (VK_FAILED(vkWaitForFences(_device, 1, &frameState.frameReadyFence, VK_TRUE, UINT64_MAX))) {
        FATAL_ERROR("Failed to wait on fence for frame {}", _currentFrameIndex);
    }

    // Acquire swap image
    if (!AcquireNextSwapchainImage(_windowState)) {
        return false;
    }

    // Reset frame fence and start command recording
    if (VK_FAILED(vkResetFences(_device, 1, &frameState.frameReadyFence))) {
        FATAL_ERROR("Failed to reset fence for frame {}", _currentFrameIndex);
    }

    VkCommandBufferBeginInfo frameCommandsBeginInfo{};
    frameCommandsBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    frameCommandsBeginInfo.pNext = nullptr;
    frameCommandsBeginInfo.flags = 0;
    frameCommandsBeginInfo.pInheritanceInfo = nullptr;

    if (VK_FAILED(vkBeginCommandBuffer(frameState.directCommandBuffer, &frameCommandsBeginInfo))) {
        FATAL_ERROR("Failed to begin command recording for frame {}", _currentFrameIndex);
    }

    return true;
}

void RenderManager::EndFrame()
{
    VulkanFrameState const& frameState = _frameStates[GetFrameInFlightIndex()];
    if (VK_FAILED(vkEndCommandBuffer(frameState.directCommandBuffer))) {
        FATAL_ERROR("Failed to end command recording for frame {}", _currentFrameIndex);
    }

    // Submit frame commands
    VkPipelineStageFlags const waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.pWaitSemaphores = &_windowState.swapImageAcquiredSemaphores[GetFrameInFlightIndex()];
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frameState.directCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_windowState.swapImageReleasedSemaphores[_windowState.currentSwapImageIdx];

    if (VK_FAILED(vkQueueSubmit(_directQueue, 1, &submitInfo, frameState.frameReadyFence))) {
        FATAL_ERROR("Failed to submit frame commands for frame {}", _currentFrameIndex);
    }

    // Present frame
    Present(_windowState);

    // Increment frame index
    _currentFrameIndex++;
}

bool RenderManager::CreateVulkanInstance()
{
    // Get instance layers and extensions
    std::vector<VkLayerProperties> const availableInstanceLayers = []() {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        return availableLayers;
    }();
    std::vector<VkExtensionProperties> const availableInstanceExtensions = []() {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
        return availableExtensions;
    }();

    // Enable instance layers and extensions
    std::vector<char const*> enabledInstanceLayers;
    if constexpr (RENDERER_ENABLE_DEBUG)
    {
        spdlog::trace("Searching for Vulkan instance layers");
        TryEnableLayer(enabledInstanceLayers, availableInstanceLayers, "VK_LAYER_KHRONOS_validation");
        TryEnableLayer(enabledInstanceLayers, availableInstanceLayers, "VK_LAYER_KHRONOS_synchronization2");
    }

    // Enable instance extensions
    spdlog::trace("Searching for Vulkan instance extensions");
    std::vector<char const*> enabledInstanceExtensions;

    uint32_t requiredExtensionCount = 0;
    char const* const* requiredInstanceExtensions = SDL_Vulkan_GetInstanceExtensions(&requiredExtensionCount);
    for (uint32_t i = 0; i < requiredExtensionCount; i++)
    {
        if (!TryEnableExtension(enabledInstanceExtensions, availableInstanceExtensions, requiredInstanceExtensions[i]))
        {
            spdlog::error("Required instance extension not available: {}", requiredInstanceExtensions[i]);
            return false;
        }
    }

    if constexpr (RENDERER_ENABLE_DEBUG)
    {
        if (!TryEnableExtension(enabledInstanceExtensions, availableInstanceExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            spdlog::error("Required instance extension not available: {}", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            return false;
        }
    }

    // Set up instance create info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "RenderEngine";
    appInfo.applicationVersion = 0;
    appInfo.pEngineName = "RenderEngine";
    appInfo.engineVersion = 0;
    appInfo.apiVersion = TARGET_VULKAN_VERSION;

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = nullptr;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledInstanceLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = enabledInstanceLayers.data();
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
    debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessengerCreateInfo.pNext = nullptr;
    debugUtilsMessengerCreateInfo.flags = 0;
    debugUtilsMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugUtilsMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugUtilsMessengerCreateInfo.pfnUserCallback = VulkanDebugCallback;
    debugUtilsMessengerCreateInfo.pUserData = nullptr;

    if constexpr (RENDERER_ENABLE_DEBUG) {
        instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfo;
    }

    spdlog::trace("Creating Vulkan instance");
    if (VK_FAILED(vkCreateInstance(&instanceCreateInfo, nullptr, &_instance)))
    {
        spdlog::error("Failed to create Vulkan instance");
        return false;
    }
    volkLoadInstance(_instance);

    if constexpr (RENDERER_ENABLE_DEBUG)
    {
        spdlog::trace("Creating Vulkan debug messenger");
        if (VK_FAILED(vkCreateDebugUtilsMessengerEXT(_instance, &debugUtilsMessengerCreateInfo, nullptr, &_debugMessenger)))
        {
            spdlog::error("Failed to create Vulkan debug messenger");
            return false;
        }
    }

    return true;
}

std::vector<RenderManager::VulkanPhysicalDeviceInfo> RenderManager::FindPhysicalDevices(VkInstance instance)
{
    spdlog::trace("Searching for supported Vulkan physical devices");

    // Get available physical devices
    std::vector<VkPhysicalDevice> const availablePhysicalDevices = [](VkInstance instance) {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> availableDevices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, availableDevices.data());
        return availableDevices;
    }(instance);

    // Gather physical device info
    std::vector<VulkanPhysicalDeviceInfo> physicalDeviceInfos{};
    physicalDeviceInfos.reserve(availablePhysicalDevices.size());
    for (auto const& physicalDevice : availablePhysicalDevices)
    {
        // Get device properties
        VkPhysicalDeviceProperties2 deviceProperties{};
        deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties.pNext = nullptr;
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties);

        // Get device features
        VkPhysicalDeviceFeatures2 availableDeviceFeatures{};
        availableDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        availableDeviceFeatures.pNext = nullptr;

        VkPhysicalDeviceVulkan11Features availableVulkan11Features{};
        availableDeviceFeatures.pNext = &availableVulkan11Features;
        availableVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        availableVulkan11Features.pNext = nullptr;

        VkPhysicalDeviceVulkan12Features availableVulkan12Features{};
        availableVulkan11Features.pNext = &availableVulkan12Features;
        availableVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        availableVulkan12Features.pNext = nullptr;

        VkPhysicalDeviceVulkan13Features availableVulkan13Features{};
        availableVulkan12Features.pNext = &availableVulkan13Features;
        availableVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        availableVulkan13Features.pNext = nullptr;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &availableDeviceFeatures);

        // Get device queue families
        std::vector<VkQueueFamilyProperties> const queueFamilies = [](VkPhysicalDevice physicalDevice) {
            uint32_t queueCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueFamilies.data());
            return queueFamilies;
        }(physicalDevice);

        // Check device support and enable required features
        bool isDeviceSupported = true; // Assume the device is supported until found otherwise
        if (deviceProperties.properties.apiVersion <= TARGET_VULKAN_VERSION) {
            isDeviceSupported = false;
        }

        VkPhysicalDeviceFeatures2 enabledDeviceFeatures{};
        enabledDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        enabledDeviceFeatures.pNext = nullptr;

        VkPhysicalDeviceVulkan11Features enabledVulkan11Features{};
        enabledVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        enabledVulkan11Features.pNext = nullptr;

        VkPhysicalDeviceVulkan12Features enabledVulkan12Features{};
        enabledVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        enabledVulkan12Features.pNext = nullptr;

        VkPhysicalDeviceVulkan13Features enabledVulkan13Features{};
        enabledVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        enabledVulkan13Features.pNext = nullptr;

        if (availableVulkan12Features.bufferDeviceAddress == VK_TRUE) {
            enabledVulkan12Features.bufferDeviceAddress = VK_TRUE;
        }
        else {
            isDeviceSupported = false;
        }

        if (availableVulkan13Features.synchronization2 == VK_TRUE) {
            enabledVulkan13Features.synchronization2 = VK_TRUE;
        }
        else {
            isDeviceSupported = false;
        }

        if (availableVulkan13Features.dynamicRendering == VK_TRUE) {
            enabledVulkan13Features.dynamicRendering = VK_TRUE;
        }
        else {
            isDeviceSupported = false;
        }

        // Search for required device queue families
        auto const findQueueFamily = [&queueFamilies](VkInstance instance, VkPhysicalDevice physicalDevice, VkQueueFlags requiredFlags, VkQueueFlags ignoredFlags, bool requireSurfaceSupport) {
            for (uint32_t queueIdx = 0; queueIdx < queueFamilies.size(); queueIdx++)
            {
                VkQueueFamilyProperties const& queueProps = queueFamilies[queueIdx];
                if ((requiredFlags & queueProps.queueFlags) == requiredFlags && (ignoredFlags & queueProps.queueFlags) == 0)
                {
                    if (requireSurfaceSupport && !SDL_Vulkan_GetPresentationSupport(instance, physicalDevice, queueIdx)) {
                        continue;
                    }

                    return queueIdx;
                }
            }
            return VK_QUEUE_FAMILY_IGNORED;
        };

        uint32_t const directQueueFamily = findQueueFamily(_instance, physicalDevice, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, 0, true);
        if (directQueueFamily == VK_QUEUE_FAMILY_IGNORED)
        {
            spdlog::warn("Physical device {} ({}) does not support the required device queues, skipping device",
                deviceProperties.properties.deviceName,
                deviceProperties.properties.deviceID
            );
            continue;
        }

        // Fill out physical device info
        VulkanPhysicalDeviceInfo physicalDeviceInfo{};
        physicalDeviceInfo.physicalDevice = physicalDevice;
        physicalDeviceInfo.isSupported = isDeviceSupported;
        physicalDeviceInfo.deviceProperties = deviceProperties;
        physicalDeviceInfo.enabledDeviceFeatures = enabledDeviceFeatures;
        physicalDeviceInfo.enabledVulkan11Features = enabledVulkan11Features;
        physicalDeviceInfo.enabledVulkan12Features = enabledVulkan12Features;
        physicalDeviceInfo.enabledVulkan13Features = enabledVulkan13Features;
        physicalDeviceInfo.directQueueFamily = directQueueFamily;

        physicalDeviceInfos.push_back(physicalDeviceInfo);
    }

    return physicalDeviceInfos;
}

bool RenderManager::CreateVulkanDevice(VulkanPhysicalDeviceInfo const& physicalDeviceInfo)
{
    _physicalDevice = physicalDeviceInfo.physicalDevice;
    _physicalDeviceInfo = physicalDeviceInfo;
    spdlog::trace("Using Vulkan physical device: \"{} ({})\"",
        physicalDeviceInfo.deviceProperties.properties.deviceName,
        physicalDeviceInfo.deviceProperties.properties.deviceID
    );

    // Get available device extensions
    std::vector<VkExtensionProperties> const availableDeviceExtensions = [](VkPhysicalDevice physicalDevice) {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
        return availableExtensions;
    }(_physicalDevice);

    // Enable device extensions
    spdlog::trace("Searching for Vulkan device extensions");
    std::vector<char const*> enabledDeviceExtensions;
    if (!TryEnableExtension(enabledDeviceExtensions, availableDeviceExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
    {
        spdlog::error("Required device extension not available: {}", VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        return false;
    }

    // Set up enabled features pNext chain
    _physicalDeviceInfo.enabledDeviceFeatures.pNext = &_physicalDeviceInfo.enabledVulkan11Features;
    _physicalDeviceInfo.enabledVulkan11Features.pNext = &_physicalDeviceInfo.enabledVulkan12Features;
    _physicalDeviceInfo.enabledVulkan13Features.pNext = &_physicalDeviceInfo.enabledVulkan13Features;
    _physicalDeviceInfo.enabledVulkan13Features.pNext = nullptr;

    // Create device queues
    static float queuePriorities[] = { 1.0F };
    VkDeviceQueueCreateInfo directQueueCreateInfo{};
    directQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    directQueueCreateInfo.pNext = nullptr;
    directQueueCreateInfo.flags = 0;
    directQueueCreateInfo.queueFamilyIndex = physicalDeviceInfo.directQueueFamily;
    directQueueCreateInfo.queueCount = 1;
    directQueueCreateInfo.pQueuePriorities = queuePriorities;

    // Create logical device
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &_physicalDeviceInfo.enabledDeviceFeatures; // Enables required features through pNext chain
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &directQueueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures = nullptr;

    spdlog::trace("Creating Vulkan device");
    if (VK_FAILED(vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device)))
    {
        spdlog::error("Failed to create Vulkan device");
        return false;
    }
    volkLoadDevice(_device);
    vkGetDeviceQueue(_device, physicalDeviceInfo.directQueueFamily, 0, &_directQueue);

    // Create VMA allocator
    VmaAllocatorCreateInfo allocatorCreateInfo{};
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT
        | VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorCreateInfo.vulkanApiVersion = TARGET_VULKAN_VERSION;
    allocatorCreateInfo.instance = _instance;
    allocatorCreateInfo.physicalDevice = _physicalDevice;
    allocatorCreateInfo.device = _device;
    allocatorCreateInfo.pVulkanFunctions = nullptr;

    VmaVulkanFunctions vmaVulkanFunctions{};
    vmaImportVulkanFunctionsFromVolk(&allocatorCreateInfo, &vmaVulkanFunctions);
    allocatorCreateInfo.pVulkanFunctions = &vmaVulkanFunctions;

    spdlog::trace("Creating VMA allocator");
    if (VK_FAILED(vmaCreateAllocator(&allocatorCreateInfo, &_allocator)))
    {
        spdlog::error("Failed to create VMA allocator");
        return false;
    }

    return true;
}

bool RenderManager::CreateVulkanFrameState(uint32_t framesInFlight)
{
    _framesInFlight = framesInFlight;
    if (framesInFlight == 0 || framesInFlight > 3)
    {
        spdlog::error("Provided frames in flight number is outside of recommended range of [0, 3] (was {})", framesInFlight);
        return false;
    }

    _frameStates.resize(_framesInFlight);
    for (auto& frameState : _frameStates)
    {
        VkFenceCreateInfo frameFenceCreateInfo{};
        frameFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        frameFenceCreateInfo.pNext = nullptr;
        frameFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (VK_FAILED(vkCreateFence(_device, &frameFenceCreateInfo, nullptr, &frameState.frameReadyFence)))
        {
            spdlog::error("Failed to create Vulkan frame fence");
            return false;
        }

        VkCommandPoolCreateInfo directCommandPoolCreateInfo{};
        directCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        directCommandPoolCreateInfo.pNext = nullptr;
        directCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        directCommandPoolCreateInfo.queueFamilyIndex = _physicalDeviceInfo.directQueueFamily;

        if (VK_FAILED(vkCreateCommandPool(_device, &directCommandPoolCreateInfo, nullptr, &frameState.directCommandPool)))
        {
            spdlog::error("Failed to create Vulkan frame command pool");
            return false;
        }

        VkCommandBufferAllocateInfo directCommandAllocateInfo{};
        directCommandAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        directCommandAllocateInfo.pNext = nullptr;
        directCommandAllocateInfo.commandPool = frameState.directCommandPool;
        directCommandAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        directCommandAllocateInfo.commandBufferCount = 1;

        if (VK_FAILED(vkAllocateCommandBuffers(_device, &directCommandAllocateInfo, &frameState.directCommandBuffer)))
        {
            spdlog::error("Failed to create Vulkan frame command buffer");
            return false;
        }
    }

    return true;
}

bool RenderManager::CreateVulkanWindowState(char const* title, uint32_t width, uint32_t height)
{
    // Create window
    spdlog::trace("Creating window");
    SDL_Window* window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN);
    if (!window)
    {
        spdlog::error("Failed to create window: {}", SDL_GetError());
        return false;
    }

    // Create surface
    spdlog::trace("Creating Vulkan surface");
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window, _instance, nullptr, &surface))
    {
        spdlog::error("Failed to create Vulkan surface");
        return false;
    }

    _windowState = VulkanWindowState{};
    _windowState.window = window;
    _windowState.surface = surface;
    if (!ConfigureSwapchain(_windowState, PREFERRED_SWAP_FORMAT, VK_PRESENT_MODE_FIFO_KHR))
    {
        spdlog::error("Failed to configure Vulkan swapchain");
        return false;
    }

    return true;
}

void RenderManager::DestroyVulkanInstance()
{
    spdlog::trace("Cleaning up instance state");

    if constexpr (RENDERER_ENABLE_DEBUG) {
        vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
    }
    vkDestroyInstance(_instance, nullptr);
}

void RenderManager::DestroyVulkanDevice()
{
    spdlog::trace("Cleaning up device state");

    vmaDestroyAllocator(_allocator);
    vkDestroyDevice(_device, nullptr);
    _directQueue = VK_NULL_HANDLE;
    _physicalDeviceInfo = {};
    _physicalDevice = VK_NULL_HANDLE;
}

void RenderManager::DestroyVulkanFrameState()
{
    spdlog::trace("Cleaning up frame state");

    for (auto const& frameState : _frameStates)
    {
        vkDestroyCommandPool(_device, frameState.directCommandPool, nullptr);
        vkDestroyFence(_device, frameState.frameReadyFence, nullptr);
    }

    _frameStates.clear();
    _currentFrameIndex = 0;
    _framesInFlight = 0;
}

void RenderManager::DestroyVulkanWindowState()
{
    spdlog::trace("Cleaning up window state");

    DestroySwapchainImageState(_windowState);
    vkDestroySwapchainKHR(_device, _windowState.swapchain, nullptr);
    vkDestroySurfaceKHR(_instance, _windowState.surface, nullptr);
    SDL_DestroyWindow(_windowState.window);
    _windowState = VulkanWindowState{};
}

RenderManager::VulkanSwapchainConfig RenderManager::GetVulkanSwapchainConfiguration(
    SDL_Window* window,
    VkSurfaceKHR surface,
    VkFormat preferredSurfaceFormat,
    VkPresentModeKHR preferredPresentMode
) const
{
    // Get window size in pixels
    int32_t windowWidth = 0, windowHeight = 0;
    SDL_GetWindowSizeInPixels(window, &windowWidth, &windowHeight);

    // Get surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, surface, &surfaceCapabilities);

    // Get surface formats and present modes
    std::vector<VkSurfaceFormatKHR> const availableSurfaceFormats = [](VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        uint32_t surfaceFormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data());
        return surfaceFormats;
    }(_physicalDevice, surface);
    std::vector<VkPresentModeKHR> const availablePresentModes = [](VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
        return presentModes;
    }(_physicalDevice, surface);

    // Find surface configuration that matches requirements
    uint32_t const preferredImageCount = surfaceCapabilities.minImageCount + 1;
    uint32_t const minImageCount = surfaceCapabilities.maxImageCount == 0 ? preferredImageCount : std::min(surfaceCapabilities.maxImageCount, preferredImageCount);

    VkSurfaceFormatKHR const swapSurfaceFormat = [&availableSurfaceFormats, &preferredSurfaceFormat]() {
        for (auto const& format : availableSurfaceFormats)
        {
            if (format.format == preferredSurfaceFormat) {
                return format;
            }
        }

        // TODO(nemjit001): Add better fallback handling for unsupported surface formats
        return availableSurfaceFormats[0]; // Return first available format
    }();

    VkPresentModeKHR const presentMode = [&availablePresentModes, &preferredPresentMode]() {
        for (auto const& presentMode : availablePresentModes)
        {
            if (presentMode == preferredPresentMode) {
                return presentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR; // Return fifo since it is always supported
    }();

    return VulkanSwapchainConfig{
        static_cast<uint32_t>(windowWidth),
        static_cast<uint32_t>(windowHeight),
        minImageCount,
        swapSurfaceFormat,
        presentMode,
        surfaceCapabilities.currentTransform,
    };
}

bool RenderManager::ConfigureSwapchain(VulkanWindowState& windowState, VkFormat preferredFormat, VkPresentModeKHR preferredPresentMode) const
{
    spdlog::trace("Configuring Vulkan swapchain");
    VkSwapchainKHR oldSwapchain = windowState.swapchain;

    // Create swapchain
    VulkanSwapchainConfig const swapchainConfig = GetVulkanSwapchainConfiguration(windowState.window, windowState.surface, preferredFormat, preferredPresentMode);
    spdlog::trace("Swapchain extent: {}x{}", swapchainConfig.width, swapchainConfig.height);

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = windowState.surface;
    swapchainCreateInfo.minImageCount = swapchainConfig.imageCount;
    swapchainCreateInfo.imageFormat = swapchainConfig.surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = swapchainConfig.surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = { swapchainConfig.width, swapchainConfig.height };
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    swapchainCreateInfo.preTransform = swapchainConfig.surfaceTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = swapchainConfig.presentMode;
    swapchainCreateInfo.clipped = VK_FALSE;
    swapchainCreateInfo.oldSwapchain = oldSwapchain;

    spdlog::trace("Creating Vulkan swapchain");
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    if (VK_FAILED(vkCreateSwapchainKHR(_device, &swapchainCreateInfo, nullptr, &swapchain)))
    {
        spdlog::error("Failed to create Vulkan swapchain");
        return false;
    }

    // Get swapchain images
    spdlog::trace("Creating Vulkan swapchain images");
    std::vector<VkImage> const swapImages = [](VkDevice device, VkSwapchainKHR swapchain) {
        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        std::vector<VkImage> swapImages(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapImages.data());
        return swapImages;
    }(_device, swapchain);

    // Create swapchain image views
    spdlog::trace("Creating Vulkan swapchain image views");
    std::vector<VkImageView> swapImageViews;
    swapImageViews.reserve(swapImages.size());
    for (auto const& image : swapImages)
    {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = swapchainConfig.surfaceFormat.format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VkImageView imageView = VK_NULL_HANDLE;
        if (VK_FAILED(vkCreateImageView(_device, &imageViewCreateInfo, nullptr, &imageView)))
        {
            spdlog::error("Failed to create Vulkan swapchain image view");
            return false;
        }

        swapImageViews.push_back(imageView);
    }

    // Create Vulkan swap semaphores
    std::vector<VkSemaphore> swapImageAcquiredSemaphores;
    swapImageAcquiredSemaphores.reserve(_framesInFlight);
    for (size_t i = 0; i < _framesInFlight; i++)
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = nullptr;
        semaphoreCreateInfo.flags = 0;

        VkSemaphore semaphore = VK_NULL_HANDLE;
        if (VK_FAILED(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &semaphore)))
        {
            spdlog::error("Failed to create Vulkan swapchain semaphore");
            return false;
        }

        swapImageAcquiredSemaphores.push_back(semaphore);
    }

    std::vector<VkSemaphore> swapImageReleasedSemaphores;
    swapImageReleasedSemaphores.reserve(swapImages.size());
    for (size_t i = 0; i < swapImages.size(); i++)
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = nullptr;
        semaphoreCreateInfo.flags = 0;

        VkSemaphore semaphore = VK_NULL_HANDLE;
        if (VK_FAILED(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &semaphore)))
        {
            spdlog::error("Failed to create Vulkan swapchain semaphore");
            return false;
        }

        swapImageReleasedSemaphores.push_back(semaphore);
    }

    // Delete old swapchain state on successful recreation
    DestroySwapchainImageState(windowState);
    vkDestroySwapchainKHR(_device, oldSwapchain, nullptr);

    // Set new swapchain state
    windowState.swapchain = swapchain;
    windowState.swapchainConfig = swapchainConfig;
    windowState.swapImages = swapImages;
    windowState.swapImageViews = swapImageViews;
    windowState.swapImageAcquiredSemaphores = swapImageAcquiredSemaphores;
    windowState.swapImageReleasedSemaphores = swapImageReleasedSemaphores;
    windowState.currentSwapImageIdx = 0;
    windowState.reconfigureSwapchain = false;
    return true;
}

void RenderManager::DestroySwapchainImageState(VulkanWindowState& windowState) const
{
    for (auto const& semaphore : windowState.swapImageAcquiredSemaphores) {
        vkDestroySemaphore(_device, semaphore, nullptr);
    }

    for (auto const& semaphore : windowState.swapImageReleasedSemaphores) {
        vkDestroySemaphore(_device, semaphore, nullptr);
    }

    for (auto const& view : windowState.swapImageViews) {
        vkDestroyImageView(_device, view, nullptr);
    }

    windowState.swapImageViews.clear();
    windowState.swapImages.clear();
    windowState.swapchainConfig = {};
}

bool RenderManager::AcquireNextSwapchainImage(VulkanWindowState& windowState) const
{
    // Handle queued reconfigure of swapchain
    if (_windowState.reconfigureSwapchain)
    {
        // TODO(nemjit001): Cache active present mode
        ConfigureSwapchain(windowState, PREFERRED_SWAP_FORMAT, VK_PRESENT_MODE_FIFO_KHR);
        return false;
    }

    // Acquire swap image for window
    VkResult const acquireResult = vkAcquireNextImageKHR(
        _device,
        windowState.swapchain,
        UINT64_MAX,
        windowState.swapImageAcquiredSemaphores[GetFrameInFlightIndex()],
        VK_NULL_HANDLE,
        &windowState.currentSwapImageIdx
    );
    if (VK_FAILED(acquireResult))
    {
        if (acquireResult == VK_SUBOPTIMAL_KHR || acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
            windowState.reconfigureSwapchain = true;
        }
        else {
            FATAL_ERROR("Failed to acquire Vulkan swapchain image for frame {} (result: {})", _currentFrameIndex, static_cast<uint32_t>(acquireResult));
        }

        return false;
    }

    return true;
}

void RenderManager::Present(VulkanWindowState& windowState) const
{
    // Present swap image to window
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &windowState.swapImageReleasedSemaphores[windowState.currentSwapImageIdx];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &windowState.swapchain;
    presentInfo.pImageIndices = &windowState.currentSwapImageIdx;
    presentInfo.pResults = nullptr;

    VkResult const presentResult = vkQueuePresentKHR(_directQueue, &presentInfo);
    if (VK_FAILED(presentResult))
    {
        if (presentResult == VK_SUBOPTIMAL_KHR || presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
            windowState.reconfigureSwapchain = true;
        }
        else {
            FATAL_ERROR("Failed to present Vulkan swapchain image {} for frame {} (result: {})", windowState.currentSwapImageIdx, _currentFrameIndex, static_cast<uint32_t>(presentResult));
        }
    }
}

void RenderManager::WaitIdle() const
{
    vkDeviceWaitIdle(_device);
}
