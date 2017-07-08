#include "vulkan_engine.h"
#include "vulkan\vulkan_wrapper.h"
#include "Vulkan\vulkan_utils.h"
#include <array>
#include <glm\gtx\euler_angles.hpp>
#include <glm\gtc\matrix_transform.hpp>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

VKAPI_ATTR VkBool32 VKAPI_CALL debugFunction(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObj, size_t location, int32_t msgCode, const char * pLayerPrefix, const char * pMsg, void * pUserData)
{
	if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		LOGI("[VK_DEBUG_REPORT] ERROR: [ %s ] Code %i : %s", pLayerPrefix, msgCode, pMsg);
	}
	else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		LOGI("[VK_DEBUG_REPORT] WARNING: [ %s ] Code %i : %s", pLayerPrefix, msgCode, pMsg);
	}
	else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
	{
		LOGI("[VK_DEBUG_REPORT] INFORMATION: [ %s ] Code %i : %s", pLayerPrefix, msgCode, pMsg);
	}
	else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		LOGI("[VK_DEBUG_REPORT] PERFORMANCE: [ %s ] Code %i : %s", pLayerPrefix, msgCode, pMsg);
	}
	else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		LOGI("[VK_DEBUG_REPORT] VALIDATION: [%s] Code %i : %s", pLayerPrefix, msgCode, pMsg);
	}
	else
		return VK_FALSE;

	return VK_SUCCESS;
}

int init_gpu_instance()
{
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = "Stardust";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Starduster1";
	app_info.engineVersion = VK_MAKE_VERSION(1, 1, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	std::array<const char*, 3> wsi_extensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
	};

	std::array<const char*, 7> wsi_layerNames = {
		"VK_LAYER_GOOGLE_threading"
		,"VK_LAYER_GOOGLE_unique_objects"
		,"VK_LAYER_LUNARG_parameter_validation"
		,"VK_LAYER_LUNARG_object_tracker"
		,"VK_LAYER_LUNARG_image"
		,"VK_LAYER_LUNARG_core_validation"
		,"VK_LAYER_LUNARG_swapchain"
		//,"VK_LAYER_LUNARG_api_dump"
		//,"VK_LAYER_LUNARG_device_limits"
	};

	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pNext = NULL;
	instance_info.flags = 0;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledLayerCount = 7;
	instance_info.ppEnabledLayerNames = wsi_layerNames.data();
	instance_info.enabledExtensionCount = wsi_extensions.size();
	instance_info.ppEnabledExtensionNames = wsi_extensions.data();

	VK_VALIDATION_RESULT(vkCreateInstance(&instance_info, VK_ALLOC_CALLBACK, &s_instance));

	vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(s_instance, "vkCreateDebugReportCallbackEXT");
	vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(s_instance, "vkDestroyDebugReportCallbackEXT");
	vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(s_instance, "vkDebugReportMessageEXT");

	s_dbgReportCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	s_dbgReportCallbackInfo.pNext = nullptr;
	s_dbgReportCallbackInfo.pUserData = nullptr;
	s_dbgReportCallbackInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
		VK_DEBUG_REPORT_DEBUG_BIT_EXT;
	s_dbgReportCallbackInfo.pfnCallback = debugFunction;

	VK_VALIDATION_RESULT(vkCreateDebugReportCallbackEXT(s_instance, &s_dbgReportCallbackInfo, nullptr, &s_debugReportCallback));

	uint32_t count = 1;
	VK_VALIDATION_RESULT(vkEnumeratePhysicalDevices(s_instance, &count, NULL));
	if (count <= 0)
		return 0;

	uint32_t gpuIndex = 1;
	VK_VALIDATION_RESULT(vkEnumeratePhysicalDevices(s_instance, &gpuIndex, &s_gpu));

	std::array<const char*, 1> extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

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

	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queue_info;
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.pNext = NULL;
	queue_info.flags = 0;
	queue_info.queueFamilyIndex = s_queue_family_index;
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

	vkGetPhysicalDeviceProperties(s_gpu, &s_gpu_properties);

	return 1;
}

