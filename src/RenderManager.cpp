#include "RenderManager.hpp"

#include <vector>
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

    // Create instance
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
    }

    return true;
}

void RenderManager::Shutdown()
{
    spdlog::trace("Cleaning up instance data");
    if constexpr (RENDERER_ENABLE_DEBUG) {
        vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
    }
    vkDestroyInstance(_instance, nullptr);

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
