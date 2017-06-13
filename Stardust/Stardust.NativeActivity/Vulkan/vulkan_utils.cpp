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