int engine_init(void)
{
	s_cpu_core_count = sysconf(_SC_NPROCESSORS_ONLN);
	s_time = 4;
	//s_font_letter_count = 0;

	s_frame = 0;

	s_camera_direction = glm::vec3(24.0f, 24.0f, 10.0f);
	s_camera_pitch = -VM_PIDIV4;
	s_camera_yaw = 1.5f * VM_PIDIV4;

	VkCommandPoolCreateInfo command_pool_info;
	command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.pNext = NULL;
	command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_info.queueFamilyIndex = s_queue_family_index;
	VK_VALIDATION_RESULT(vkCreateCommandPool(s_gpu_device, &command_pool_info, VK_ALLOC_CALLBACK, &s_command_pool));

	VkCommandBufferAllocateInfo cmdbuf_info = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL, s_command_pool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY, k_Resource_Buffering
	};
	VK_VALIDATION_RESULT(vkAllocateCommandBuffers(s_gpu_device, &cmdbuf_info, s_cmdbuf_display));
	VK_VALIDATION_RESULT(vkAllocateCommandBuffers(s_gpu_device, &cmdbuf_info, s_cmdbuf_clear));

	/* render targets object pool */ {
		VkMemoryAllocateInfo alloc_info = {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, VK_ALIGN_PAGE(s_win_width * s_win_height * 32),
			get_mem_type_index(s_gpu,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		};
		if (!VKU_Create_Buffer_Memory_Pool(s_gpu_device, s_gpu_queue, &alloc_info, &s_buffer_mempool_target))
			return 0;
		if (!VKU_Create_Image_Memory_Pool(s_gpu_device, s_gpu_queue, &alloc_info, &s_image_mempool_target))
			return 0;
	}
	/* state objects pool */ {
		VkMemoryAllocateInfo alloc_info = {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, 1024 * 1024 * 8,
			get_mem_type_index(s_gpu, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		};
		if (!VKU_Create_Buffer_Memory_Pool(s_gpu_device, s_gpu_queue, &alloc_info, &s_buffer_mempool_state))
			return 0;
		if (!VKU_Create_Image_Memory_Pool(s_gpu_device, s_gpu_queue, &alloc_info, &s_image_mempool_state))
			return 0;
	}
	/* texture pool */ {
		VkMemoryAllocateInfo alloc_info = {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, 1024 * 1024 * 32,
			get_mem_type_index(s_gpu, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		};
		if (!VKU_Create_Buffer_Memory_Pool(s_gpu_device, s_gpu_queue, &alloc_info, &s_buffer_mempool_texture))
			return 0;
		if (!VKU_Create_Image_Memory_Pool(s_gpu_device, s_gpu_queue, &alloc_info, &s_image_mempool_texture))
			return 0;
	}

	VkSamplerCreateInfo sampler_info0 = {
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, NULL, 0,
		VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.0f, VK_FALSE, 1.0f, VK_TRUE, VK_COMPARE_OP_ALWAYS, 0.0f, 0.0f,
		VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FALSE
	};
	VK_VALIDATION_RESULT(vkCreateSampler(s_gpu_device, &sampler_info0, VK_ALLOC_CALLBACK, &s_sampler));

	VkSamplerCreateInfo sampler_info1 = {
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, NULL, 0,
		VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
		0.0f, VK_FALSE, 1.0f, VK_TRUE, VK_COMPARE_OP_ALWAYS, 0.0f, 0.0f,
		VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FALSE
	};
	VK_VALIDATION_RESULT(vkCreateSampler(s_gpu_device, &sampler_info1, VK_ALLOC_CALLBACK, &s_sampler_repeat));

	VkSamplerCreateInfo sampler_info2 = {
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, NULL, 0,
		VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.0f, VK_FALSE, 1, VK_TRUE, VK_COMPARE_OP_ALWAYS, 0.0f, 0.0f,
		VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FALSE
	};
	VK_VALIDATION_RESULT(vkCreateSampler(s_gpu_device, &sampler_info2, VK_ALLOC_CALLBACK, &s_sampler_nearest));

	if (!create_depth_stencil())
		return 0;
	if (!create_common_dset())
		return 0;
	if (!create_particles())
		return 0;
	if (!create_float_renderpass())
		return 0;
	if (!init_dynamic_states())
		return 0;
	if (!create_particle_pipeline())
		return 0;
	if (!create_display_renderpass())
		return 0;
	if (!create_display_pipeline())
		return 0;
	//if (!Create_Graph_Pipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, &s_graph_tri_strip_pipe)) LOG_AND_RETURN0();
	//if (!Create_Graph_Pipeline(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, &s_graph_line_strip_pipe)) LOG_AND_RETURN0();
	//if (!Create_Graph_Pipeline(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, &s_graph_line_list_pipe)) LOG_AND_RETURN0();
	if (!create_window_framebuffer())
		return 0;
	if (!create_constant_memory())
		return 0;
	if (!create_float_image_and_framebuffer())
		return 0;
	//if (!Create_Font_Resources()) LOG_AND_RETURN0();
	//if (!Create_Font_Pipeline()) LOG_AND_RETURN0();
	if (!create_skybox_geometry())
		return 0;
	if (!create_skybox_pipeline())
		return 0;
	if (!create_skybox_generate_pipeline())
		return 0;
	if (!create_skybox_image())
		return 0;
	//if (!Create_Palette_Images()) LOG_AND_RETURN0();
	if (!render_to_skybox_image())
		return 0;
	//if (!Render_To_Palette_Images()) LOG_AND_RETURN0();
	//if (!Create_Common_Graph_Resources()) LOG_AND_RETURN0();

	// CPU Graph

	// create the semaphore
	for (int i = 0; i < s_cpu_core_count; ++i) {
		sem_init(&(s_cmdgen_sem[i]), 1, 0);
	}
	// create the thread
	for (int i = 0; i < s_cpu_core_count; ++i) {
		s_thread[i].tid = i;
		if (!init_particle_thread(&s_thread[i]))
			return 0;
	}

	return 1;
}

int engine_shutdown(void)
{
	return 0;
}

int engine_update(void)
{
	if (s_fence[s_res_idx]) {
		VK_VALIDATION_RESULT(vkWaitForFences(s_gpu_device, 1, &s_fence[s_res_idx], VK_TRUE, UINT64_MAX));
	}

	update_camera();
	if (!update_constant_memory())
		return 0;
	update_common_dset();

	//Todo: How to get the cpu status in Android?
	/*for (int i = 0; i < s_glob_state->cpu_core_count; ++i) {
		if (!Graph_Update_Buffer(&s_graph[i], &s_glob_state->graph_data[i])) LOG_AND_RETURN0();
	}
	if (!Update_Common_Graph_Resources()) LOG_AND_RETURN0();
	if (!Generate_Text()) LOG_AND_RETURN0();*/

#ifdef MT_UPDATE
	// Todo: How to use the system sychronous mechanism?
	/*for (int i = 1; i < s_cpu_core_count; ++i) {
		sem_post(&s_cmdgen_sem[i]);
	}*/
#endif

	VkCommandBufferBeginInfo begin_info = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL
	};
	VK_VALIDATION_RESULT(vkBeginCommandBuffer(s_cmdbuf_clear[s_res_idx], &begin_info));
	cmd_clear(s_cmdbuf_clear[s_res_idx]);
	cmd_render_skybox(s_cmdbuf_clear[s_res_idx]);
	VK_VALIDATION_RESULT(vkEndCommandBuffer(s_cmdbuf_clear[s_res_idx]));

	VK_VALIDATION_RESULT(vkBeginCommandBuffer(s_cmdbuf_display[s_res_idx], &begin_info));
	cmd_display_fractal(s_cmdbuf_display[s_res_idx]);
	/*for (int i = 0; i < s_glob_state->cpu_core_count; ++i) {
		Graph_Draw(&s_graph[i], s_cmdbuf_display[s_res_idx]);
	}
	Cmd_Draw_Text(s_cmdbuf_display[s_res_idx]);*/
	VK_VALIDATION_RESULT(vkEndCommandBuffer(s_cmdbuf_display[s_res_idx]));

#ifdef MT_UPDATE
	/*update_particle_thread(&s_thread[0]);
	for (int i = 1; i < s_cpu_core_count; ++i) {
		sem_wait(&(s_thread[i].sem));
	}*/
#else
	for (int i = 0; i < s_glob_state->cpu_core_count; ++i) {
		Update_Particle_Thread(&s_thread[i]);
	}
#endif

	if (!s_fence[s_res_idx]) {
		VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, 0 };
		VK_VALIDATION_RESULT(vkCreateFence(s_gpu_device, &fence_info, VK_ALLOC_CALLBACK, &s_fence[s_res_idx]));
	}

	VkCommandBuffer cmdbuf[2 + MAX_CPU_CORES];
	int cmdbuf_count = 0;

	cmdbuf[cmdbuf_count++] = s_cmdbuf_clear[s_res_idx];
	//for (int i = 0; i < s_cpu_core_count; ++i) {
	//	cmdbuf[cmdbuf_count++] = s_thread[i].cmdbuf[s_res_idx];
	//}
	//cmdbuf[cmdbuf_count++] = s_cmdbuf_display[s_res_idx];

	VK_VALIDATION_RESULT(vkResetFences(s_gpu_device, 1, &s_fence[s_res_idx]));

	VkPipelineStageFlags graphicFlag = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	VkSubmitInfo submit_info;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &s_swap_chain_image_ready_semaphore;
	submit_info.pWaitDstStageMask = &graphicFlag;
	submit_info.commandBufferCount = cmdbuf_count;
	submit_info.pCommandBuffers = cmdbuf;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = NULL;
	VK_VALIDATION_RESULT(vkQueueSubmit(s_gpu_queue, 1, &submit_info, s_fence[s_res_idx]));

	return STARDUST_CONTINUE;
}

void update_camera(void)
{
	const float move_scale = 10.0f * s_time_delta;

	glm::vec4 forward = glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::vec3 rot = glm::vec3(s_camera_pitch, s_camera_yaw, 0.0f);

	glm::mat4 trans = glm::eulerAngleZY(s_camera_pitch, s_camera_yaw);

	glm::vec4 transForward = trans * forward;
	s_camera_direction = glm::vec3(transForward[0], transForward[1], transForward[2]);
	s_camera_right = glm::cross(s_camera_direction, up);
	glm::normalize(s_camera_right);
}

int init_device(android_app* pApp, void* pWnd, int width, int height, float init_time, unsigned int init_seed, bool is_transf_anim, VkBool32 windowed)
{
	init_gpu_instance();

	vkGetDeviceQueue(s_gpu_device, s_queue_family_index, 0, &s_gpu_queue);

	if (!init_framebuffer(pWnd, width, height, windowed, k_Window_Buffering, s_win_images))
		return 0;

	s_win_width = width;
	s_win_height = height;
	s_app = pApp;

	s_time = init_time;
	s_seed = init_seed;
	s_transf_animate = is_transf_anim;
	return 0;
}

void deinit_device(void)
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
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
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

int create_depth_stencil(void)
{
	VkImageCreateInfo ds_info = {
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, NULL, 0, VK_IMAGE_TYPE_2D,
		VK_FORMAT_D24_UNORM_S8_UINT,{ s_win_width, s_win_height, 1 }, 1, 1, VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_SHARING_MODE_EXCLUSIVE, 0, NULL, VK_IMAGE_LAYOUT_UNDEFINED
	};
	VK_VALIDATION_RESULT(vkCreateImage(s_gpu_device, &ds_info, VK_ALLOC_CALLBACK, &s_depth_stencil_image));
	if (!VKU_Alloc_Image_Object(s_image_mempool_target, s_depth_stencil_image, NULL, get_mem_type_index(s_gpu, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)))
		return 0;

	VkComponentMapping channels = {
		VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A
	};
	VkImageSubresourceRange subres_range = {
		VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1
	};
	VkImageViewCreateInfo ds_view_info = {
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, NULL, 0,
		s_depth_stencil_image, VK_IMAGE_VIEW_TYPE_2D,
		VK_FORMAT_D24_UNORM_S8_UINT, channels, subres_range
	};
	VK_VALIDATION_RESULT(vkCreateImageView(s_gpu_device, &ds_view_info, VK_ALLOC_CALLBACK, &s_depth_stencil_view));

	return 1;
}

