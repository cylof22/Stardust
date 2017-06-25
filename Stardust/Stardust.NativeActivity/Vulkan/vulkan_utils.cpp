#include <stdio.h>
#include <stdlib.h>
#include "vulkan_utils.h"
#include "vulkan_wrapper.h"

uint32_t get_mem_type_index(VkPhysicalDevice gpu, VkMemoryPropertyFlagBits bit)
{
	VkPhysicalDeviceMemoryProperties physMemProperties;
	vkGetPhysicalDeviceMemoryProperties(gpu, &physMemProperties);

	for (uint32_t i = 0; i < physMemProperties.memoryTypeCount; ++i) {
		if ((physMemProperties.memoryTypes[i].propertyFlags & bit) == bit) {
			return  i;
		}
	}
	return 0;
}
//=============================================================================
int VKU_Create_Buffer_Memory_Pool(
	VkDevice                   device,
	VkQueue                    queue,
	const VkMemoryAllocateInfo *alloc,
	VKU_BUFFER_MEMORY_POOL     **mempool)
{
	if (!device || !alloc || !mempool)
		return 0;

	VkDeviceMemory mem;
	VK_VALIDATION_RESULT(vkAllocateMemory(device, alloc, VK_ALLOC_CALLBACK, &mem));

	VKU_BUFFER_MEMORY_POOL *mpool = (VKU_BUFFER_MEMORY_POOL*)malloc(sizeof(*mpool));
	if (!mpool) {
		if (mem) { vkFreeMemory(device, mem, VK_ALLOC_CALLBACK); mem = VK_NULL_HANDLE; }
		return 0;
	}

	mpool->device = device;
	mpool->queue = queue;
	mpool->memory = mem;
	mpool->offset = 0;
	mpool->size = alloc->allocationSize;
	mpool->buffers = NULL;

	*mempool = mpool;

	return 1;
}
//-----------------------------------------------------------------------------
int VKU_Create_Image_Memory_Pool(
	VkDevice                   device,
	VkQueue                    queue,
	const VkMemoryAllocateInfo *alloc,
	VKU_IMAGE_MEMORY_POOL      **mempool)
{
	if (!device || !alloc || !mempool)
		return 0;

	VkDeviceMemory mem;
	VK_VALIDATION_RESULT(vkAllocateMemory(device, alloc, VK_ALLOC_CALLBACK, &mem));

	VKU_IMAGE_MEMORY_POOL *mpool = (VKU_IMAGE_MEMORY_POOL*)malloc(sizeof(*mpool));
	if (!mpool) {
		if (mem) { vkFreeMemory(device, mem, VK_ALLOC_CALLBACK); mem = VK_NULL_HANDLE; }
		return 0;
	}

	mpool->device = device;
	mpool->queue = queue;
	mpool->memory = mem;
	mpool->offset = 0;
	mpool->size = alloc->allocationSize;
	mpool->images = NULL;

	*mempool = mpool;

	return 1;
}
//-----------------------------------------------------------------------------
int VKU_Alloc_Image_Memory(
	VKU_IMAGE_MEMORY_POOL *mempool,
	VkDeviceSize           size,
	VkDeviceSize           alignment,
	VkDeviceSize          *offset)
{
	if (!mempool || size == 0 || !offset)
		return 0;

	if (alignment == 0) alignment = 1;

	VkDeviceSize r = mempool->offset % alignment;
	VkDeviceSize offset_change = r > 0 ? alignment - r : 0;
	if (mempool->offset + offset_change + size > mempool->size)
		return 0;

	mempool->offset += offset_change;
	*offset = mempool->offset;
	mempool->offset += size;

	return 1;
}
//-----------------------------------------------------------------------------
int VKU_Alloc_Image_Object(VKU_IMAGE_MEMORY_POOL *mempool,
	VkImage           image,
	VkDeviceSize     *offset,
	uint32_t          memtypeindex)
{
	if (!mempool || !image)
		return 0;

	VkMemoryRequirements mreq;
	vkGetImageMemoryRequirements(mempool->device, image, &mreq);

	if (mreq.size > 0) {
		VkDeviceSize off;
		if (!VKU_Alloc_Image_Memory(mempool, mreq.size, mreq.alignment, &off))
			return 0;

		VkMemoryAllocateInfo mem_alloc;
		mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		mem_alloc.pNext = NULL;
		mem_alloc.allocationSize = mreq.size;
		mem_alloc.memoryTypeIndex = memtypeindex;

		VK_VALIDATION_RESULT(vkAllocateMemory(mempool->device, &mem_alloc, NULL, &mempool->memory));
		VK_VALIDATION_RESULT(vkBindImageMemory(mempool->device, image, mempool->memory, 0));

		// Todo: how to add additional memory flag to the allocated memory
		//sb_push(mempool->images, image);
		if (offset) *offset = off;
	}

	return 1;
}

