#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

static const char *VALIDATION_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation",
};

static const char *EXTENSIONS[] = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
};

static const uint32_t IMAGE_WIDTH = 4096;
static const uint32_t IMAGE_HEIGHT = 4096;

static VKAPI_ATTR VkBool32 debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
    void*                                            pUserData) {

    // pMessage is NULL if messageTypes is equal to VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT.
    if(!pCallbackData->pMessage) {
        return VK_FALSE;
    }

    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        fprintf(stderr, "%s\n", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        fprintf(stderr, "%s\n", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        fprintf(stderr, "%s\n", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        fprintf(stderr, "%s\n", pCallbackData->pMessage);
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

    const VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = debug_info,
        .pApplicationInfo = &app_info,
        .ppEnabledLayerNames = VALIDATION_LAYERS,
        .enabledLayerCount = ARRAY_LENGTH(VALIDATION_LAYERS),
        .ppEnabledExtensionNames = EXTENSIONS,
        .enabledExtensionCount = ARRAY_LENGTH(EXTENSIONS),
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

static uint32_t find_compute_family(VkPhysicalDevice physical_device) {
    uint32_t property_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &property_count, NULL);

    VkQueueFamilyProperties *properties = malloc(sizeof(VkQueueFamilyProperties) * property_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &property_count, properties);

    for(uint32_t i = 0; i < property_count; i++) {
        VkQueueFamilyProperties queue = properties[i];
        if(queue.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            free(properties);
            return i;
        }
    }

    // All Vulkan implementations are required to support compute, so this code should be unreachable.
    free(properties);
    fprintf(stderr, "Error: No compute queue family found!\n");
    exit(EXIT_FAILURE);
}

static VkDevice create_device(VkPhysicalDevice physical_device, uint32_t compute_queue) {
    const float queue_priorities = 1.0f;
    const VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount = 1,
        .queueFamilyIndex = compute_queue,
        .pQueuePriorities = &queue_priorities,
    };

    const VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .ppEnabledLayerNames = VALIDATION_LAYERS,
        .enabledLayerCount = ARRAY_LENGTH(VALIDATION_LAYERS),
        .pQueueCreateInfos = &queue_create_info,
        .queueCreateInfoCount = 1,
    };
    
    VkDevice device;
    VkResult result = vkCreateDevice(physical_device, &device_create_info, NULL, &device);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create device utils messenger: %s\n", string_VkResult(result));
        return NULL;
    }

    return device;
}

VkImage create_image(VkDevice device) {
    const VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent.width = IMAGE_WIDTH,
        .extent.height = IMAGE_HEIGHT,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
    };

    VkImage image;
    VkResult result = vkCreateImage(device, &image_info, NULL, &image);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create image: %s\n", string_VkResult(result));
        return NULL;
    }

    return image;
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
            //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
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

    uint32_t compute_queue_index = find_compute_family(physical_device);
    VkDevice device = create_device(physical_device, compute_queue_index);
    if(!device) {
        fprintf(stderr, "Cannot proceed without a device");
        return EXIT_FAILURE;
    }
    
    VkQueue compute_queue;
    vkGetDeviceQueue(device, compute_queue_index, 0, &compute_queue);

    VkImage image = create_image(device);
    if(!image) {
        fprintf(stderr, "Cannot proceed without an image");
        return EXIT_FAILURE;
    }
    
    vkDestroyImage(device, image, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroyDebugUtilsMessengerEXT(instance, messenger, NULL);
    vkDestroyInstance(instance, NULL);
    return EXIT_SUCCESS;
}