int create_common_dset(void)
{
	VkDescriptorSetLayoutBinding desc6_info = {
		6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, NULL
	};
	VkDescriptorSetLayoutBinding desc5_info = {
		5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_ALL, NULL
	};
	VkDescriptorSetLayoutBinding desc4_info = {
		4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_ALL, NULL
	};
	VkDescriptorSetLayoutBinding desc3_info = {
		3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_ALL, NULL
	};
	VkDescriptorSetLayoutBinding desc2_info = {
		2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_ALL, NULL
	};
	VkDescriptorSetLayoutBinding desc1_info = {
		1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_ALL, NULL
	};
	VkDescriptorSetLayoutBinding desc0_info = {
		0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, NULL
	};

	std::array<VkDescriptorSetLayoutBinding, 4> infos = {
		desc0_info,
		desc1_info,
		desc2_info,
		//desc3_info,
		//desc4_info,
		//desc5_info,
		desc6_info
	};

	VkDescriptorSetLayoutCreateInfo set_info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, NULL, 0,
		infos.size(), infos.data()
	};

	VK_VALIDATION_RESULT(vkCreateDescriptorSetLayout(s_gpu_device, &set_info, VK_ALLOC_CALLBACK, &s_common_dset_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_info;
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.pNext = NULL;
	pipeline_layout_info.flags = 0;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &s_common_dset_layout;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = NULL;

	VK_VALIDATION_RESULT(vkCreatePipelineLayout(s_gpu_device, &pipeline_layout_info, VK_ALLOC_CALLBACK, &s_common_pipeline_layout));
	
	std::array<VkDescriptorPoolSize, 2> desc_type_count;
	desc_type_count[0] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 * k_Resource_Buffering };
	desc_type_count[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 * k_Resource_Buffering };

	VkDescriptorPoolCreateInfo pool_info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, NULL, 0,
		k_Resource_Buffering, desc_type_count.size(), desc_type_count.data()
	};

	VK_VALIDATION_RESULT(vkCreateDescriptorPool(s_gpu_device, &pool_info, VK_ALLOC_CALLBACK, &s_common_dpool));

	const VkDescriptorSetAllocateInfo desc_set_alloc_info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, NULL,
		s_common_dpool, 1, &s_common_dset_layout
	};

	for (int i = 0; i < k_Resource_Buffering; i++)
		VK_VALIDATION_RESULT(vkAllocateDescriptorSets(s_gpu_device, &desc_set_alloc_info, &s_common_dset[i]));

	return 1;
}

int update_constant_memory(void)
{
	typedef struct CONSTANT
	{
		glm::mat4 viewproj;
		unsigned int data[48];
		float palette_factor;
	} CONSTANT;
	CONSTANT *ptr;

	VK_VALIDATION_RESULT(vkMapMemory(s_gpu_device, s_constant_mem[s_res_idx], 0, sizeof(CONSTANT), 0, (void **)&ptr));

	glm::mat4 rot, view, proj;
	glm::vec3 up, atv;
	glm::vec3 eye, at;
	eye = s_camera_position;
	atv = s_camera_position + s_camera_direction;
	at = atv;

	up = glm::vec3(0.0f, 1.0f, 0.0f);

	rot = glm::eulerAngleY((float)s_time * 0.04f);
	view = glm::lookAt(eye, at, up);
	proj = glm::perspective(VM_PI / 3, (float) s_win_width / s_win_height, 0.1f, 100.0f);

	ptr->viewproj = proj * view * rot;

	float tt = s_transf_time * s_transf_time * (3.0f - 2.0f * s_transf_time);
	if (s_transf_animate) {
		s_palette_factor += s_time_delta * 0.25f;
		if (s_palette_factor > 1.0f) {
			for (int i = 0; i < 9; ++i) {
				RND_GEN(s_seed);
			}
			s_palette_factor = 0.0f;
			s_palette_image_idx = (s_palette_image_idx + 1) % 5;
		}
	}
	ptr->palette_factor = s_palette_factor * s_palette_factor * (3.0f - 2.0f * s_palette_factor);

	if (s_transf_animate) {
		s_transf_time += s_time_delta * 0.2f;

		if (s_transf_time > 1.0f) {
			s_transf_time = 0.0f;
		}
	}

	ptr->data[0] = s_seed;

	vkUnmapMemory(s_gpu_device, s_constant_mem[s_res_idx]);

	return 1;
}

void update_common_dset(void)
{
	/*VkDescriptorImageInfo font_image_sampler_info = {
		s_sampler_nearest, s_font_image_view, VK_IMAGE_LAYOUT_GENERAL
	};*/
	/*VkDescriptorImageInfo palette1_image_sampler_info = {
		s_sampler_repeat, s_palette_image_view[(s_glob_state->palette_image_idx + 1) % 5], VK_IMAGE_LAYOUT_GENERAL
	};*/
	/*VkDescriptorImageInfo palette0_image_sampler_info = {
		s_sampler_repeat, s_palette_image_view[s_glob_state->palette_image_idx], VK_IMAGE_LAYOUT_GENERAL
	};*/
	VkDescriptorImageInfo skybox_image_sampler_info = {
		s_sampler, s_skybox_image_view, VK_IMAGE_LAYOUT_GENERAL
	};
	VkDescriptorImageInfo float_image_sampler_info = {
		s_sampler, s_float_image_view, VK_IMAGE_LAYOUT_GENERAL
	};

	VkDescriptorBufferInfo constant_buf_info = {
		s_constant_buf[s_res_idx], 0, 4 * sizeof(glm::mat4) + 48 * sizeof(unsigned int) + sizeof(float)
	};

	VkDescriptorBufferInfo skybox_buf_info = {
		s_skybox_buf, 0, 14 * sizeof(glm::vec4)
	};

	VkWriteDescriptorSet update_skybox_buffers = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, s_common_dset[s_res_idx],
		6, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_NULL_HANDLE,
		&skybox_buf_info, VK_NULL_HANDLE
	};
	/*VkWriteDescriptorSet update_sampler_font_image = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, s_common_dset[s_res_idx],
		5, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &font_image_sampler_info,
		VK_NULL_HANDLE, VK_NULL_HANDLE
	};*/
	/*VkWriteDescriptorSet update_sampler_palette1_image = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, s_common_dset[s_res_idx],
		4, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &palette1_image_sampler_info,
		VK_NULL_HANDLE, VK_NULL_HANDLE
	};*/
	/*VkWriteDescriptorSet update_sampler_palette0_image = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, s_common_dset[s_res_idx],
		3, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &palette0_image_sampler_info,
		VK_NULL_HANDLE, VK_NULL_HANDLE
	};*/
	VkWriteDescriptorSet update_sampler_skybox_image = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, s_common_dset[s_res_idx],
		2, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &skybox_image_sampler_info,
		VK_NULL_HANDLE, VK_NULL_HANDLE
	};
	VkWriteDescriptorSet update_sampler_float_image = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, s_common_dset[s_res_idx],
		1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &float_image_sampler_info,
		VK_NULL_HANDLE, VK_NULL_HANDLE
	};
	VkWriteDescriptorSet update_buffers = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, s_common_dset[s_res_idx],
		0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_NULL_HANDLE,
		&constant_buf_info, VK_NULL_HANDLE
	};

	std::array<VkWriteDescriptorSet, 4> write_descriptors = {
		update_buffers,
		update_sampler_float_image,
		update_sampler_skybox_image,
		//update_sampler_palette0_image,
		//update_sampler_palette1_image,
		//update_sampler_font_image,
		update_skybox_buffers
	};

	vkUpdateDescriptorSets(s_gpu_device, write_descriptors.size(), write_descriptors.data(), 0, NULL);
}

int create_float_renderpass(void)
{
	VkAttachmentLoadOp color_ld_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	VkAttachmentStoreOp color_st_op = VK_ATTACHMENT_STORE_OP_STORE;
	VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;

	VkAttachmentDescription color_attachment_desc;
	color_attachment_desc.flags = 0;
	color_attachment_desc.format = colorFormat;
	color_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment_desc.loadOp = color_ld_op;
	color_attachment_desc.storeOp = color_st_op;
	color_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment_desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription ds_attachment_desc;
	ds_attachment_desc.flags = 0;
	ds_attachment_desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
	ds_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
	ds_attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ds_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ds_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ds_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	ds_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ds_attachment_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_ref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference ds_attachment_ref = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass_desc;
	subpass_desc.flags = 0;
	subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_desc.inputAttachmentCount = 0;
	subpass_desc.pInputAttachments = NULL;
	subpass_desc.colorAttachmentCount = 1;
	subpass_desc.pColorAttachments = &color_attachment_ref;
	subpass_desc.pResolveAttachments = NULL;
	subpass_desc.pDepthStencilAttachment = &ds_attachment_ref;
	subpass_desc.preserveAttachmentCount = 0;
	subpass_desc.pPreserveAttachments = NULL;

	VkAttachmentDescription attachment_descs[2];
	attachment_descs[0] = color_attachment_desc;
	attachment_descs[1] = ds_attachment_desc;

	VkRenderPassCreateInfo render_info;
	render_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_info.pNext = NULL;
	render_info.flags = 0;
	render_info.attachmentCount = 2;
	render_info.pAttachments = attachment_descs;
	render_info.subpassCount = 1;
	render_info.pSubpasses = &subpass_desc;
	render_info.dependencyCount = 0;
	render_info.pDependencies = NULL;

	VK_VALIDATION_RESULT(vkCreateRenderPass(s_gpu_device, &render_info, VK_ALLOC_CALLBACK, &s_float_renderpass));

	return 1;
}

