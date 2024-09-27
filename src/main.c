#include "utils.h"
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

    const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .ppEnabledLayerNames = VALIDATION_LAYERS,
        .enabledLayerCount = ARRAY_LENGTH(VALIDATION_LAYERS),
        .pQueueCreateInfos = &queue_create_info,
        .queueCreateInfoCount = 1,
    };
    
    VkDevice device;
    VkResult result = vkCreateDevice(physical_device, &device_info, NULL, &device);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create device : %s\n", string_VkResult(result));
        return NULL;
    }

    return device;
}

static uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    fprintf(stderr, "Failed to find suitable memory type\n");
    exit(EXIT_FAILURE);
}

static VkImage create_image(VkDevice device) {
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
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkImage image;
    VkResult result = vkCreateImage(device, &image_info, NULL, &image);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create image: %s\n", string_VkResult(result));
        return NULL;
    }

    return image;
}

static VkImageView create_image_view(VkDevice device, VkImage image) {
    const VkImageViewCreateInfo image_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };

    VkImageView image_view;
    VkResult result = vkCreateImageView(device, &image_view_info, NULL, &image_view);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create image view: %s\n", string_VkResult(result));
        return NULL;
    }
    
    return image_view;
}

static VkDeviceMemory allocate_image(VkPhysicalDevice physical_device, VkDevice device, VkImage image) {
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, image, &memory_requirements);

    const VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = find_memory_type(physical_device, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    VkDeviceMemory image_memory;
    VkResult result = vkAllocateMemory(device, &alloc_info, NULL, &image_memory);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate memory for image: %s\n", string_VkResult(result));
        return NULL;
    }

    VkResult bind_result = vkBindImageMemory(device, image, image_memory, 0);
    if(bind_result != VK_SUCCESS) {
        fprintf(stderr, "Failed to bind image memory: %s\n", string_VkResult(bind_result));
        return NULL;
    }

    return image_memory;
}

static VkDescriptorSetLayout create_descriptor_set_layout(VkDevice device) {
    const VkDescriptorSetLayoutBinding image_layout_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    };

    const VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &image_layout_binding
    };

    VkDescriptorSetLayout descriptor_set_layout;
    VkResult result = vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, NULL, &descriptor_set_layout);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor set layout: %s\n", string_VkResult(result));
        return NULL;
    }

    return descriptor_set_layout;
}

static VkPipelineLayout create_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_layout) {
    const VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_layout,
    };

    VkPipelineLayout pipeline_layout;
    VkResult result = vkCreatePipelineLayout(device, &pipeline_layout_info, NULL, &pipeline_layout);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout: %s\n", string_VkResult(result));
        return NULL;
    }

    return pipeline_layout;
}

static VkDescriptorPool create_descriptor_pool(VkDevice device) {
    const VkDescriptorPoolSize pool_size = {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
    };

    const VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_size,
        .maxSets = 1
    };

    VkDescriptorPool descriptor_pool;
    VkResult result = vkCreateDescriptorPool(device, &pool_info, NULL, &descriptor_pool);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor pool: %s\n", string_VkResult(result));
        return NULL;
    }

    return descriptor_pool;
}

static VkDescriptorSet create_descriptor_set(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout layout) {
    const VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet descriptor_set;
    VkResult result = vkAllocateDescriptorSets(device, &alloc_info, &descriptor_set);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor set: %s\n", string_VkResult(result));
        return NULL;
    }

    return descriptor_set;
}

static VkCommandPool create_command_pool(VkDevice device, uint32_t compute_queue_index) {
    const VkCommandPoolCreateInfo command_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = compute_queue_index,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };

    VkCommandPool command_pool;
    VkResult result = vkCreateCommandPool(device, &command_pool_info, NULL, &command_pool);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool: %s\n", string_VkResult(result));
        return NULL;
    }

    return command_pool;
}

static VkCommandBuffer create_command_buffer(VkDevice device, VkCommandPool command_pool) {
    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer buffer;
    VkResult result = vkAllocateCommandBuffers(device, &alloc_info, &buffer);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command buffer: %s\n", string_VkResult(result));
        return NULL;
    }

    return buffer;
}

