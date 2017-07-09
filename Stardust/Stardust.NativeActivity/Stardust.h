#pragma once

// Copyright 2016 Intel Corporation All Rights Reserved
// 
// Intel makes no representations about the suitability of this software for any purpose.
// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,
// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY
// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Intel does not assume any responsibility for any errors which may appear in this software
// nor any responsibility to update it.

#pragma once

#include "Settings.h"

struct graph_data_t
{
	int sampleidx;
	float sample[k_Graph_Samples];
	float sampley[k_Graph_Samples];
	float scale;
	int empty_flag;
};

struct glob_state_t
{
	void *hwnd;
	ANativeWindow *window;
	int width;
	int height;

	int frame;
	float transform_time;
	int transform_animate;
	float palette_factor;
	int palette_image_idx;
	unsigned int seed;
	struct graph_data_t graph_data[MAX_CPU_CORES];

	int batch_size;
	int point_count;
	int windowed;
	int verbose;
	int cpu_core_count;
	android_app* app;
};

static int s_app_start_tics;
static int s_frame_begin_tics;
static int s_last_current_time_printf_tics;
static struct glob_state_t s_glob_state;

int application_Init(android_app* pApp, ANativeWindow* pWnd, int argc, char **argv);

int VK_Init();
int VK_Shutdown();
int VK_Run();

int update_frame_status(double *time, float *time_delta, int frame, int filter_fps, float *fps, float *ms);

// Todo: Handle event in android is different from the desktop
//int Handle_Events(struct glob_state_t *state);