//Todo: define the split viewport of the whole window
int init_dynamic_states(void)
{
	VkViewport vp = { 0.0f, 0.0f, (float)s_win_width, (float)s_win_height, 0.0f, 1.0f };
	VkRect2D scissor = { { 0, 0 },{ s_win_width, s_win_height } };

	s_vp_state.viewportCount = 1;
	s_vp_state.viewport = vp;
	s_vp_state.scissorCount = 1;
	s_vp_state.scissors = scissor;

	{
		VkViewport vp = { 0.0f, 0.0f, 1024.0f, 1024.0f, 0.0f, 1.0f };
		VkRect2D scissor = { { 0, 0 },{ 1024, 1024 } };
		s_vp_state_copy_skybox.viewportCount = 1;
		s_vp_state_copy_skybox.viewport = vp;
		s_vp_state_copy_skybox.scissorCount = 1;
		s_vp_state_copy_skybox.scissors = scissor;
	}
	{
		VkViewport vp = { 0.0f, 0.0f, 256.0f, 1.0f, 0.0f, 1.0f };
		VkRect2D scissor = { { 0, 0 },{ 256, 1 } };
		s_vp_state_copy_palette.viewportCount = 1;
		s_vp_state_copy_palette.viewport = vp;
		s_vp_state_copy_palette.scissorCount = 1;
		s_vp_state_copy_palette.scissors = scissor;
	}
	{
		VkViewport vp = { 16.0f, 180.0f, 40.0f, 20.0f, 0.0f, 1.0f };
		VkRect2D scissor = { { 0, 0 },{ s_win_width, s_win_height } };
		s_vp_state_legend_cpu.viewportCount = 1;
		s_vp_state_legend_cpu.viewport = vp;
		s_vp_state_legend_cpu.scissorCount = 1;
		s_vp_state_legend_cpu.scissors = scissor;
	}
	{
		VkViewport vp = { 16.0f, 210.0f, 40.0f, 20.0f, 0.0f, 1.0f };
		VkRect2D scissor = { { 0, 0 },{ s_win_width, s_win_height } };
		s_vp_state_legend_gpu.viewportCount = 1;
		s_vp_state_legend_gpu.viewport = vp;
		s_vp_state_legend_gpu.scissorCount = 1;
		s_vp_state_legend_gpu.scissors = scissor;
	}

	s_rs_state.depthBias = 0.0f;
	s_rs_state.depthBiasClamp = 0.0f;
	s_rs_state.slopeScaledDepthBias = 0.0f;
	s_rs_state.lineWidth = 1.0f;

	s_cb_state.blendConst[0] = 0.0f;
	s_cb_state.blendConst[1] = 0.0f;
	s_cb_state.blendConst[2] = 0.0f;
	s_cb_state.blendConst[3] = 0.0f;

	s_ds_state.minDepthBounds = 0.0f;
	s_ds_state.maxDepthBounds = 1.0f;

	return 1;
}

int create_display_renderpass(void)
{
	VkAttachmentLoadOp color_ld_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	VkAttachmentStoreOp color_st_op = VK_ATTACHMENT_STORE_OP_STORE;
	VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;

	VkAttachmentDescription color_attachment_desc;
	color_attachment_desc.flags = 0;
	color_attachment_desc.format = colorFormat;
	color_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment_desc.loadOp = color_ld_op;
	color_attachment_desc.storeOp = color_st_op;
	color_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription ds_attachment_desc;
	ds_attachment_desc.flags = 0;
	ds_attachment_desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
	ds_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
	ds_attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ds_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ds_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ds_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	ds_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ds_attachment_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_ref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference ds_attachment_ref = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass_desc;
	subpass_desc.flags = 0;
	subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_desc.inputAttachmentCount = 0;
	subpass_desc.pInputAttachments = NULL;
	subpass_desc.colorAttachmentCount = 1;
	subpass_desc.pColorAttachments = &color_attachment_ref;
	subpass_desc.pResolveAttachments = NULL;
	subpass_desc.pDepthStencilAttachment = &ds_attachment_ref;
	subpass_desc.preserveAttachmentCount = 0;
	subpass_desc.pPreserveAttachments = NULL;

	VkAttachmentDescription attachment_descs[2];
	attachment_descs[0] = color_attachment_desc;
	attachment_descs[1] = ds_attachment_desc;

	VkRenderPassCreateInfo render_info;
	render_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_info.pNext = NULL;
	render_info.flags = 0;
	render_info.attachmentCount = 2;
	render_info.pAttachments = attachment_descs;
	render_info.subpassCount = 1;
	render_info.pSubpasses = &subpass_desc;
	render_info.dependencyCount = 0;
	render_info.pDependencies = NULL;

	VK_VALIDATION_RESULT(vkCreateRenderPass(s_gpu_device, &render_info, VK_ALLOC_CALLBACK, &s_win_renderpass));

	return 1;
}

int create_display_pipeline(void)
{
	VkShaderModule vs, fs;
	vs = VK_NULL_HANDLE;
	fs = VK_NULL_HANDLE;

	//VKU_Compile_Shader(s_app->activity->assetManager, s_gpu_device,"Shader_GLSL/VS_Quad_UL.vert", shaderc_glsl_vertex_shader, &vs);
	//VKU_Compile_Shader(s_app->activity->assetManager, s_gpu_device,"Shader_GLSL/FS_Display.frag", shaderc_glsl_fragment_shader, &fs);
	VKU_Load_Shader(s_app->activity->assetManager, s_gpu_device, "Shader_GLSL/VS_Quad_UL.bil", &vs);
	VKU_Load_Shader(s_app->activity->assetManager, s_gpu_device, "Shader_GLSL/FS_Display.bil", &fs);

	if (vs == VK_NULL_HANDLE || fs == VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(s_gpu_device, vs, VK_ALLOC_CALLBACK);
		vkDestroyShaderModule(s_gpu_device, fs, VK_ALLOC_CALLBACK);
		return 0;
	}

	VkPipelineShaderStageCreateInfo vs_info = {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vs, "main", NULL
	};
	VkPipelineShaderStageCreateInfo fs_info = {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fs, "main", NULL
	};
	VkPipelineInputAssemblyStateCreateInfo ia_info = {
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, NULL, 0,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_FALSE
	};
	VkPipelineTessellationStateCreateInfo tess_info = {
		VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, NULL, 0, 0
	};
	VkPipelineViewportStateCreateInfo vp_info = {
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, NULL, 0, s_vp_state.viewportCount,
		&s_vp_state.viewport, s_vp_state.scissorCount, &s_vp_state.scissors
	};
	VkPipelineRasterizationStateCreateInfo rs_info = {
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, NULL, 0, VK_FALSE,
		VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE,
		s_rs_state.depthBias, s_rs_state.depthBiasClamp,
		s_rs_state.slopeScaledDepthBias, s_rs_state.lineWidth
	};
	VkPipelineMultisampleStateCreateInfo ms_info = {
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, NULL, 0, VK_SAMPLE_COUNT_1_BIT, VK_FALSE,
		1.0f, NULL, VK_FALSE, VK_FALSE
	};
	VkPipelineColorBlendAttachmentState attachment = {
		VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, 0x0f
	};
	VkPipelineColorBlendStateCreateInfo cb_info = {
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, NULL, 0, VK_FALSE,
		VK_LOGIC_OP_COPY, 1, &attachment, s_cb_state.blendConst[0]
	};
	VkPipelineDepthStencilStateCreateInfo db_info = {
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, NULL, 0,
		VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE, VK_FALSE,
		{ VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0, 0, 0 },
		{ VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0, 0, 0 },
		s_ds_state.minDepthBounds, s_ds_state.maxDepthBounds
	};

	VkPipelineShaderStageCreateInfo shader_stage_infos[2];
	shader_stage_infos[0] = vs_info;
	shader_stage_infos[1] = fs_info;


	VkVertexInputBindingDescription vf_binding_desc = {
		0, sizeof(uint32_t), VK_VERTEX_INPUT_RATE_VERTEX
	};

	std::array<VkVertexInputAttributeDescription, 1> vf_attribute_desc = {
		{
			0, 0, VK_FORMAT_R32_UINT, 0
		},
	};

	VkPipelineVertexInputStateCreateInfo vf_info = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, NULL, 0,
		1, &vf_binding_desc, vf_attribute_desc.size(), vf_attribute_desc.data()
	};

	VkGraphicsPipelineCreateInfo pi_info = {
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, NULL, 0, 2, shader_stage_infos,
		&vf_info, &ia_info, &tess_info, &vp_info, &rs_info, &ms_info, &db_info, &cb_info,
		NULL, s_common_pipeline_layout, s_win_renderpass, 0, VK_NULL_HANDLE, 0
	};

	VK_VALIDATION_RESULT(vkCreateGraphicsPipelines(s_gpu_device, VK_NULL_HANDLE, 1, &pi_info, VK_ALLOC_CALLBACK, &s_display_pipe));

	vkDestroyShaderModule(s_gpu_device, vs, VK_ALLOC_CALLBACK);
	vkDestroyShaderModule(s_gpu_device, fs, VK_ALLOC_CALLBACK);

	return 1;
}

