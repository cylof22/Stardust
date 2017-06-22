#include "Stardust.h"
#include "Vulkan\vulkan_utils.h"
#include "Vulkan\vulkan_wrapper.h"
#include <thread>

int application_Init(android_app* pApp, ANativeWindow* pWnd, int argc, char ** argv)
{
	s_glob_state.cpu_core_count = std::thread::hardware_concurrency();
	s_glob_state.transform_time = 4;
	s_glob_state.transform_animate = 1;
	s_glob_state.windowed = 1;
	s_glob_state.seed = 23232323;
	s_glob_state.batch_size = k_Def_Batch_Size;
	s_glob_state.point_count = k_Def_Point_Count;

	for (size_t i = 0; i < 6 * 9; i++)
		RND_GEN(s_glob_state.seed);

	for (size_t i = 0; i < s_glob_state.cpu_core_count; i++) {
		s_glob_state.graph_data[i].empty_flag = 1;
	}

	s_glob_state.app = pApp;

	s_glob_state.window = pWnd;
	s_glob_state.height = ANativeWindow_getHeight(pWnd);
	s_glob_state.width = ANativeWindow_getWidth(pWnd);

	// Todo: CPU Metric Initialization

	// 

	return STARDUST_CONTINUE;
}

int VK_Init()
{
	init_device(s_glob_state.app,s_glob_state.window, s_glob_state.width, s_glob_state.height, true);
	
	// Todo: Crash in Mali No call
	/*VkPhysicalDeviceProperties gpu_properties;
	vkGetPhysicalDeviceProperties(s_gpu, &s_gpu_properties);*/

	engine_init();

	return STARDUST_CONTINUE;
}

int VK_Shutdown()
{
	return 0;
}

int VK_Run()
{
#ifdef MT_UPDATE
	set_exit_code(STARDUST_CONTINUE);
	for (int i = 1; i < s_cpu_core_count; ++i) {
		pthread_create(&(s_thread[i].thread), NULL, &particle_thread, NULL);
	}
#endif

	while (s_exit_code == STARDUST_CONTINUE) {
		/*int recalculate_fps = Update_Frame_Stats(&s_time, &s_time_delta, s_glob_state->frame,
			0, &s_fps, &s_ms);

		Set_Exit_Code(Handle_Events(s_glob_state));*/

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

		if (!engine_update()) {
			VK_LOG("Demo_Update failed\n");
			set_exit_code(STARDUST_ERROR);
		}

		//s_glob_state->frame++;
		s_frame++;

		/*if (recalculate_fps) {
			const float *cpu_load = Metrics_GetCPUData();

			for (int i = 0; i < s_glob_state->cpu_core_count; i++) {
				s_glob_state->graph_data[i].scale = 1.0f;
				float s = cpu_load[i];
				Graph_Add_Sample(&s_glob_state->graph_data[i], s / 100.0f);
				Log("CPU %d: %f", i, cpu_load[i]);
			}
		}*/

		swap_image_index = s_win_idx;
		if (!present(&swap_image_index)) {
			VK_LOG("VKU_Present failed\n");
			set_exit_code(STARDUST_ERROR);
		}

		vkDestroySemaphore(s_gpu_device, s_swap_chain_image_ready_semaphore, VK_ALLOC_CALLBACK);
	}

	if (s_exit_code != STARDUST_EXIT)
	{
		uint32_t swap_image_index = 0xffffffff;
		present(&swap_image_index);
	}

	s_res_idx = 0;
	s_win_idx = 0;

#ifdef MT_UPDATE
	for (int i = 1; i < s_cpu_core_count; ++i) {
		finish_particle_thread(&s_thread[i]);
	}
#endif

	return s_exit_code;
}
