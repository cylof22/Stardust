#pragma once
#include <thread>
#include <pthread.h>
#include <semaphore.h>
#include <glm\glm.hpp>
#include "Settings.h"
#include "Vulkan\Include\vulkan.h"
#include "Vulkan\vulkan_utils.h"

enum exit_code_t
{
	STARDUST_EXIT = 0,
	STARDUST_ERROR = 1,
	STARDUST_NOT_SUPPORTED,
	STARDUST_CONTINUE
};

//=============================================================================
#define k_Window_Buffering k_Resource_Buffering
#define DRAW_PER_THREAD ((k_Def_Point_Count / k_Def_Batch_Size) / 8) // Todo: how to get the cpu_count, use a definited value: 8
#define MT_UPDATE
//=============================================================================
#define RND_GEN(x) (x = x * 196314165 + 907633515)
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
	sem_t                               sem;
	pthread_t                           thread;
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
static android_app*                    s_app;
static double                          s_time;
static float                           s_time_delta;
static float                           s_fps;
static float                           s_ms;
static int                             s_exit_code;
//=============================================================================
static VkInstance					   s_instance = VK_NULL_HANDLE;
static VkPhysicalDevice				   s_gpu = VK_NULL_HANDLE;
static VkDevice						   s_gpu_device = VK_NULL_HANDLE;
static VkQueue						   s_gpu_queue = VK_NULL_HANDLE;
static uint32_t						   s_queue_family_index = 0;
static VkSwapchainKHR				   s_swap_chain = { VK_NULL_HANDLE };
static VkSemaphore                     s_swap_chain_image_ready_semaphore;
static int							   s_back_buffer = 0;
static VkSurfaceKHR                    s_surface = VK_NULL_HANDLE;
static VkPhysicalDeviceProperties      s_gpu_properties;
static VkImage                         s_win_images[k_Window_Buffering];
static VkImageView                     s_win_image_view[k_Window_Buffering];
static VkFramebuffer                   s_win_framebuffer[k_Window_Buffering];
static VkRenderPass                    s_win_renderpass;
static uint32_t                        s_win_width;
static uint32_t                        s_win_height;
static int                             s_res_idx;
static int                             s_win_idx;
static int                             s_cpu_core_count;
static int                             s_frame;
//=============================================================================
static VkImage                         s_depth_stencil_image;
static VkImageView                     s_depth_stencil_view;
//=============================================================================
static VkCommandPool                   s_command_pool;
static VkCommandBuffer                 s_cmdbuf_display[k_Resource_Buffering];
static VkCommandBuffer                 s_cmdbuf_clear[k_Resource_Buffering];
static VkFence                         s_fence[k_Resource_Buffering];
//=============================================================================
static VkDescriptorPool                s_common_dpool;
static VkDescriptorSetLayout           s_common_dset_layout;
static VkPipelineLayout                s_common_pipeline_layout;
static VkDescriptorSet                 s_common_dset[k_Resource_Buffering];
//=============================================================================
static VKU_BUFFER_MEMORY_POOL          *s_buffer_mempool_target;
static VKU_BUFFER_MEMORY_POOL          *s_buffer_mempool_state;
static VKU_BUFFER_MEMORY_POOL          *s_buffer_mempool_texture;
static VKU_IMAGE_MEMORY_POOL           *s_image_mempool_target;
static VKU_IMAGE_MEMORY_POOL           *s_image_mempool_state;
static VKU_IMAGE_MEMORY_POOL           *s_image_mempool_texture;
//=============================================================================
static VkImage                         s_float_image;
static VkImageView                     s_float_image_view;
static VkFramebuffer                   s_float_framebuffer;
static VkRenderPass                    s_float_renderpass;
static VkPipeline                      s_display_pipe;
//=============================================================================
static ViewportState                   s_vp_state;
static ViewportState                   s_vp_state_copy_skybox;
static ViewportState                   s_vp_state_copy_palette;
static ViewportState                   s_vp_state_legend_cpu;
static ViewportState                   s_vp_state_legend_gpu;
static RasterState                     s_rs_state;
static ColorBlendState                 s_cb_state;
static DepthStencilState               s_ds_state;
//=============================================================================
static VkDeviceMemory                  s_constant_mem[k_Resource_Buffering];
static VkBuffer                        s_constant_buf[k_Resource_Buffering];
//=============================================================================
// The copy renderpass is used to merge the six images into a cube box images for the skybox rendering
static VkRenderPass                     s_copy_renderpass; 
static VkPipeline                       s_copy_image_pipe;
static VkFramebuffer                    s_copy_framebuffer;
//=============================================================================
static VkImage                          s_skybox_image;
static VkImageView                      s_skybox_image_view;
static VkPipeline                       s_skybox_pipe;
static VkPipeline                       s_skybox_generate_pipe;
static VkDeviceMemory                   s_skybox_mem;
static VkBuffer                         s_skybox_buf;
//=============================================================================
static VkSampler                        s_sampler;
static VkSampler                        s_sampler_repeat;
static VkSampler                        s_sampler_nearest;
//=============================================================================
static THREAD_DATA                      s_thread[MAX_CPU_CORES];
static sem_t                            s_cmdgen_sem[MAX_CPU_CORES];
//=============================================================================
static VkDeviceMemory                   s_particle_seed_mem;
static VkBuffer                         s_particle_seed_buf;
static VkPipeline                       s_particle_pipe;
//=============================================================================
static glm::vec3                        s_camera_position;
static glm::vec3                        s_camera_direction;
static glm::vec3                        s_camera_right;
static float                            s_camera_pitch;
static float                            s_camera_yaw;
//=============================================================================
#define VM_PI 3.141592654f
#define VM_2PI 6.283185307f
#define VM_1DIVPI 0.318309886f
#define VM_1DIV2PI 0.159154943f
#define VM_PIDIV2 1.570796327f
#define VM_PIDIV4 0.785398163f
//=============================================================================
int engine_init(void);
int engine_shutdown(void);
int engine_update(void);
void update_camera(void);
//=============================================================================
int init_device(android_app* pApp, void* pWnd, int width, int height, VkBool32 windowed);
void deinit_device(void);
int init_framebuffer(void* pWnd, int width, int height, VkBool32 windowed, uint32_t image_count, VkImage *images);