// Todo: create the vertex buffer for all the particles
int create_particles(void)
{
	VkBufferCreateInfo buffer_info = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL, 0, k_Def_Point_Count * sizeof(uint32_t),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, 0, NULL
	};
	VK_VALIDATION_RESULT(vkCreateBuffer(s_gpu_device, &buffer_info, VK_ALLOC_CALLBACK, &s_particle_seed_buf));

	VkMemoryAllocateInfo alloc_info = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL,
		VK_ALIGN_PAGE(k_Def_Point_Count * sizeof(uint32_t)),
		get_mem_type_index(s_gpu, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	};
	VK_VALIDATION_RESULT(vkAllocateMemory(s_gpu_device, &alloc_info, VK_ALLOC_CALLBACK, &s_particle_seed_mem));
	VK_VALIDATION_RESULT(vkBindBufferMemory(s_gpu_device, s_particle_seed_buf, s_particle_seed_mem, 0));

	unsigned int seed = 23232323;
	uint32_t *pseed = (uint32_t*) malloc(k_Def_Point_Count * sizeof(*pseed));
	if (!pseed)
		return 0;

	for (int i = 0; i < k_Def_Point_Count; ++i) {
		RND_GEN(seed);
		pseed[i] = seed;
	}

	void *ptr;
	VK_VALIDATION_RESULT(vkMapMemory(s_gpu_device, s_particle_seed_mem, 0, k_Def_Point_Count * sizeof(*pseed), 0, &ptr));
	memcpy(ptr, pseed, k_Def_Point_Count * sizeof(*pseed));
	vkUnmapMemory(s_gpu_device, s_particle_seed_mem);
	free(pseed);

	return 1;
}

// Todo: create the particle rendering pipeline
int create_particle_pipeline(void)
{
	return 1;
	VkShaderModule vs, fs;
	vs = VK_NULL_HANDLE;
	fs = VK_NULL_HANDLE;

	VKU_Load_Shader(s_app->activity->assetManager,s_gpu_device, "Shader_GLSL/VS_Particle_Draw.bil", &vs);
	VKU_Load_Shader(s_app->activity->assetManager,s_gpu_device, "Shader_GLSL/FS_Particle_Draw.bil", &fs);

	if (vs == VK_NULL_HANDLE || fs == VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(s_gpu_device, vs, VK_ALLOC_CALLBACK);
		vkDestroyShaderModule(s_gpu_device, fs, VK_ALLOC_CALLBACK);
		return 0;
	}

	VkPipelineShaderStageCreateInfo vs_info = {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vs, "main", NULL
	};
	VkPipelineShaderStageCreateInfo fs_info = {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fs, "main", NULL
	};
	VkPipelineInputAssemblyStateCreateInfo ia_info = {
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, NULL, 0,
		VK_PRIMITIVE_TOPOLOGY_POINT_LIST, VK_FALSE
	};
	VkPipelineTessellationStateCreateInfo tess_info = {
		VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, NULL, 0, 0
	};
	VkPipelineViewportStateCreateInfo vp_info = {
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, NULL, 0, s_vp_state.viewportCount,
		&s_vp_state.viewport, s_vp_state.scissorCount, &s_vp_state.scissors
	};
	VkPipelineRasterizationStateCreateInfo rs_info = {
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, NULL, 0, VK_FALSE,
		VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE,
		s_rs_state.depthBias, s_rs_state.depthBiasClamp,
		s_rs_state.slopeScaledDepthBias, s_rs_state.lineWidth
	};
	VkPipelineMultisampleStateCreateInfo ms_info = {
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, NULL, 0, VK_SAMPLE_COUNT_1_BIT, VK_FALSE,
		1.0f, NULL, VK_FALSE, VK_FALSE
	};
	VkPipelineColorBlendAttachmentState attachment = {
		VK_TRUE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, 0x0f
	};
	VkPipelineColorBlendStateCreateInfo cb_info = {
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, NULL, 0, VK_FALSE,
		VK_LOGIC_OP_COPY, 1, &attachment, s_cb_state.blendConst[0]
	};
	VkPipelineDepthStencilStateCreateInfo db_info = {
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, NULL, 0,
		VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE, VK_FALSE,
		{ VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0, 0, 0 },
		{ VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0, 0, 0 },
		s_ds_state.minDepthBounds, s_ds_state.maxDepthBounds
	};

	VkVertexInputBindingDescription vf_binding_desc = {
		0, sizeof(uint32_t), VK_VERTEX_INPUT_RATE_VERTEX
	};

	std::array<VkVertexInputAttributeDescription, 1> vf_attribute_desc = {
		{
			0, 0, VK_FORMAT_R32_UINT, 0
		},
	};

	VkPipelineVertexInputStateCreateInfo vf_info = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, NULL, 0,
		1, &vf_binding_desc, vf_attribute_desc.size(), vf_attribute_desc.data()
	};

	VkPipelineShaderStageCreateInfo shader_stage_infos[2];
	shader_stage_infos[0] = vs_info;
	shader_stage_infos[1] = fs_info;

	VkGraphicsPipelineCreateInfo pi_info = {
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, NULL, 0, 2, shader_stage_infos,
		&vf_info, &ia_info, &tess_info, &vp_info, &rs_info, &ms_info, &db_info, &cb_info,
		NULL, s_common_pipeline_layout, s_float_renderpass, 0, VK_NULL_HANDLE, 0
	};

	VkResult r = vkCreateGraphicsPipelines(s_gpu_device, VK_NULL_HANDLE, 1, &pi_info, VK_ALLOC_CALLBACK, &s_particle_pipe);

	vkDestroyShaderModule(s_gpu_device, vs, VK_ALLOC_CALLBACK);
	vkDestroyShaderModule(s_gpu_device, fs, VK_ALLOC_CALLBACK);

	if (r != VK_SUCCESS)
		return 0;

	return 1;
}

int create_graph_pipeline(VkPrimitiveTopology topology, VkPipeline * pipe)
{
	return 0;
}

int create_window_framebuffer(void)
{
	for (int i = 0; i < k_Window_Buffering; ++i) {
		VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;

		VkComponentMapping channels = {
			VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A
		};
		VkImageSubresourceRange subres_range = {
			VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
		};
		VkImageViewCreateInfo view_info = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, NULL, 0, s_win_images[i],
			VK_IMAGE_VIEW_TYPE_2D, colorFormat, channels, subres_range
		};
		VK_VALIDATION_RESULT(vkCreateImageView(s_gpu_device, &view_info, VK_ALLOC_CALLBACK, &s_win_image_view[i]));

		VkImageView view_infos[2];
		view_infos[0] = s_win_image_view[i];
		view_infos[1] = s_depth_stencil_view;

		VkFramebufferCreateInfo fb_info = {
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, NULL, 0, s_win_renderpass,
			2, view_infos, s_win_width, s_win_height, 1
		};
		VK_VALIDATION_RESULT(vkCreateFramebuffer(s_gpu_device, &fb_info, VK_ALLOC_CALLBACK, &s_win_framebuffer[i]));
	}

	return 1;
}

int create_constant_memory(void)
{
	VkBufferCreateInfo buffer_info = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL, 0, 4 * sizeof(glm::mat4) + 48 * sizeof(unsigned int) + sizeof(float),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, 0, NULL
	};
	VkMemoryAllocateInfo alloc_info = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, VK_PAGE_SIZE,
		get_mem_type_index(s_gpu, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	};

	for (int i = 0; i < k_Resource_Buffering; ++i) {
		VK_VALIDATION_RESULT(vkCreateBuffer(s_gpu_device, &buffer_info, VK_ALLOC_CALLBACK, &s_constant_buf[i]));
		VK_VALIDATION_RESULT(vkAllocateMemory(s_gpu_device, &alloc_info, VK_ALLOC_CALLBACK, &s_constant_mem[i]));
		VK_VALIDATION_RESULT(vkBindBufferMemory(s_gpu_device, s_constant_buf[i], s_constant_mem[i], 0));
	}

	return 1;
}