static VkPipeline create_compute_pipeline(VkDevice device, VkPipelineLayout pipeline_layout, VkShaderModule shader) {
    const VkPipelineShaderStageCreateInfo compute_shader_stage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader,
        .pName = "main"
    };

    const VkComputePipelineCreateInfo compute_pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .layout = pipeline_layout,
        .stage = compute_shader_stage,
    };
    
    VkPipeline pipeline;
    VkResult result = vkCreateComputePipelines(device, NULL, 1, &compute_pipeline_info, NULL, &pipeline);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create compute pipeline: %s\n", string_VkResult(result));
        return NULL;
    }

    return pipeline;
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

    VkDeviceMemory image_memory = allocate_image(physical_device, device, image);
    if(!image_memory) {
        fprintf(stderr, "Cannot proceed without allocated image memory");
        return EXIT_FAILURE;
    }

    VkImageView image_view = create_image_view(device, image);
    if(!image_view) {
        fprintf(stderr, "Cannot proceed without an image view");
        return EXIT_FAILURE;
    }

    VkDescriptorSetLayout descriptor_set_layout = create_descriptor_set_layout(device);
    if(!descriptor_set_layout) {
        fprintf(stderr, "Cannot proceed without a descriptor set layout");
        return EXIT_FAILURE;
    }

    VkPipelineLayout pipeline_layout = create_pipeline_layout(device, descriptor_set_layout);
    if(!pipeline_layout) {
        fprintf(stderr, "Cannot proceed without a pipeline layout");
        return EXIT_FAILURE;
    }

    VkDescriptorPool descriptor_pool = create_descriptor_pool(device);
    if(!descriptor_pool)  {
        fprintf(stderr, "Cannot proceed without a descriptor pool");
        return EXIT_FAILURE;
    }

    VkDescriptorSet descriptor_set = create_descriptor_set(device, descriptor_pool, descriptor_set_layout);
    if(!descriptor_set) {
        fprintf(stderr, "Cannot proceed without a descriptor set");
        return EXIT_FAILURE;
    }

    const VkDescriptorImageInfo descriptor_image_info = {
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .imageView = image_view,
    };

    const VkWriteDescriptorSet descriptor_write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .pImageInfo = &descriptor_image_info,
    };

    vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, NULL);

    VkCommandPool command_pool = create_command_pool(device, compute_queue_index);
    if(!command_pool) {
        fprintf(stderr, "Cannot proceed without a command pool");
        return EXIT_FAILURE;
    }

    VkCommandBuffer command_buffer = create_command_buffer(device, command_pool);
    if(!command_buffer) {
        fprintf(stderr, "Cannot proceed without a command buffer");
        return EXIT_FAILURE;
    }

    VkFence compute_completed_fence;
    {
        const VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        };

        VkResult result = vkCreateFence(device, &fence_info, NULL, &compute_completed_fence);
        if(result != VK_SUCCESS) {
            fprintf(stderr, "Failed to create fence");
            return EXIT_FAILURE;
        }
    }

    size_t shader_code_len;
    const uint8_t *shader_code = read_file("shaders/pathtracer.comp.spv", &shader_code_len);

    const VkShaderModuleCreateInfo shader_mod_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pCode = (uint32_t*)shader_code,
        .codeSize = shader_code_len,
    };

    VkShaderModule shader_mod;
    {
        VkResult result = vkCreateShaderModule(device, &shader_mod_info, NULL, &shader_mod);
        if(result != VK_SUCCESS) {
            fprintf(stderr, "Failed to create shader module: %s\n", string_VkResult(result));
            return EXIT_FAILURE;
        }
    }

    VkPipeline pipeline = create_compute_pipeline(device, pipeline_layout, shader_mod);
    if(!pipeline) {
        fprintf(stderr, "Cannot proceed without a pipeline");
        return EXIT_FAILURE;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };

    if(vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        fprintf(stderr, "Failed to begin recording command buffers");
        return EXIT_FAILURE;
    }

    const VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
    };

    vkCmdPipelineBarrier(
        command_buffer, 
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
        0, 
        0, NULL, 
        0, NULL, 
        1, &barrier
    );

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
    vkCmdDispatch(command_buffer, (IMAGE_WIDTH + 7) / 8, (IMAGE_HEIGHT + 7) / 8, 1);

    if(vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to end recording command buffers");
        return EXIT_FAILURE;
    }

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };

    VkResult submit_result = vkQueueSubmit(compute_queue, 1, &submit_info, compute_completed_fence);
    if(submit_result != VK_SUCCESS) {
        fprintf(stderr, "Failed to submit command buffers");
        return EXIT_FAILURE;
    }

    VkResult wait_result = vkWaitForFences(device, 1, &compute_completed_fence, VK_TRUE, UINT64_MAX);
    if(wait_result != VK_SUCCESS) {
        fprintf(stderr, "Failed to wait for fences");
        return EXIT_FAILURE;
    }
    
    vkDestroyShaderModule(device, shader_mod, NULL);
    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyFence(device, compute_completed_fence, NULL);
    vkDestroyCommandPool(device, command_pool, NULL);
    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);
    vkDestroyImageView(device, image_view, NULL);
    vkFreeMemory(device, image_memory, NULL);
    vkDestroyImage(device, image, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroyDebugUtilsMessengerEXT(instance, messenger, NULL);
    vkDestroyInstance(instance, NULL);
    return EXIT_SUCCESS;
}
