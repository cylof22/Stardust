#pragma once
#include <cassert>
#include "Include\vulkan.h"

#define VK_ALLOC_CALLBACK (VkAllocationCallbacks*)NULL
#define VK_PAGE_SIZE (1024*64)
#define VK_ALIGN(v, a) (((v) % (a)) ? ((v) + ((a) - ((v) % (a)))) : (v))
#define VK_ALIGN_PAGE(v) VK_ALIGN(v, VK_PAGE_SIZE)
#define VK_VALIDATION_RESULT(res) { VkResult _res = (res); assert(_res); }
#define VK_LOG(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "VULKAN_APP", __VA_ARGS__))

#if defined(_WIN32) || defined(_WIN64)
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_KHR_PLATFORM_SPECIFIC_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(__ANDROID__)
#define VK_KHR_PLATFORM_SPECIFIC_SURFACE_EXTENSION_NAME VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#endif

//==========================================================================
uint32_t get_mem_type_index(VkPhysicalDevice gpu, VkMemoryPropertyFlagBits bit);
//==========================================================================