int create_float_image_and_framebuffer(void)
{
	VkImageCreateInfo image_info = {
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, NULL, 0, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
		{ s_win_width, s_win_height, 1 }, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_SHARING_MODE_EXCLUSIVE, 0, NULL, VK_IMAGE_LAYOUT_UNDEFINED
	};
	VK_VALIDATION_RESULT(vkCreateImage(s_gpu_device, &image_info, VK_ALLOC_CALLBACK, &s_float_image));
	if (!VKU_Alloc_Image_Object(s_image_mempool_target, s_float_image, NULL, get_mem_type_index(s_gpu, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)))
		return 0;

	VkImageViewCreateInfo image_view = {
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, NULL, 0, s_float_image, VK_IMAGE_VIEW_TYPE_2D,
		VK_FORMAT_R8G8B8A8_UNORM,
		{
			VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		},
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};
	VK_VALIDATION_RESULT(vkCreateImageView(s_gpu_device, &image_view, VK_ALLOC_CALLBACK, &s_float_image_view));

	VkImageView view_infos[2];
	view_infos[0] = s_float_image_view;
	view_infos[1] = s_depth_stencil_view;

	VkFramebufferCreateInfo fb_info = {
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, NULL, 0, s_float_renderpass,
		2, view_infos, s_win_width, s_win_height, 1
	};
	VK_VALIDATION_RESULT(vkCreateFramebuffer(s_gpu_device, &fb_info, VK_ALLOC_CALLBACK, &s_float_framebuffer));

	return 1;
}

int create_skybox_geometry(void)
{
	VkDeviceSize size = 14 * sizeof(glm::vec4);
	VkBufferCreateInfo buffer_info = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL, 0, size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE, 0, NULL
	};
	VK_VALIDATION_RESULT(vkCreateBuffer(s_gpu_device, &buffer_info, VK_ALLOC_CALLBACK, &s_skybox_buf));

	VkMemoryAllocateInfo alloc_info = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, VK_ALIGN_PAGE(size),
		get_mem_type_index(s_gpu, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	};
	VK_VALIDATION_RESULT(vkAllocateMemory(s_gpu_device, &alloc_info, VK_ALLOC_CALLBACK, &s_skybox_mem));
	VK_VALIDATION_RESULT(vkBindBufferMemory(s_gpu_device, s_skybox_buf, s_skybox_mem, 0));

	/*std::array<glm::vec4, 14> bufferData;
	for (size_t i = 0; i < 14; i++)
	{
		int b = 1 << i;
		float x = (2.0 * float((0x287a & b) != 0) - 1.0) * 1000.0;
		float y = (2.0 * float((0x02af & b) != 0) - 1.0) * 1000.0;
		float z = (2.0 * float((0x31e3 & b) != 0) - 1.0) * 1000.0;

		bufferData[i] = glm::vec4(x, y, z, 1.0);
	}

	void* data;
	vkMapMemory(s_gpu_device, s_skybox_mem, 0, size, 0, &data);
	memcpy(data, bufferData.data(), size);
	vkUnmapMemory(s_gpu_device, s_skybox_mem);*/

	return 1;
}

int create_skybox_pipeline(void)
{
	VkShaderModule vs, fs;
	vs = VK_NULL_HANDLE;
	fs = VK_NULL_HANDLE;

	VKU_Load_Shader(s_app->activity->assetManager, s_gpu_device, "Shader_GLSL/VS_Skybox.bil", &vs);
	VKU_Load_Shader(s_app->activity->assetManager, s_gpu_device, "Shader_GLSL/FS_Skybox.bil", &fs);

	if (vs == VK_NULL_HANDLE || fs == VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(s_gpu_device, vs, VK_ALLOC_CALLBACK);
		vkDestroyShaderModule(s_gpu_device, fs, VK_ALLOC_CALLBACK);

		return 0;
	}

	VkPipelineShaderStageCreateInfo vs_info = {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vs, "main", NULL
	};
	VkPipelineShaderStageCreateInfo fs_info = {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fs, "main", NULL
	};
	VkPipelineInputAssemblyStateCreateInfo ia_info = {
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, NULL, 0,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_FALSE
	};
	VkPipelineTessellationStateCreateInfo tess_info = {
		VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, NULL, 0, 0
	};
	VkPipelineViewportStateCreateInfo vp_info = {
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, NULL, 0, s_vp_state.viewportCount,
		&s_vp_state.viewport, s_vp_state.scissorCount, &s_vp_state.scissors
	};
	VkPipelineRasterizationStateCreateInfo rs_info = {
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, NULL, 0, VK_FALSE,
		VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE,
		VK_FALSE, s_rs_state.depthBias, s_rs_state.depthBiasClamp,
		s_rs_state.slopeScaledDepthBias, s_rs_state.lineWidth
	};
	VkPipelineMultisampleStateCreateInfo ms_info = {
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, NULL, 0, VK_SAMPLE_COUNT_1_BIT, VK_FALSE,
		1.0f, NULL, VK_FALSE, VK_FALSE
	};
	VkPipelineColorBlendAttachmentState attachment = {
		VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, 0x0f
	};
	VkPipelineColorBlendStateCreateInfo cb_info = {
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, NULL, 0, VK_FALSE,
		VK_LOGIC_OP_COPY, 1, &attachment, s_cb_state.blendConst[0]
	};
	VkPipelineDepthStencilStateCreateInfo db_info = {
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, NULL, 0,
		VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE, VK_FALSE,
		{ VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0, 0, 0 },
		{ VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0, 0, 0 },
		s_ds_state.minDepthBounds, s_ds_state.maxDepthBounds
	};

	VkVertexInputBindingDescription vf_binding_desc = {
		0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX
	};

	std::array<VkVertexInputAttributeDescription, 1> vf_attribute_desc = {
		{
			0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0
		}
	};

	VkPipelineVertexInputStateCreateInfo vf_info = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, NULL, 0,
		1, &vf_binding_desc, vf_attribute_desc.size(), vf_attribute_desc.data()
	};

	VkPipelineShaderStageCreateInfo shader_stage_infos[2];
	shader_stage_infos[0] = vs_info;
	shader_stage_infos[1] = fs_info;

	VkGraphicsPipelineCreateInfo pi_info = {
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, NULL, 0, 2, shader_stage_infos,
		&vf_info, &ia_info, &tess_info, &vp_info, &rs_info, &ms_info, &db_info, &cb_info,
		NULL, s_common_pipeline_layout, s_float_renderpass, 0, VK_NULL_HANDLE, 0
	};

	VkResult r = vkCreateGraphicsPipelines(s_gpu_device, VK_NULL_HANDLE, 1, &pi_info, VK_ALLOC_CALLBACK, &s_skybox_pipe);

	vkDestroyShaderModule(s_gpu_device, vs, VK_ALLOC_CALLBACK);
	vkDestroyShaderModule(s_gpu_device, fs, VK_ALLOC_CALLBACK);

	if (r != VK_SUCCESS)
		return 0;

	return 1;
}

int create_skybox_generate_pipeline(void)
{
	VkShaderModule cs;
	cs = VK_NULL_HANDLE;
	VKU_Load_Shader(s_app->activity->assetManager, s_gpu_device, "Shader_GLSL/CS_Skybox_Generate.bil", &cs);

	if (cs == VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(s_gpu_device, cs, VK_ALLOC_CALLBACK);
		return 0;
	}

	VkPipelineShaderStageCreateInfo stage_info;
	stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.pNext = NULL;
	stage_info.flags = 0;
	stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage_info.module = cs;
	stage_info.pName = "main";
	stage_info.pSpecializationInfo = NULL;

	VkComputePipelineCreateInfo infoPipe;
	infoPipe.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	infoPipe.pNext = NULL;
	infoPipe.flags = 0;
	infoPipe.stage = stage_info;
	infoPipe.layout = s_common_pipeline_layout;
	infoPipe.basePipelineHandle = VK_NULL_HANDLE;
	infoPipe.basePipelineIndex = 0;

	VkResult r = vkCreateComputePipelines(s_gpu_device, VK_NULL_HANDLE, 1, &infoPipe, VK_ALLOC_CALLBACK, &s_skybox_generate_pipe);
	vkDestroyShaderModule(s_gpu_device, cs, VK_ALLOC_CALLBACK);

	if (r != VK_SUCCESS)
		return 0;

	return 1;
}

