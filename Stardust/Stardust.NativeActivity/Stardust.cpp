#include "Stardust.h"
#include <chrono>
#include "vulkan_engine.h"
#include "Vulkan\vulkan_utils.h"
#include "Vulkan\vulkan_wrapper.h"

int application_Init(android_app* pApp, ANativeWindow* pWnd, int argc, char ** argv)
{
	s_glob_state.cpu_core_count = sysconf(_SC_NPROCESSORS_ONLN);
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
	init_device(s_glob_state.app,s_glob_state.window, s_glob_state.width, s_glob_state.height, s_glob_state.transform_time, s_glob_state.seed,
		0, true);

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
	/*for (int i = 1; i < s_glob_state.cpu_core_count; ++i) {
		pthread_create(&(s_thread[i].thread), NULL, &particle_thread, &(s_thread[i]));
	}*/
#endif

	while (s_exit_code == STARDUST_CONTINUE) {
		int recalculate_fps = update_frame_status(&s_time, &s_time_delta, s_frame,
			0, &s_fps, &s_ms);

		/*Set_Exit_Code(Handle_Events(s_glob_state));*/

		update_swapChain();

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

		if (!present(NULL)) {
			VK_LOG("VKU_Present failed\n");
			set_exit_code(STARDUST_ERROR);
		}
	}

	if (s_exit_code != STARDUST_EXIT)
	{
		uint32_t swap_image_index = 0xffffffff;
		present(&swap_image_index);
	}

	s_res_idx = 0;
	s_win_idx = 0;

#ifdef MT_UPDATE
	/*for (int i = 1; i < s_cpu_core_count; ++i) {
		finish_particle_thread(&s_thread[i]);
	}*/
#endif

	return s_exit_code;
}

int update_frame_status(double * time, float * time_delta, int frame, int filter_fps, float * fps, float * ms)
{
	static double prev_time;
	static double prev_fps_time;
	static int fps_frame = 0;

	std::chrono::time_point<std::chrono::system_clock> current_time = std::chrono::system_clock::now();
	s_frame_begin_tics = std::chrono::system_clock::to_time_t(current_time);
	
	if (frame == 0) {
		s_app_start_tics = s_frame_begin_tics;
		prev_time = s_frame_begin_tics * 0.001;
		prev_fps_time = s_frame_begin_tics * 0.001;
	}

	*time = s_frame_begin_tics * 0.001;
	*time_delta = (float)(*time - prev_time);
	prev_time = *time;

	if ((*time - prev_fps_time) >= 0.5) {
		float fps_unfiltered = fps_frame / (float)(*time - prev_fps_time);

		*fps = fps_unfiltered;
		*ms = 1.0f / fps_unfiltered * 1000;
		//Log("[%.1f fps %.3f ms]\n", *fps, *ms);

		prev_fps_time = *time;
		fps_frame = 1;
		return 1;
	}
	fps_frame++;
	return 0;
}