//=============================================================================
int VKU_Load_Shader(AAssetManager * pMgr, VkDevice device, const char * filename, VkShaderModule * shaderModule)
{
	if (!device || !filename || !shaderModule)
		return 0;

	AAsset* pAsset = AAssetManager_open(pMgr, filename, AASSET_MODE_UNKNOWN);
	if (!pAsset)
		return 0;

	size_t size = AAsset_getLength(pAsset);

	unsigned char *bytecode = (unsigned char*)malloc(size);
	off_t readSize = AAsset_read(pAsset, (void*)bytecode, size);
	assert(size == readSize);

	if (!bytecode) return 0;

	AAsset_close(pAsset);

	VkShaderModule sm;
	const VkShaderModuleCreateInfo shader_module_info = {
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0, size, (uint32_t*)bytecode
	};

	VkResult res = vkCreateShaderModule(device, &shader_module_info, VK_ALLOC_CALLBACK, &sm);
	if (res != VK_SUCCESS) {
		free(bytecode);
		return 0;
	}

	free(bytecode);
	*shaderModule = sm;

	return 1;
}

stbi_uc* load_image(AAssetManager * pMgr, const char * fileName, int * x, int * y, int * comp, int req_comp)
{
	AAsset* pAsset = AAssetManager_open(pMgr, fileName, AASSET_MODE_UNKNOWN);
	if (pAsset == nullptr)
		return nullptr;

	size_t size = AAsset_getLength(pAsset);
	if (size == 0)
		return nullptr;

	unsigned char* imageData = (unsigned char*)malloc(size);
	off_t readSize = AAsset_read(pAsset, (void*)imageData, size);
	assert(size = readSize);

	return stbi_load_from_memory(imageData, readSize, x, y, comp, req_comp);
}

//int VKU_Compile_Shader(AAssetManager * pMgr, VkDevice device, const char * fileName, shaderc_shader_kind kind, VkShaderModule * shaderModule)
//{
//	AAsset* pAsset = AAssetManager_open(pMgr, fileName, AASSET_MODE_UNKNOWN);
//	if (!pAsset)
//		return 0;
//
//	size_t size = AAsset_getLength(pAsset);
//
//	char *glslShader = (char*)malloc(size);
//	off_t readSize = AAsset_read(pAsset, (void*)glslShader, size);
//	assert(size == readSize);
//
//	if (!glslShader) return 0;
//
//	AAsset_close(pAsset);
//
//	// convert the file to Vulkan spv format
//	shaderc::Compiler compiler;
//	shaderc::CompileOptions options;
//
//	shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
//		glslShader, readSize, kind, fileName, options);
//
//	shaderc_compilation_status compileRes = module.GetCompilationStatus();
//	const char* errorInfo = nullptr;
//	//assert(compileRes == shaderc_compilation_status_success);
//	if (compileRes == shaderc_compilation_status_success) {
//		std::vector<uint32_t> result(module.cbegin(), module.cend());
//
//		VkResult res;
//		// create the shaderModule
//		VkShaderModuleCreateInfo info = {};
//		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//		info.pNext = nullptr;
//		info.flags = 0;
//		info.codeSize = result.size() * sizeof(uint32_t);
//		info.pCode = result.data();
//
//		VkShaderModule shaderModule = VK_NULL_HANDLE;
//		res = vkCreateShaderModule(device, &info, nullptr, &shaderModule);
//
//		return 1;
//	}
//	return 0;
//}