int create_skybox_image(void)
{
	VkImageCreateInfo image_info = {
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, NULL, 0,
		VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,{ 1024, 1024, 1 }, 1, 6, VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT |
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SHARING_MODE_EXCLUSIVE, 0,
		NULL, VK_IMAGE_LAYOUT_UNDEFINED
	};
	VK_VALIDATION_RESULT(vkCreateImage(s_gpu_device, &image_info, VK_ALLOC_CALLBACK, &s_skybox_image));
	if (!VKU_Alloc_Image_Object(s_image_mempool_texture, s_skybox_image, NULL, get_mem_type_index(s_gpu, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)))
		return 0;

	VkImageViewCreateInfo image_view_info = {
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, NULL, 0,
		s_skybox_image, VK_IMAGE_VIEW_TYPE_CUBE, VK_FORMAT_R8G8B8A8_UNORM,
		{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 }
	};
	VK_VALIDATION_RESULT(vkCreateImageView(s_gpu_device, &image_view_info, VK_ALLOC_CALLBACK, &s_skybox_image_view));

	return 1;
}

//Todo: need to implment the png image reading
int render_to_skybox_image(void)
{
	// Use stagging buffer to copy the skybox image
	struct {
		VkBuffer buffer;
		VkDeviceMemory memory;
	} staging_res;

	VkBufferCreateInfo buffer_info = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL, 0,
		1024 * 1024 * 4 * 6,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, 0, NULL
	};
	VK_VALIDATION_RESULT(vkCreateBuffer(s_gpu_device, &buffer_info, VK_ALLOC_CALLBACK, &staging_res.buffer));

	VkMemoryRequirements mreq_buffer = { 0 };
	vkGetBufferMemoryRequirements(s_gpu_device, staging_res.buffer, &mreq_buffer);

	VkMemoryAllocateInfo alloc_info_buffer = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, mreq_buffer.size,
		get_mem_type_index(s_gpu, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	};
	VK_VALIDATION_RESULT(vkAllocateMemory(s_gpu_device, &alloc_info_buffer, VK_ALLOC_CALLBACK, &staging_res.memory));
	VK_VALIDATION_RESULT(vkBindBufferMemory(s_gpu_device, staging_res.buffer, staging_res.memory, 0));

	uint8_t *ptr;
	VK_VALIDATION_RESULT(vkMapMemory(s_gpu_device, staging_res.memory, 0, VK_WHOLE_SIZE, 0, (void **)&ptr));

	const char *name[6] = {
		"Texture/Skybox_right1.png", "Texture/Skybox_left2.png",
		"Texture/Skybox_top3.png", "Texture/Skybox_bottom4.png",
		"Texture/Skybox_front5.png", "Texture/Skybox_back6.png"
	};

	VkDeviceSize offset = 0;
	std::array<VkBufferImageCopy, 6> buffer_copy_regions;
	for (int i = 0; i < 6; ++i) {
		int w, h, comp;
		stbi_uc* data = load_image(s_app->activity->assetManager, name[i], &w, &h, &comp, 4);
		if (!data)
			continue;

		size_t size = w * h * comp;
		memcpy(ptr + offset, data, size);

		buffer_copy_regions[i].bufferRowLength = 0;
		buffer_copy_regions[i].bufferImageHeight = 0;
		buffer_copy_regions[i].bufferOffset = size;
		buffer_copy_regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		buffer_copy_regions[i].imageSubresource.baseArrayLayer = i;
		buffer_copy_regions[i].imageSubresource.layerCount = 1;
		buffer_copy_regions[i].imageSubresource.mipLevel = 0;
		buffer_copy_regions[i].imageExtent.width = w;
		buffer_copy_regions[i].imageExtent.height = h;
		buffer_copy_regions[i].imageExtent.depth = 1;
		buffer_copy_regions[i].imageOffset.x = 0;
		buffer_copy_regions[i].imageOffset.y = 0;
		buffer_copy_regions[i].imageOffset.z = 0;
		
		offset += size;
		stbi_image_free(data);
	}
	vkUnmapMemory(s_gpu_device, staging_res.memory);

	VkImage optimal_image;
	VkImageCreateInfo image_info = {
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, NULL, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
		VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,{ 1024, 1024, 1 }, 1, 6, VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, 0,
		NULL, VK_IMAGE_LAYOUT_UNDEFINED
	};

	VK_VALIDATION_RESULT(vkCreateImage(s_gpu_device, &image_info, VK_ALLOC_CALLBACK, &optimal_image));

	VkMemoryRequirements mreq_image = { 0 };
	vkGetImageMemoryRequirements(s_gpu_device, optimal_image, &mreq_image);

	VkDeviceMemory optimal_image_mem;
	VkMemoryAllocateInfo alloc_info_image = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, mreq_image.size,
		get_mem_type_index(s_gpu, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};
	VK_VALIDATION_RESULT(vkAllocateMemory(s_gpu_device, &alloc_info_image, VK_ALLOC_CALLBACK, &optimal_image_mem));
	VK_VALIDATION_RESULT(vkBindImageMemory(s_gpu_device, optimal_image, optimal_image_mem, 0));

	// update the stagging buffer into the cubebox image
	VkCommandBufferAllocateInfo staggingCmdInfo;
	staggingCmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	staggingCmdInfo.pNext = NULL;
	staggingCmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	staggingCmdInfo.commandPool = s_command_pool;
	staggingCmdInfo.commandBufferCount = 1;

	VkCommandBuffer staggingCmd;
	vkAllocateCommandBuffers(s_gpu_device, &staggingCmdInfo, &staggingCmd);

	VkCommandBufferBeginInfo staggingBeginInfo = {};
	staggingBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	staggingBeginInfo.pNext = NULL;
	staggingBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	staggingBeginInfo.pInheritanceInfo = NULL;

	VkImageSubresourceRange skybox_subresource_range;
	skybox_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	skybox_subresource_range.baseMipLevel = 0;
	skybox_subresource_range.levelCount = 1;
	skybox_subresource_range.baseArrayLayer = 0;
	skybox_subresource_range.layerCount = 6;

	vkBeginCommandBuffer(staggingCmd, &staggingBeginInfo);

	setImageLayout(staggingCmd, optimal_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		 skybox_subresource_range);

	vkCmdCopyBufferToImage(staggingCmd, staging_res.buffer, optimal_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, buffer_copy_regions.size(), buffer_copy_regions.data());

	setImageLayout(staggingCmd, optimal_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
		skybox_subresource_range);

	vkEndCommandBuffer(staggingCmd);

	VkFence skyboxFence;
	VkFenceCreateInfo skyboxFenceInfo;
	skyboxFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	skyboxFenceInfo.pNext = NULL;
	skyboxFenceInfo.flags = 0;
	vkCreateFence(s_gpu_device, &skyboxFenceInfo, VK_ALLOC_CALLBACK, &skyboxFence);

	VkPipelineStageFlags skyboxPipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	VkSubmitInfo skyboxSubmitInfo = {};
	skyboxSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	skyboxSubmitInfo.pNext = NULL;
	skyboxSubmitInfo.commandBufferCount = 1;
	skyboxSubmitInfo.pCommandBuffers = &staggingCmd;

	vkQueueSubmit(s_gpu_queue, 1, &skyboxSubmitInfo, skyboxFence);

	vkWaitForFences(s_gpu_device, 1, &skyboxFence, true, UINT64_MAX);

	VkImageView optimal_image_view;
	VkImageViewCreateInfo image_view_info = {
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, NULL, 0,
		optimal_image, VK_IMAGE_VIEW_TYPE_CUBE, VK_FORMAT_R8G8B8A8_UNORM,
		{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 }
	};
	VK_VALIDATION_RESULT(vkCreateImageView(s_gpu_device, &image_view_info, VK_ALLOC_CALLBACK, &optimal_image_view));

	update_common_dset();

	return 1;
}

void cmd_render_skybox(VkCommandBuffer cmdbuf)
{
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, s_skybox_generate_pipe);
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, s_common_pipeline_layout, 0, 1, &s_common_dset[s_res_idx], 0, NULL);
	vkCmdDispatch(cmdbuf, 1, 1, 1);

	vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 0, NULL);


	VkCommandBufferBeginInfo begin_info = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL
	};

	VkRect2D render_area = { { 0, 0 },{ s_win_width, s_win_height } };
	VkClearValue clear_color[2];
	clear_color[0].color.float32[0] = 1.0;
	clear_color[0].color.float32[1] = 0.0;
	clear_color[0].color.float32[2] = 0.0;

	clear_color[1].depthStencil.depth = 1.0;
	clear_color[1].depthStencil.stencil = 0.0;

	VkRenderPassBeginInfo rpBegin;
	rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBegin.pNext = NULL;
	rpBegin.renderPass = s_float_renderpass;
	rpBegin.framebuffer = s_float_framebuffer;
	rpBegin.renderArea = render_area;
	rpBegin.clearValueCount = 2;
	rpBegin.pClearValues = clear_color;
	vkCmdBeginRenderPass(cmdbuf, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_skybox_pipe);
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_common_pipeline_layout, 0, 1, &s_common_dset[s_res_idx], 0, NULL);

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmdbuf, 0, 1, &s_skybox_buf, &offset);

	vkCmdDraw(cmdbuf, 14, 1, 0, 0);

	vkCmdEndRenderPass(cmdbuf);
}

