#include "RenderManager.hpp"

#include <spdlog/spdlog.h>
#include <SDL3/SDL_vulkan.h>

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

    return true;
}

void RenderManager::Shutdown()
{
    DestroyVulkanDevice();
    DestroyVulkanInstance();

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

    return true;
}

void RenderManager::DestroyVulkanInstance()
{
    spdlog::trace("Cleaning up instance data");
    if constexpr (RENDERER_ENABLE_DEBUG) {
        vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
    }
    vkDestroyInstance(_instance, nullptr);
}

void RenderManager::DestroyVulkanDevice()
{
    spdlog::trace("Cleaning up device data");
    vkDestroyDevice(_device, nullptr);
    _directQueue = VK_NULL_HANDLE;
    _physicalDeviceInfo = {};
    _physicalDevice = VK_NULL_HANDLE;
}
