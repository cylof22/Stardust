#pragma once
#include <cassert>
#include <android\asset_manager.h>
#include "Include\vulkan.h"

#define VK_ALLOC_CALLBACK (VkAllocationCallbacks*)NULL
#define VK_PAGE_SIZE (1024*64)
#define VK_ALIGN(v, a) (((v) % (a)) ? ((v) + ((a) - ((v) % (a)))) : (v))
#define VK_ALIGN_PAGE(v) VK_ALIGN(v, VK_PAGE_SIZE)
#define VK_VALIDATION_RESULT(res) { assert(res == VK_SUCCESS); }
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

typedef struct VKU_BUFFER_MEMORY_POOL
{
	VkDevice       device;
	VkQueue        queue;
	VkDeviceMemory memory;
	VkDeviceSize   size;
	VkDeviceSize   offset;
	VkBuffer      *buffers;
} VKU_BUFFER_MEMORY_POOL;

typedef struct VKU_IMAGE_MEMORY_POOL
{
	VkDevice       device;
	VkQueue        queue;
	VkDeviceMemory memory;
	VkDeviceSize   size;
	VkDeviceSize   offset;
	VkImage       *images;
} VKU_IMAGE_MEMORY_POOL;

int VKU_Create_Buffer_Memory_Pool(VkDevice                   device,
	VkQueue                    queue,
	const VkMemoryAllocateInfo *alloc,
	VKU_BUFFER_MEMORY_POOL     **mempool);

int VKU_Create_Image_Memory_Pool(VkDevice                   device,
	VkQueue                    queue,
	const VkMemoryAllocateInfo *alloc,
	VKU_IMAGE_MEMORY_POOL      **mempool);

int VKU_Alloc_Image_Memory(VKU_IMAGE_MEMORY_POOL *mempool,
	VkDeviceSize          size,
	VkDeviceSize          alignment,
	VkDeviceSize          *offset);

int VKU_Alloc_Image_Object(VKU_IMAGE_MEMORY_POOL *objpool,
	VkImage               image,
	VkDeviceSize          *offset,
	uint32_t              memtypeindex);

int VKU_Load_Shader(AAssetManager* pMgr, VkDevice device,
	const char      *filename,
	VkShaderModule  *shaderModule);