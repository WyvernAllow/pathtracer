#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

static VKAPI_ATTR VkBool32 debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
    void*                                            pUserData) {

    if(!pCallbackData->pMessage) {
        return VK_FALSE;
    }

    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        fprintf(stderr, "Trace: %s\n", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        fprintf(stderr, "Info: %s\n", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        fprintf(stderr, "Warn: %s\n", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        fprintf(stderr, "Error: %s\n", pCallbackData->pMessage);
        break;
    default:
        break;
    }

    return VK_FALSE;
}

static VkInstance create_instance(const VkDebugUtilsMessengerCreateInfoEXT *debug_info) {
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_0,
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .pApplicationName = NULL,
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .pEngineName = NULL,
    };

    const char *layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    const char *extensions[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    const VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = debug_info,
        .pApplicationInfo = &app_info,
        .ppEnabledLayerNames = layers,
        .enabledLayerCount = ARRAY_LENGTH(layers),
        .ppEnabledExtensionNames = extensions,
        .enabledExtensionCount = ARRAY_LENGTH(extensions),
    };

    VkInstance instance;
    VkResult result = vkCreateInstance(&instance_info, NULL, &instance);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create instance: %s\n", string_VkResult(result));
        return NULL;
    }

    return instance;
}

static VkDebugUtilsMessengerEXT create_messenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *debug_info) {
    VkDebugUtilsMessengerEXT messenger;
    VkResult result = vkCreateDebugUtilsMessengerEXT(instance, debug_info, NULL, &messenger);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create debug utils messenger: %s\n", string_VkResult(result));
        return NULL;
    }

    return messenger;
}

static int rate_physical_device(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);

    int score = 0;
    if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 100;
    }

    return score;
}

static VkPhysicalDevice find_physical_device(VkInstance instance) {
    uint32_t physical_device_count;
    vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL);

    VkPhysicalDevice *physical_devices = malloc(sizeof(VkPhysicalDevice) * physical_device_count);
    if(!physical_devices) {
        fprintf(stderr, "Failed to find a physical device: out of memory");
        return NULL;
    }

    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);

    int highest_score = INT_MIN;
    VkPhysicalDevice best_physical_device = NULL;

    for(uint32_t i = 0; i < physical_device_count; i++) {
        VkPhysicalDevice physical_device = physical_devices[i];
        int score = rate_physical_device(physical_device);
        if(score > highest_score) {
            highest_score = score;
            best_physical_device = physical_device;
        }
    }

    free(physical_devices);
    return best_physical_device;
}

int main() {
    const VkDebugUtilsMessengerCreateInfoEXT debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pfnUserCallback = debug_callback,
        .messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    };

    VkInstance instance = create_instance(&debug_info);
    if(!instance) {
        fprintf(stderr, "Cannot proceed without a valid instance");
        return EXIT_FAILURE;
    }

    VkDebugUtilsMessengerEXT messenger = create_messenger(instance, &debug_info);
    if(!messenger) {
        fprintf(stderr, "Cannot proceed without a debug utils messenger");
        return EXIT_FAILURE;
    }

    VkPhysicalDevice physical_device = find_physical_device(instance);
    if(!physical_device) {
        fprintf(stderr, "Failed to find a suitable physical device");
        return EXIT_FAILURE;
    }

    
    
    vkDestroyDebugUtilsMessengerEXT(instance, messenger, NULL);
    vkDestroyInstance(instance, NULL);
    return EXIT_SUCCESS;
}
