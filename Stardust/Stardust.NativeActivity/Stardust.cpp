#include "Stardust.h"
#include <thread>

#define RND_GEN(x) (x = x * 196314165 + 907633515)

int application_Init(ANativeWindow* pWnd, int argc, char ** argv)
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

	s_glob_state.window = pWnd;
	s_glob_state.height = ANativeWindow_getHeight(pWnd);
	s_glob_state.width = ANativeWindow_getWidth(pWnd);

	// Todo: CPU Metric Initialization

	// 
	return STARDUST_CONTINUE;
}

int VK_Init()
{

	return 0;
}

int VK_Shutdown()
{
	return 0;
}

int VK_Run()
{
	return 0;
}