int init_particle_thread(THREAD_DATA * thrd)
{
	VkCommandPoolCreateInfo command_pool_info;
	command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.pNext = NULL;
	command_pool_info.flags = 0;
	command_pool_info.queueFamilyIndex = s_queue_family_index;
	VK_VALIDATION_RESULT(vkCreateCommandPool(s_gpu_device, &command_pool_info, VK_ALLOC_CALLBACK, &thrd->cmdpool));

	VkCommandBufferAllocateInfo cmdbuf_info = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL, thrd->cmdpool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY, k_Resource_Buffering
	};
	VK_VALIDATION_RESULT(vkAllocateCommandBuffers(s_gpu_device, &cmdbuf_info, thrd->cmdbuf));

	// Todo: Can it be replaced by the Vulkan's synchronous mechanism?
	sem_init(&thrd->sem, 1, 0);

	return 1;
}

int finish_particle_thread(THREAD_DATA * thrd)
{
	//Todo: how to implement the thread exist thread
	if (s_exit_code == STARDUST_ERROR) {
		sem_post(&s_cmdgen_sem[thrd->tid]);
	}

	// No mechanism to wait for a std::thread.join()
	// SDL_WaitThread(thrd->thread, NULL);
	pthread_join(thrd->thread, NULL);
	return 0;
}

int release_particle_thread(THREAD_DATA * thrd)
{
	sem_destroy(&thrd->sem);
	//Todo: destroy the semaphore
	//SDL_DestroySemaphore(thrd->sem);
	//thrd->sem = NULL;

	vkFreeCommandBuffers(s_gpu_device, thrd->cmdpool, k_Resource_Buffering, thrd->cmdbuf);
	vkDestroyCommandPool(s_gpu_device, thrd->cmdpool, VK_ALLOC_CALLBACK);
	return 0;
}

int update_particle_thread(THREAD_DATA * thrd)
{
	VkCommandBufferBeginInfo begin_info = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL
	};

	VK_VALIDATION_RESULT(vkBeginCommandBuffer(thrd->cmdbuf[s_res_idx], &begin_info));

	VkRect2D render_area = { { 0, 0 },{ s_win_width, s_win_height } };
	VkClearValue clear_color = { 0 };
	VkRenderPassBeginInfo rpBegin;
	rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBegin.pNext = NULL;
	rpBegin.renderPass = s_float_renderpass;
	rpBegin.framebuffer = s_float_framebuffer;
	rpBegin.renderArea = render_area;
	rpBegin.clearValueCount = 2;
	rpBegin.pClearValues = &clear_color;
	vkCmdBeginRenderPass(thrd->cmdbuf[s_res_idx], &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(thrd->cmdbuf[s_res_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, s_particle_pipe);
	vkCmdBindDescriptorSets(thrd->cmdbuf[s_res_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, s_common_pipeline_layout,
		0, 1, &s_common_dset[s_res_idx], 0, NULL);

	VkDeviceSize offsets = 0;
	vkCmdBindVertexBuffers(thrd->cmdbuf[s_res_idx], 0, 1, &s_particle_seed_buf, &offsets);
	for (int i = 0; i < DRAW_PER_THREAD; ++i) {
		uint32_t firstVertex = (i + thrd->tid * DRAW_PER_THREAD) * k_Def_Batch_Size;
		vkCmdDraw(thrd->cmdbuf[s_res_idx], k_Def_Batch_Size, 1, firstVertex, 0);
	}

	vkCmdEndRenderPass(thrd->cmdbuf[s_res_idx]);
	VK_VALIDATION_RESULT(vkEndCommandBuffer(thrd->cmdbuf[s_res_idx]));

	return 1;
}

//Todo: How to update the thread value data
void* particle_thread(void * data)
{
	THREAD_DATA *thrd = (THREAD_DATA*)data;

#if defined(_WIN32)
	SetThreadAffinityMask(GetCurrentThread(), ((DWORD_PTR)1) << thrd->tid);
#endif

	do {
		sem_wait(&s_cmdgen_sem[thrd->tid]);
		update_particle_thread(thrd);
		sem_post(&thrd->sem);
	} while (s_exit_code == STARDUST_CONTINUE);
	return nullptr;
}

void update_swapChain()
{
	VkSemaphoreCreateInfo semaphoreInfo;
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = NULL;
	semaphoreInfo.flags = 0;
	VK_VALIDATION_RESULT(vkCreateSemaphore(s_gpu_device, &semaphoreInfo, VK_ALLOC_CALLBACK,
		&s_swap_chain_image_ready_semaphore));

	uint32_t swap_image_index;
	VK_VALIDATION_RESULT(vkAcquireNextImageKHR(s_gpu_device, s_swap_chain, UINT64_MAX,
		s_swap_chain_image_ready_semaphore, VK_NULL_HANDLE, &swap_image_index));

	s_res_idx = swap_image_index;
	s_win_idx = swap_image_index;
}

//Todo: Where to place the global state 
void cmd_clear(VkCommandBuffer cmdbuf)
{
	VkImageSubresourceRange color_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VkImageSubresourceRange depth_stencil_range[2] = {
		{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 },
		{ VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 },
	};

	if (!s_frame) 
	{
		for (int i = 0; i < k_Resource_Buffering; ++i) {
			VkImageSubresourceRange color_subresource_range;
			color_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			color_subresource_range.baseMipLevel = 0;
			color_subresource_range.levelCount = 1;
			color_subresource_range.baseArrayLayer = 0;
			color_subresource_range.layerCount = 1;

			setImageLayout(cmdbuf, s_win_images[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, color_subresource_range);
		}
	}

	VkClearColorValue clear_color = { { 1.0f, 0.0f, 0.0f, 1.0f } };
	//vkCmdClearColorImage(cmdbuf, s_win_images[s_win_idx], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &clear_color, 1, &color_range);
	if (!s_frame) {
		VkImageSubresourceRange color_subresource_range;
		color_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		color_subresource_range.baseMipLevel = 0;
		color_subresource_range.levelCount = 1;
		color_subresource_range.baseArrayLayer = 0;
		color_subresource_range.layerCount = 1;

		setImageLayout(cmdbuf, s_float_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, color_subresource_range);
	}

	if (!s_frame)
	{
		VkImageSubresourceRange ds_subresource_range;
		ds_subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		ds_subresource_range.baseMipLevel = 0;
		ds_subresource_range.levelCount = 1;
		ds_subresource_range.baseArrayLayer = 0;
		ds_subresource_range.layerCount = 1;

		setImageLayout(cmdbuf, s_depth_stencil_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, ds_subresource_range);
	}

	VkClearDepthStencilValue ds_value = { 1.0f, 0 };
	//vkCmdClearDepthStencilImage(cmdbuf, s_depth_stencil_image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &ds_value, 2, depth_stencil_range);
}

void cmd_display_fractal(VkCommandBuffer cmdbuf)
{
	VkCommandBufferBeginInfo begin_info = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL
	};

	VkRect2D render_area = { { 0, 0 },{ s_win_width, s_win_height } };
	VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 0.0f };
	VkRenderPassBeginInfo rpBegin;
	rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBegin.pNext = NULL;
	rpBegin.renderPass = s_win_renderpass;
	rpBegin.framebuffer = s_win_framebuffer[s_win_idx];
	rpBegin.renderArea = render_area;
	rpBegin.clearValueCount = 2;
	rpBegin.pClearValues = &clear_color;
	vkCmdBeginRenderPass(cmdbuf, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_display_pipe);
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_common_pipeline_layout, 0, 1, &s_common_dset[s_res_idx], 0, NULL);
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmdbuf, 0, 1, &s_skybox_buf, &offset);

	vkCmdDraw(cmdbuf, 4, 1, 0, 0);

	vkCmdEndRenderPass(cmdbuf);
}

int present(uint32_t * image_indice)
{
	VkResult result = VK_RESULT_MAX_ENUM;
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = NULL;
	present_info.waitSemaphoreCount = 0;
	present_info.pWaitSemaphores = NULL;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &s_swap_chain;
	if (image_indice == NULL)
		present_info.pImageIndices = &s_win_idx;
	else
		present_info.pImageIndices = image_indice;
	present_info.pResults = &result;

	VK_VALIDATION_RESULT(vkQueuePresentKHR(s_gpu_queue, &present_info));
	VK_VALIDATION_RESULT(result);

	//vkDestroySemaphore(s_gpu_device, s_swap_chain_image_ready_semaphore, VK_ALLOC_CALLBACK);

	return 1;
}

