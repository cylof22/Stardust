#pragma once
#include <thread>
#include "glm\glm.hpp"
#include "Settings.h"
#include "Vulkan\Include\vulkan.h"

//=============================================================================
#define k_Window_Buffering k_Resource_Buffering
#define DRAW_PER_THREAD ((s_glob_state->point_count / s_glob_state->batch_size) / s_glob_state->cpu_core_count)
#define MT_UPDATE
//=============================================================================

typedef struct ViewportState
{
	uint32_t                           viewportCount;
	VkViewport                         viewport;
	uint32_t                           scissorCount;
	VkRect2D                           scissors;
} ViewportState;

typedef struct RasterState
{
	float                              depthBias;
	float                              depthBiasClamp;
	float                              slopeScaledDepthBias;
	float                              lineWidth;
} RasterState;

typedef struct ColorBlendState
{
	float                              blendConst[4];
} ColorBlendState;

typedef struct DepthStencilState
{
	float                              minDepthBounds;
	float                              maxDepthBounds;
} DepthStencilState;

typedef struct THREAD_DATA
{
	int                                 tid;
	VkCommandBuffer                     cmdbuf[k_Resource_Buffering];
	VkCommandPool                       cmdpool;
	// Todo: how to implement semaphore by using C++ thread library?
	// SDL_sem                             *sem;
	std::thread                          thread;
} THREAD_DATA;

typedef struct GRAPH_SHADER_IN
{
	glm::vec2                          p;
	glm::vec4                          c;
} GRAPH_SHADER_IN;

typedef struct GRAPH
{
	VkDeviceMemory                     buffer_mem[k_Resource_Buffering];
	VkBuffer                           buffer[k_Resource_Buffering];
	int                                x, y, w, h;
	float                              color[4];
	ViewportState                      viewport;
	int                                draw_background;
} GRAPH;

//=============================================================================
static VkInstance					   s_instance = VK_NULL_HANDLE;
static VkPhysicalDevice				   s_gpu = VK_NULL_HANDLE;
static VkDevice						   s_gpu_device = VK_NULL_HANDLE;
static VkQueue						   s_gpu_queue = VK_NULL_HANDLE;
static uint32_t						   s_queue_family_index = 0;
static VkSwapchainKHR				   s_swap_chain = { VK_NULL_HANDLE };
static int							   s_back_buffer = 0;
static VkSurfaceKHR                    s_surface = VK_NULL_HANDLE;
static VkPhysicalDeviceProperties      s_gpu_properties;
static VkImage                         s_win_images[k_Window_Buffering];
static VkImageView                     s_win_image_view[k_Window_Buffering];
static VkFramebuffer                   s_win_framebuffer[k_Window_Buffering];
//=============================================================================
int init_device(void* pWnd, int width, int height, VkBool32 windowed);
void deinit(void);
int init_framebuffer(void* pWnd, int width, int height, VkBool32 windowed, uint32_t image_count, VkImage *images);