int create_depth_stencil(void);
int create_common_dset(void);
int update_constant_memory(void);
void update_common_dset(void);
int create_float_renderpass(void);
int init_dynamic_states(void);
int create_display_renderpass(void);
int create_display_pipeline(void);

//Todo: particle should be possiblely placed in the Stardust.h
int create_particles(void);
int create_particle_pipeline(void);

// Todo: CPU load graph should be done in the engine part
int create_graph_pipeline(VkPrimitiveTopology topology, VkPipeline *pipe);

int create_window_framebuffer(void);

int create_constant_memory(void);

int create_float_image_and_framebuffer(void);

int create_copy_renderpass(void);
int create_copy_pipeline(void);

//int Create_Font_Resources(void);
//int Create_Font_Pipeline(void);

int create_skybox_geometry(void);
int create_skybox_pipeline(void);
int create_skybox_generate_pipeline(void);
int create_skybox_image(void);
int render_to_skybox_image(void);
void cmd_render_skybox(VkCommandBuffer cmdbuf);

//Todo: What's the palette meaning and the font used for?
//int Create_Palette_Images(void);
//int Render_To_Palette_Images(void);

int init_particle_thread(THREAD_DATA *thrd);
int finish_particle_thread(THREAD_DATA *thrd);
int release_particle_thread(THREAD_DATA *thrd);
int update_particle_thread(THREAD_DATA *thrd);
void* particle_thread(void *data);

void cmd_clear(VkCommandBuffer cmdbuf);
void cmd_display_fractal(VkCommandBuffer cmdbuf);

static inline int set_exit_code(int exit_code)
{
	assert(exit_code != STARDUST_ERROR);
	return s_exit_code = exit_code;
}


int present(uint32_t *image_indice);
