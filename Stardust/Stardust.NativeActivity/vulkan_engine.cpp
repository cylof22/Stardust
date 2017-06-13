#include "vulkan_engine.h"
#include "vulkan\vulkan_wrapper.h"
#include "Vulkan\vulkan_utils.h"
#include <array>

int init_gpu_instance()
{
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = "Stardust";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Starduster1";
	app_info.engineVersion = VK_MAKE_VERSION(1, 1, 0);
	app_info.apiVersion = VK_API_VERSION;

	std::array<const char*, 2> wsi_extensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_PLATFORM_SPECIFIC_SURFACE_EXTENSION_NAME
	};

	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pNext = NULL;
	instance_info.flags = 0;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledLayerCount = 0;
	instance_info.ppEnabledLayerNames = NULL;
	instance_info.enabledExtensionCount = wsi_extensions.size();
	instance_info.ppEnabledExtensionNames = wsi_extensions.data();

	VK_VALIDATION_RESULT(vkCreateInstance(&instance_info, VK_ALLOC_CALLBACK, &s_instance));

	uint32_t count = 1;
	VK_VALIDATION_RESULT(vkEnumeratePhysicalDevices(s_instance, &count, &s_gpu));

	std::array<const char*, 1> extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queue_info;
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	queue_info.pNext = NULL;
	queue_info.flags = 0;
	queue_info.queueFamilyIndex = 0;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = &queuePriority;

	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pNext = NULL;
	device_info.flags = 0;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.enabledLayerCount = 0;
	device_info.ppEnabledLayerNames = NULL;
	device_info.enabledExtensionCount = extensions.size();
	device_info.ppEnabledExtensionNames = extensions.data();
	device_info.pEnabledFeatures = NULL;

	VK_VALIDATION_RESULT(vkCreateDevice(s_gpu, &device_info, VK_ALLOC_CALLBACK, &s_gpu_device));

	uint32_t queuesCount;
	vkGetPhysicalDeviceQueueFamilyProperties(s_gpu, &queuesCount, NULL);

	VkQueueFamilyProperties *queueFamilyProperties =
		(VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queuesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(s_gpu, &queuesCount, queueFamilyProperties);

	for (uint32_t i = 0; i < queuesCount; i++) {
		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 &&
			(queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
			s_queue_family_index = i;
	}

	return 1;
}

int init_device(void* pWnd, int width, int height, VkBool32 windowed)
{
	init_gpu_instance();

	vkGetDeviceQueue(s_gpu_device, s_queue_family_index, 0, &s_gpu_queue);

	if (!init_framebuffer(pWnd, width, height, windowed, k_Window_Buffering, s_win_images))
		return 0;

	return 0;
}

void deinit(void)
{
	if (s_gpu_device) vkDeviceWaitIdle(s_gpu_device);

	if (s_swap_chain) {
		vkDestroySwapchainKHR(s_gpu_device, s_swap_chain, VK_ALLOC_CALLBACK);
		s_swap_chain = VK_NULL_HANDLE;
	}
	if (s_gpu_device) {
		vkDestroyDevice(s_gpu_device, VK_ALLOC_CALLBACK);
		s_gpu_device = VK_NULL_HANDLE;
	}
	if (s_instance) {
		vkDestroyInstance(s_instance, VK_ALLOC_CALLBACK);
		s_instance = VK_NULL_HANDLE;
	}
}

int init_framebuffer(void* pWnd, int width, int height, VkBool32 windowed, uint32_t image_count, VkImage *images)
{
	VkFormat image_format = VK_FORMAT_UNDEFINED;

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	const VkAndroidSurfaceCreateInfoKHR androidSurfaceCreateInfo = {
		VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, NULL, 0, (ANativeWindow*)pWnd
	};
	VK_VALIDATION_RESULT(vkCreateAndroidSurfaceKHR(s_instance, &androidSurfaceCreateInfo, VK_ALLOC_CALLBACK, &s_surface));
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
	if (!vkGetPhysicalDeviceWin32PresentationSupportKHR(s_gpu, s_queue_family_index)) {
		Log("vkGetPhysicalDeviceWin32PresentationSupportKHR returned FALSE, %d queue index does not support presentation\n", s_queue_family_index);
		LOG_AND_RETURN0();
	}
	HINSTANCE hinstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	const VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {
		VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, NULL, 0, hinstance, hwnd
	};
	VKU_VR(vkCreateWin32SurfaceKHR(s_instance, &win32SurfaceCreateInfo, NO_ALLOC_CALLBACK, &s_surface));
#endif

	VkBool32 surface_supported;
	VK_VALIDATION_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(s_gpu, s_queue_family_index, s_surface, &surface_supported));
	if (!surface_supported) {
		VK_LOG("vkGetPhysicalDeviceSurfaceSupportKHR returned FALSE, %d queue index does not support given surface\n", s_queue_family_index);
		return 0;
	}

	// Surface capabilities
	VkSurfaceCapabilitiesKHR surface_caps;
	VK_VALIDATION_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s_gpu, s_surface, &surface_caps));

	if (surface_caps.maxImageCount != 0 && image_count > surface_caps.maxImageCount) {
		image_count = surface_caps.maxImageCount;
		VK_LOG("Surface could not support requested image count. Got maximum possilble: %d\n", surface_caps.maxImageCount);
	}

	if (!(surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
		return 0;

	// Surface formats
	uint32_t formats_count;
	VK_VALIDATION_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(s_gpu, s_surface, &formats_count, NULL));
	VkSurfaceFormatKHR *surface_formats = (VkSurfaceFormatKHR *)malloc(sizeof(VkSurfaceFormatKHR) * formats_count);
	VK_VALIDATION_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(s_gpu, s_surface, &formats_count, surface_formats));

	VkSurfaceFormatKHR *chosen_surface_format = NULL;
	for (uint32_t i = 0; i < formats_count; i++) {
		if (surface_formats[i].format == VK_FORMAT_R8G8B8A8_UNORM && surface_formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
			chosen_surface_format = &surface_formats[i];
			break;
		}
	}

	if (!chosen_surface_format && formats_count) {
		VK_LOG("VK_FORMAT_R8G8B8A8_UNORM (SRGB_NONLINEAR) not found in supported surface formats, will use any available\n");
		chosen_surface_format = &surface_formats[0];
	}
	if (!chosen_surface_format) {
		VK_LOG("No supported VkSurface format\n");
		return 0;
	}

	image_format = chosen_surface_format->format;
	if (!windowed && !(image_format >= VK_FORMAT_R8G8B8A8_UNORM && image_format <= VK_FORMAT_R8G8B8A8_SRGB)) {
		VK_LOG("Swap chain image format's Bits Per Pixel != 32, fullscreen not supported\n");
		return 0;
	}

	// Surface present modes
	VkPresentModeKHR *chosen_present_mode = NULL;
	uint32_t present_mode_count = 0;
	VK_VALIDATION_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(s_gpu, s_surface, &present_mode_count, NULL));
	VkPresentModeKHR *present_modes = (VkPresentModeKHR *)malloc(sizeof(VkPresentModeKHR) * present_mode_count);
	VK_VALIDATION_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(s_gpu, s_surface, &present_mode_count, present_modes));

	for (uint32_t i = 0; i< present_mode_count; i++) {
		if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR || present_modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
			chosen_present_mode = &present_modes[i];
			break;
		}
	}

	if (!chosen_present_mode && present_mode_count) {
		VK_LOG("No supported present modes found, will use any available\n");
		chosen_present_mode = &present_modes[0];
	}
	if (!chosen_present_mode) {
		VK_LOG("No supported present modes\n");
		return 0;
	}

	// Create VkSwapChain
	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.surface = s_surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = image_format;
	create_info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	create_info.imageExtent.width = (int32_t)width;
	create_info.imageExtent.height = (int32_t)height;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 1;
	create_info.pQueueFamilyIndices = &s_queue_family_index;
	create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = *chosen_present_mode;
	create_info.clipped = VK_FALSE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	VK_VALIDATION_RESULT(vkCreateSwapchainKHR(s_gpu_device, &create_info, VK_ALLOC_CALLBACK, &s_swap_chain));
	uint32_t swapchain_image_count = 0;
	VK_VALIDATION_RESULT(vkGetSwapchainImagesKHR(s_gpu_device, s_swap_chain, &swapchain_image_count, NULL));
	VK_VALIDATION_RESULT(vkGetSwapchainImagesKHR(s_gpu_device, s_swap_chain, &swapchain_image_count, images));
	if (image_count != swapchain_image_count) {
		VK_LOG("Number of images returned in vkGetSwapchainImagesKHR does not match requested number\n");
		return 0;
	}

	return 1;
}
