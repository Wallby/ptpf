#include "ptpf_loader.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifdef PTPF_USEVULKAN
#define VSP_DIRECT_ACCESS
#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan_script_pipeline_for_ptpf_loader.h>
#endif
#if defined(_WIN32)
#include <winsock2.h>
#else
//#include <xcb/xcb.h>
#include <X11/Xlib.h>
#endif

#pragma GCC diagnostic ignored "-Wdelete-incomplete"

// NOTE: tcp-mini only supports windows and linux, hence we only need to handle these two platforms (if tcp-mini doesn't compile,..
//       .. you can't compile ptpf-server)


#define PTPF_ALWAYS_INLINE gnu::always_inline

[[PTPF_ALWAYS_INLINE]]
inline void* operator new[](std::size_t a)
{
return malloc(a);
}
[[PTPF_ALWAYS_INLINE]]
inline void* operator new(std::size_t a) //< can throw std::bad_alloc?
{
	return malloc(a);
}
[[PTPF_ALWAYS_INLINE]]
inline void operator delete(void* a)
{
	free(a);
}
[[PTPF_ALWAYS_INLINE]]
inline void operator delete(void* a, std::size_t)
{
	free(a); //< there is no check here on purpose, please catch this issue elsewhere
}

namespace
{
	template<typename T>
	T min(T a, T b)
	{
		return a < b ? a : b;
	}
	template<typename T>
	T max(T a, T b)
	{
		return a > b ? a : b;
	}
	template<typename T>
	T clamp(T a, T minValue, T maxValue)
	{
		return min(max(a, minValue), maxValue);
	}

	// NOTE: T& is a reference to adress "decay issue" (where T[] becomes T*) as explained here http://www.cplusplus.com/articles/D4SGz8AR/
	template<typename T, int A>
	int length(T (&a)[A])
	{
		return sizeof a/sizeof *a;
	}
}

//*****************************************************************************

//extern char* ptpf_application_name;
extern int ptpf_scriptpipeline_api;
extern int ptpf_presentation_api;
extern int (*ptpf_first_called_function)(int argc, char** argv);
extern "C" int ptpf_main(int, char**);
extern void (*ptpf_last_called_function)();

extern void(*ptpf_on_receive)(struct tm_message_t*, int);

extern ptpf_view_t ptpf_view;

//************************* graphics frame applying ***************************

namespace
{
#ifdef PTPF_USEVULKAN
	struct vulkan_t
	{
		VkInstance a;
		VkPhysicalDevice c;
		VkDevice d;
		VkCommandPool e;
		struct
		{
			VkQueue a;
			int b; //< i.e. family index
		} presentQueue;
		VkPresentModeKHR presentMode;
		struct
		{
			VkQueue a;
			int b; //< i.e. family index
		} graphicsQueue;
	};
	struct vulkan_t vulkan;

#if defined(_WIN32)
#else
	char VK_KHR_XLIB_SURFACE_EXTENSION_NAME_STR[] = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
#endif
	char VK_KHR_SURFACE_EXTENSION_NAME_STR[] = VK_KHR_SURFACE_EXTENSION_NAME;
	char* additional_instance_extensions[] =
		{
#if defined(_WIN32)
#else
			VK_KHR_XLIB_SURFACE_EXTENSION_NAME_STR,
#endif
			VK_KHR_SURFACE_EXTENSION_NAME_STR
		};

	char VK_KHR_SWAPCHAIN_EXTENSION_NAME_STR[] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	char* additional_device_extensions[] =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME_STR
		};

	int additional_queue_families[1];
#endif
}

#ifdef PTPF_USEVULKAN
char** vsp_additional_instance_extensions = additional_instance_extensions;
int vsp_num_additional_instance_extensions = length(additional_instance_extensions);

char** vsp_additional_device_extensions = additional_device_extensions;
int vsp_num_additional_device_extensions = length(additional_device_extensions);

int* vsp_additional_queue_families = additional_queue_families;
int vsp_num_additional_queue_families = length(additional_queue_families);
#endif

namespace
{
	struct cached_view_settings_t
	{
		int bIsArmed = 0;
#ifdef PTPF_USEVULKAN
		struct
		{
			VkSurfaceKHR a;
			VkSurfaceFormatKHR format;
		} surface;
		struct
		{
			VkSwapchainKHR a;
			VkImageView b[2];
			VkFramebuffer c[2];
			VkCommandBuffer d[2];
		} swapchain;
		struct
		{
			VkShaderModule a;
		} vertexShader, fragmentShader;
		VkPipelineLayout a;
		VkRenderPass b;
		VkPipeline c; //< graphics pipeline
		VkSemaphore d; //< i.e. to schedule queue after image is available (and "pipeline is free" because of mask)
		VkSemaphore e;  //< i.e. to schedule present after render job done
		// NOTE: d will wait until image was supplied by swapchain, hence..
		//       .. present is guarded implicitely..?
#endif
	};
	cached_view_settings_t cachedViewSettings;

#ifdef PTPF_USEVULKAN
	void my_on_provide_graphics_queue(VkQueue a, int b)
	{
		vulkan.graphicsQueue.a = a;
		vulkan.graphicsQueue.b = b;

		//<plugin name>_on_provide_graphics_queue
	}
	void my_on_take_graphics_queue()
	{
		//<plugin name_on_take_graphics_queue

		vulkan.graphicsQueue.a = VK_NULL_HANDLE;
	}

	// IDEA: int ptpf_builtin_check_physical_device(VkPhysicalDevice a)
	int my_check_physical_device(VkPhysicalDevice a)
	{
		if(cachedViewSettings.surface.a == VK_NULL_HANDLE)
		{
			return 1; //< i.e. don't actually check (i.e. view functionality is not available)
		}

		uint32_t b;
		vkGetPhysicalDeviceQueueFamilyProperties(a, &b, NULL);

		for(uint32_t i = 0; i < b; ++i)
		{
			VkBool32 c;
			vkGetPhysicalDeviceSurfaceSupportKHR(a, i, cachedViewSettings.surface.a, &c);
			if(c == VK_FALSE)
			{
				continue;
			}

			uint32_t e;
			vkGetPhysicalDeviceSurfaceFormatsKHR(a, cachedViewSettings.surface.a, &e, NULL);

			if(e == 0)
			{
				continue;
			}

			uint32_t f;
			vkGetPhysicalDeviceSurfacePresentModesKHR(a, cachedViewSettings.surface.a, &f, NULL);

			if(f == 0)
			{
				continue;
			}

			//VkSurfaceFormatKHR* g = new VkSurfaceFormatKHR[e];
			VkSurfaceFormatKHR* g = (VkSurfaceFormatKHR*)new char[sizeof(VkSurfaceFormatKHR) * e];

			vkGetPhysicalDeviceSurfaceFormatsKHR(a, cachedViewSettings.surface.a, &e, g);

			/*
			for(int j = 0; j < e; ++j)
			{
				if(g[j].format == VK_FORMAT_R8G8B8A8_SRGB && g[j].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{

				}
			}*/

			cachedViewSettings.surface.format = g[0];

			delete g;

			//VkPresentModeKHR* h = new VkPresentModeKHR[f];
			VkPresentModeKHR* h = (VkPresentModeKHR*)new char[sizeof(VkPresentModeKHR) * f];

			vkGetPhysicalDeviceSurfacePresentModesKHR(a, cachedViewSettings.surface.a, &f, h);

			/*
			for(int j = 0; j < f; ++j)
			{
				if(h[j] = VK_PRESENT_MODE_MAILBOX_KHR)
				{

				}
			}*/

			// NOTE: VK_PRESENT_MODE_FIFO_KHR "is guaranteed to..
			//       .. be available" according to https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain
			vulkan.presentMode = VK_PRESENT_MODE_FIFO_KHR;

			delete h;

			// NOTE: will succeed (i.e. this function will..
			//       .. only be run if device is already valid..
			//       .. for vsp)

			vulkan.c = a;

			vsp_additional_queue_families[0] = i;
			vulkan.presentQueue.b = i;

			return 1;
		}

		return 0;
	}

	int disarm_view_for_vsp()
	{
		vkDeviceWaitIdle(vulkan.d);

		if(cachedViewSettings.e != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(vulkan.d, cachedViewSettings.e, NULL);
		}

		if(cachedViewSettings.d != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(vulkan.d, cachedViewSettings.d, NULL);
		}

		if(vulkan.e != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(vulkan.d, vulkan.e, NULL);
		}

		if(cachedViewSettings.c != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(vulkan.d, cachedViewSettings.c, NULL);
		}

		vkDestroyRenderPass(vulkan.d, cachedViewSettings.b, NULL);
		vkDestroyPipelineLayout(vulkan.d, cachedViewSettings.a, NULL);

		vkDestroyShaderModule(vulkan.d, cachedViewSettings.fragmentShader.a, NULL);
		vkDestroyShaderModule(vulkan.d, cachedViewSettings.vertexShader.a, NULL);

		for(int i = 0; i < 2; ++i)
		{
			if(cachedViewSettings.swapchain.b[i] != VK_NULL_HANDLE)
			{
				vkDestroyImageView(vulkan.d, cachedViewSettings.swapchain.b[i], NULL);
			}
		}

		vkDestroySwapchainKHR(vulkan.d, cachedViewSettings.swapchain.a, NULL);

		return 1;
	}

	int arm_view_for_vsp()
	{
		int(*create_swapchain)(VkSwapchainKHR*) = [](VkSwapchainKHR* a)
			{
				VkSurfaceCapabilitiesKHR b;
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan.c, cachedViewSettings.surface.a, &b);

				VkExtent2D extent;
				extent.width = ptpf_view.widthInPixels;
				extent.height = ptpf_view.heightInPixels;

				clamp(extent.width, b.minImageExtent.width, b.maxImageExtent.width);
				clamp(extent.height, b.minImageExtent.height, b.maxImageExtent.height);

				if((int)extent.width != ptpf_view.widthInPixels ||
				   (int)extent.height != ptpf_view.heightInPixels)
				{
					ptpf_view.widthInPixels = extent.width;
					ptpf_view.heightInPixels = extent.height;

					fputs("warning: view \"width and/or height\" was clamped", stdout);
				}

				if(b.maxImageCount != 0 && b.maxImageCount < 2)
				{
					return 0;
				}
				if(b.minImageCount > 2)
				{
					return 0;
				}
				// NOTE: two images are needed (drawing directly to screen is not desired)..?

				VkSwapchainCreateInfoKHR d;
				d.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
				d.pNext = NULL;
				d.flags = 0;
				d.surface = cachedViewSettings.surface.a;
				d.minImageCount = 2;
				d.imageFormat = cachedViewSettings.surface.format.format;
				d.imageColorSpace = cachedViewSettings.surface.format.colorSpace;
				d.imageExtent = extent;
				d.imageArrayLayers = 1; //< 2;..? //< for stereoscopic 3D?
				d.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

				if(vulkan.graphicsQueue.b != vulkan.presentQueue.b)
				{
					d.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
					d.queueFamilyIndexCount = 2;
					uint32_t l[2] = {(uint32_t)vulkan.graphicsQueue.b, (uint32_t)vulkan.presentQueue.b};
					d.pQueueFamilyIndices = l;
				}
				else
				{
					d.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
					d.queueFamilyIndexCount = 0;
					d.pQueueFamilyIndices = NULL;
				}

				d.preTransform = b.currentTransform;
				d.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
				d.presentMode = vulkan.presentMode;
				d.clipped = VK_TRUE;
				d.oldSwapchain = VK_NULL_HANDLE;

				VkSwapchainKHR c;

				if(vkCreateSwapchainKHR(vulkan.d, &d, NULL, &c) != VK_SUCCESS)
				{
					return 0;
				}

				uint32_t e;
				vkGetSwapchainImagesKHR(vulkan.d, c, &e, NULL);

				if(e != 2) //< i.e. enforce two images
				{
					vkDestroySwapchainKHR(vulkan.d, c, NULL);
					return 0;
				}

				*a = c;

				return 1;
			};

		int(*create_image_view)(VkImage, VkImageView*) = [](VkImage a, VkImageView* b)
			{
				VkImageViewCreateInfo c;
				c.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				c.pNext = NULL;
				c.flags = 0;
				c.image = a;
				c.viewType = VK_IMAGE_VIEW_TYPE_2D;
				c.format = cachedViewSettings.surface.format.format;
				c.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				c.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				c.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				c.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				c.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				c.subresourceRange.baseMipLevel = 0;
				c.subresourceRange.levelCount = 1;
				c.subresourceRange.baseArrayLayer = 0;
				c.subresourceRange.layerCount = 1;

				VkImageView d;

				if(vkCreateImageView(vulkan.d, &c, NULL, &d) != VK_SUCCESS)
				{
					return 0;
				}

				*b = d;

				return 1;
			};

		// NOTE: returns 1 if succeeded
		//       returns 0 if something went wrong
		int(*create_shader_module)(unsigned int*, int, VkShaderModule*) = [](unsigned int* code, int codeLength, VkShaderModule* a)
			{
				VkShaderModuleCreateInfo b;
				b.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				b.pNext = 0;
				b.flags = 0;
				b.codeSize = codeLength * sizeof(uint32_t);
				b.pCode = code;

				VkShaderModule c;
				if(vkCreateShaderModule(vulkan.d, &b, NULL, &c) != VK_SUCCESS)
				{
					return 0;
				}

				*a = c;

				return 1;
			};

		int (*create_framebuffer)(VkImageView, VkFramebuffer*) = [](VkImageView a, VkFramebuffer* b)
			{
				VkFramebufferCreateInfo c;
				c.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				c.pNext = NULL;
				c.flags = 0;
				c.renderPass = cachedViewSettings.b;
				c.attachmentCount = 1;
				c.pAttachments = &a;
				c.width = ptpf_view.widthInPixels;
				c.height = ptpf_view.heightInPixels;
				c.layers = 1;

				VkFramebuffer d;

				if(vkCreateFramebuffer(vulkan.d, &c, NULL, &d) != VK_SUCCESS)
				{
					return 0;
				}

				*b = d;
				return 1;
			};

		int(*create_everything_else)(VkPipeline*, VkPipelineLayout*, VkRenderPass*) = [](VkPipeline* a, VkPipelineLayout* b, VkRenderPass* c)
			{
				VkPipelineLayoutCreateInfo e;
				e.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				e.pNext = NULL;
				e.flags = 0;
				e.setLayoutCount = 0;
				e.pSetLayouts = NULL;
				e.pushConstantRangeCount = 0;
				e.pPushConstantRanges = NULL;

				VkPipelineLayout f;
				if(vkCreatePipelineLayout(vulkan.d, &e, NULL, &f) != VK_SUCCESS)
				{
					return 0;
				}

				VkAttachmentDescription r;
				r.flags = 0;
				r.format = cachedViewSettings.surface.format.format;
				r.samples = VK_SAMPLE_COUNT_1_BIT;
				r.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				r.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				r.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				r.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				r.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				r.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

				VkAttachmentReference s;
				s.attachment = 0;
				s.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				VkSubpassDescription t;
				t.flags = 0;
				t.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				t.inputAttachmentCount = 0;
				t.pInputAttachments = NULL;
				t.colorAttachmentCount = 1;
				t.pColorAttachments = &s;
				t.pResolveAttachments = NULL;
				t.pDepthStencilAttachment = NULL;
				t.preserveAttachmentCount = 0;
				t.pPreserveAttachments = NULL;

				VkSubpassDependency d;
				d.srcSubpass = VK_SUBPASS_EXTERNAL;
				d.dstSubpass = 0;
				d.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				d.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				d.srcAccessMask = 0;
				d.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

				VkRenderPassCreateInfo u;
				u.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				u.pNext = NULL;
				u.flags = 0;
				u.attachmentCount = 1;
				u.pAttachments = &r;
				u.subpassCount = 1;
				u.pSubpasses = &t;
				u.dependencyCount = 1;
				u.pDependencies = &d;

				VkRenderPass v;

				if(vkCreateRenderPass(vulkan.d, &u, NULL, &v) != VK_SUCCESS)
				{
					return 0;
				}

				VkPipelineShaderStageCreateInfo g[2];
				g[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				g[0].pNext = NULL;
				g[0].flags = 0;
				g[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
				g[0].module = cachedViewSettings.vertexShader.a;
				g[0].pName = "main";
				g[0].pSpecializationInfo = NULL;

				g[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				g[1].pNext = NULL;
				g[1].flags = 0;
				g[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				g[1].module = cachedViewSettings.fragmentShader.a;
				g[1].pName = "main";
				g[1].pSpecializationInfo = NULL;

				VkPipelineVertexInputStateCreateInfo h;
				h.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				h.pNext = NULL;
				h.flags = 0;
				h.vertexBindingDescriptionCount = 0;
				h.pVertexBindingDescriptions = NULL;
				h.vertexAttributeDescriptionCount = 0;
				h.pVertexAttributeDescriptions = NULL;

				VkPipelineInputAssemblyStateCreateInfo j;
				j.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				j.pNext = NULL;
				j.flags = 0;
				j.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				//j.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; //< for quads..?
				j.primitiveRestartEnable = VK_FALSE;

				VkViewport k;
				k.x = 0.0f;
				k.y = 0.0f;
				k.width = (float)ptpf_view.widthInPixels;
				k.height = (float)ptpf_view.heightInPixels;
				k.minDepth = 0.0f;
				k.maxDepth = 1.0f;

				VkRect2D l; //< "scissor rectangle"
				l.offset.x = 0;
				l.offset.y = 0;
				l.extent.width = (float)ptpf_view.widthInPixels;
				l.extent.height = (float)ptpf_view.heightInPixels;

				VkPipelineViewportStateCreateInfo m;
				m.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				m.pNext = NULL;
				m.flags = 0;
				m.viewportCount = 1;
				m.pViewports = &k;
				m.scissorCount = 1;
				m.pScissors = &l;

				VkPipelineRasterizationStateCreateInfo n;
				n.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				n.pNext = NULL;
				n.flags = 0;
				n.depthClampEnable = VK_FALSE;
				n.rasterizerDiscardEnable = VK_FALSE;
				n.polygonMode = VK_POLYGON_MODE_FILL;
				n.cullMode = VK_CULL_MODE_NONE;
				n.frontFace = VK_FRONT_FACE_CLOCKWISE;
				n.depthBiasEnable = VK_FALSE;
				n.depthBiasConstantFactor = 0.0f;
				n.depthBiasClamp = 0.0f;
				n.depthBiasSlopeFactor = 0.0f;
				n.lineWidth = 0.0f; //< not rendering lines so not relevant?

				VkPipelineMultisampleStateCreateInfo o;
				o.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				o.pNext = NULL;
				o.flags = 0;
				o.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
				o.sampleShadingEnable = VK_FALSE;
				//o.minSampleShading = 0.0f;
				o.pSampleMask = NULL;
				o.alphaToCoverageEnable = VK_FALSE;
				o.alphaToOneEnable = VK_FALSE;

				VkPipelineColorBlendAttachmentState p;
				p.blendEnable = VK_FALSE;
				//p.srcColorBlendFactor = 0.0f;
				//p.dstColorBlendFactor = 0.0f;
				//p.colorBlendOp = VK_BLEND_OP_ADD;
				//p.srcAlphaBlendFactor = 0.0f;
				//p.dstAlphaBlendFactor = 0.0f;
				//p.alphaBlendOp = VK_BLEND_OP_ADD;
				p.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

				VkPipelineColorBlendStateCreateInfo q;
				q.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				q.pNext = NULL;
				q.flags = 0;
				q.logicOpEnable = VK_FALSE;
				//q.logicOp = VK_LOGIC_OP_CLEAR;
				q.attachmentCount = 1;
				q.pAttachments = &p;
				//q.blendConstants[0] = 0.0f;
				//q.blendConstants[1] = 0.0f;
				//q.blendConstants[2] = 0.0f;
				//q.blendConstants[3] = 0.0f;

				VkGraphicsPipelineCreateInfo w;
				w.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				w.pNext = NULL;
				w.flags = 0;
				w.stageCount = length(g);
				w.pStages = g;
				w.pVertexInputState = &h;
				w.pInputAssemblyState = &j;
				w.pTessellationState = NULL;
				w.pViewportState = &m;
				w.pRasterizationState = &n;
				w.pMultisampleState = &o;
				w.pDepthStencilState = NULL;
				w.pColorBlendState = &q;
				w.pDynamicState = NULL;
				w.layout = f;
				w.renderPass = v;
				w.subpass = 0;
				w.basePipelineHandle = VK_NULL_HANDLE;
				w.basePipelineIndex = -1;

				VkPipeline x;

				if(vkCreateGraphicsPipelines(vulkan.d, VK_NULL_HANDLE, 1, &w, NULL, &x) != VK_SUCCESS)
				{
					return 0;
				}

				*a = x;
				*b = f;
				*c = v;

				return 1;
			};

		if(create_swapchain(&cachedViewSettings.swapchain.a) == 0)
		{
			return 0;
		}

		uint32_t a = 2;
		VkImage b[2];
		vkGetSwapchainImagesKHR(vulkan.d, cachedViewSettings.swapchain.a, &a, b);

		for(int i = 0; i < 2; ++i)
		{
			VkImageView t;
			if(create_image_view(b[i], &t) == 0)
			{
				disarm_view_for_vsp();
				return 0;
			}

			cachedViewSettings.swapchain.b[i] = t;
		}

		unsigned int a_spv[] = {0x7230203,0x10000,0x80008,0x45,0x0,0x20011,0x1,0x6000b,0x1,0x4c534c47,0x6474732e,0x3035342e,0x0,0x3000e,0x0,0x1,0x8000f,0x0,0x4,0x6e69616d,0x0,0x2b,0x35,0x40,0x30003,0x2,0x1c2,0x40005,0x4,0x6e69616d,0x0,0x60005,0xb,0x65646e69,0x72655078,0x74726556,0x7865,0x70005,0x16,0x69736f70,0x6e6f6974,0x56726550,0x65747265,0x78,0x60005,0x21,0x6f6c6f63,0x72655072,0x74726556,0x7865,0x30005,0x29,0x61,0x60005,0x2b,0x565f6c67,0x65747265,0x646e4978,0x7865,0x60005,0x33,0x505f6c67,0x65567265,0x78657472,0x0,0x60006,0x33,0x0,0x505f6c67,0x7469736f,0x6e6f69,0x70006,0x33,0x1,0x505f6c67,0x746e696f,0x657a6953,0x0,0x70006,0x33,0x2,0x435f6c67,0x4470696c,0x61747369,0x65636e,0x70006,0x33,0x3,0x435f6c67,0x446c6c75,0x61747369,0x65636e,0x30005,0x35,0x0,0x50005,0x40,0x4374756f,0x726f6c6f,0x0,0x40047,0x2b,0xb,0x2a,0x50048,0x33,0x0,0xb,0x0,0x50048,0x33,0x1,0xb,0x1,0x50048,0x33,0x2,0xb,0x3,0x50048,0x33,0x3,0xb,0x4,0x30047,0x33,0x2,0x40047,0x40,0x1e,0x0,0x20013,0x2,0x30021,0x3,0x2,0x40015,0x6,0x20,0x1,0x40015,0x7,0x20,0x0,0x4002b,0x7,0x8,0x6,0x4001c,0x9,0x6,0x8,0x40020,0xa,0x6,0x9,0x4003b,0xa,0xb,0x6,0x4002b,0x6,0xc,0x0,0x4002b,0x6,0xd,0x1,0x4002b,0x6,0xe,0x2,0x4002b,0x6,0xf,0x3,0x9002c,0x9,0x10,0xc,0xd,0xe,0xe,0xf,0xc,0x30016,0x11,0x20,0x40017,0x12,0x11,0x2,0x4002b,0x7,0x13,0x4,0x4001c,0x14,0x12,0x13,0x40020,0x15,0x6,0x14,0x4003b,0x15,0x16,0x6,0x4002b,0x11,0x17,0xbf800000,0x5002c,0x12,0x18,0x17,0x17,0x4002b,0x11,0x19,0x3f800000,0x5002c,0x12,0x1a,0x19,0x17,0x5002c,0x12,0x1b,0x19,0x19,0x5002c,0x12,0x1c,0x17,0x19,0x7002c,0x14,0x1d,0x18,0x1a,0x1b,0x1c,0x40017,0x1e,0x11,0x3,0x4001c,0x1f,0x1e,0x13,0x40020,0x20,0x6,0x1f,0x4003b,0x20,0x21,0x6,0x4002b,0x11,0x22,0x0,0x6002c,0x1e,0x23,0x22,0x22,0x22,0x6002c,0x1e,0x24,0x19,0x22,0x22,0x6002c,0x1e,0x25,0x22,0x19,0x22,0x6002c,0x1e,0x26,0x19,0x19,0x22,0x7002c,0x1f,0x27,0x23,0x24,0x25,0x26,0x40020,0x28,0x7,0x6,0x40020,0x2a,0x1,0x6,0x4003b,0x2a,0x2b,0x1,0x40020,0x2d,0x6,0x6,0x40017,0x30,0x11,0x4,0x4002b,0x7,0x31,0x1,0x4001c,0x32,0x11,0x31,0x6001e,0x33,0x30,0x11,0x32,0x32,0x40020,0x34,0x3,0x33,0x4003b,0x34,0x35,0x3,0x40020,0x37,0x6,0x12,0x40020,0x3d,0x3,0x30,0x40020,0x3f,0x3,0x1e,0x4003b,0x3f,0x40,0x3,0x40020,0x42,0x6,0x1e,0x50036,0x2,0x4,0x0,0x3,0x200f8,0x5,0x4003b,0x28,0x29,0x7,0x3003e,0xb,0x10,0x3003e,0x16,0x1d,0x3003e,0x21,0x27,0x4003d,0x6,0x2c,0x2b,0x50041,0x2d,0x2e,0xb,0x2c,0x4003d,0x6,0x2f,0x2e,0x3003e,0x29,0x2f,0x4003d,0x6,0x36,0x29,0x50041,0x37,0x38,0x16,0x36,0x4003d,0x12,0x39,0x38,0x50051,0x11,0x3a,0x39,0x0,0x50051,0x11,0x3b,0x39,0x1,0x70050,0x30,0x3c,0x3a,0x3b,0x22,0x19,0x50041,0x3d,0x3e,0x35,0xc,0x3003e,0x3e,0x3c,0x4003d,0x6,0x41,0x29,0x50041,0x42,0x43,0x21,0x41,0x4003d,0x1e,0x44,0x43,0x3003e,0x40,0x44,0x100fd,0x10038};

		if(create_shader_module(a_spv, length(a_spv), &cachedViewSettings.vertexShader.a) == 0)
		{
			disarm_view_for_vsp();
			return 0;
		}

		unsigned int b_spv[] = {0x7230203,0x10000,0x80008,0x13,0x0,0x20011,0x1,0x6000b,0x1,0x4c534c47,0x6474732e,0x3035342e,0x0,0x3000e,0x0,0x1,0x7000f,0x4,0x4,0x6e69616d,0x0,0x9,0xc,0x30010,0x4,0x7,0x30003,0x2,0x1c2,0x40005,0x4,0x6e69616d,0x0,0x50005,0x9,0x4374756f,0x726f6c6f,0x0,0x40005,0xc,0x6f436e69,0x726f6c,0x40047,0x9,0x1e,0x0,0x40047,0xc,0x1e,0x0,0x20013,0x2,0x30021,0x3,0x2,0x30016,0x6,0x20,0x40017,0x7,0x6,0x4,0x40020,0x8,0x3,0x7,0x4003b,0x8,0x9,0x3,0x40017,0xa,0x6,0x3,0x40020,0xb,0x1,0xa,0x4003b,0xb,0xc,0x1,0x4002b,0x6,0xe,0x3f800000,0x50036,0x2,0x4,0x0,0x3,0x200f8,0x5,0x4003d,0xa,0xd,0xc,0x50051,0x6,0xf,0xd,0x0,0x50051,0x6,0x10,0xd,0x1,0x50051,0x6,0x11,0xd,0x2,0x70050,0x7,0x12,0xf,0x10,0x11,0xe,0x3003e,0x9,0x12,0x100fd,0x10038};

		if(create_shader_module(b_spv, length(b_spv), &cachedViewSettings.fragmentShader.a) == 0)
		{
			disarm_view_for_vsp();
			return 0;
		}

		if(create_everything_else(&cachedViewSettings.c, &cachedViewSettings.a, &cachedViewSettings.b) != 1)
		{
			disarm_view_for_vsp();
			return 0;
		}

		for(int i = 0; i < 2; ++i)
		{
			if(create_framebuffer(cachedViewSettings.swapchain.b[i], &cachedViewSettings.swapchain.c[i]) == 0)
			{
				disarm_view_for_vsp();
				return 0;
			}
		}

		VkCommandPoolCreateInfo c;
		c.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		c.pNext = NULL;
		c.flags = 0;
		c.queueFamilyIndex = vulkan.graphicsQueue.b;

		VkCommandPool d;

		if(vkCreateCommandPool(vulkan.d, &c, NULL, &d) != VK_SUCCESS)
		{
			disarm_view_for_vsp();
			return 0;
		}

		vulkan.e = d;

		VkCommandBuffer f[2];

		VkCommandBufferAllocateInfo g;
		g.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		g.pNext = NULL;
		g.commandPool = vulkan.e;
		g.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		g.commandBufferCount = 2;

		if(vkAllocateCommandBuffers(vulkan.d, &g, f) != VK_SUCCESS)
		{
			disarm_view_for_vsp();

			return 0;
		}

		memcpy(cachedViewSettings.swapchain.d, f, sizeof(VkCommandBuffer)*2);

		// if clip coordinates -> ndc is w. <clip coordinates>..
		// ../<ndc>.. and <clip coordinates>.z is "far z" -..
		// .. "near z".. is <clip coordinates>.xy "resolution"..
		// .. * ("far z" - "near z")?

		for(int i = 0; i < 2; ++i)
		{
			VkCommandBufferBeginInfo m;
			m.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			m.pNext = NULL;
			m.flags = 0;
			m.pInheritanceInfo = NULL;

			if(vkBeginCommandBuffer(cachedViewSettings.swapchain.d[i], &m) != VK_SUCCESS)
			{
				disarm_view_for_vsp();
				return 0;
			}

			VkExtent2D extent;
			extent.width = ptpf_view.widthInPixels;
			extent.height = ptpf_view.heightInPixels;

			VkRenderPassBeginInfo n;
			n.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			n.pNext = NULL;
			n.renderPass = cachedViewSettings.b;
			n.framebuffer = cachedViewSettings.swapchain.c[i];
			n.renderArea.extent = extent;
			n.renderArea.offset.x = 0;
			n.renderArea.offset.y = 0;
			n.clearValueCount = 0;
			n.pClearValues = NULL;

			vkCmdBeginRenderPass(cachedViewSettings.swapchain.d[i], &n, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(cachedViewSettings.swapchain.d[i], VK_PIPELINE_BIND_POINT_GRAPHICS, cachedViewSettings.c);

			vkCmdDraw(cachedViewSettings.swapchain.d[i], 6, 1, 0, 0);

			vkCmdEndRenderPass(cachedViewSettings.swapchain.d[i]);

			if(vkEndCommandBuffer(cachedViewSettings.swapchain.d[i]) != VK_SUCCESS)
			{
				disarm_view_for_vsp(); //< is this ok..? (i.e. will command buffer be "freeable"?)
				return 0;
			}
		}

		VkSemaphoreCreateInfo h;
		h.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		h.pNext = NULL;
		h.flags = 0;

		VkSemaphore k;

		if(vkCreateSemaphore(vulkan.d, &h, NULL, &k) != VK_SUCCESS)
		{
			disarm_view_for_vsp();
			return 0;
		}

		cachedViewSettings.d = k;

		if(vkCreateSemaphore(vulkan.d, &h, NULL, &k) != VK_SUCCESS)
		{
			disarm_view_for_vsp();
			return 0;
		}

		cachedViewSettings.e = k;

		// NOTE: i.e...
		//       .. wait for semaphore to indicate "pipeline is free" AND image is available
		//       .. render to image (i.e. off-screen image)
		//       .. wait for semaphore to indicate render job is done
		//       .. present

		return 1;
	}

	void set_graphics_frame_for_vsp(void* graphicsFrame)
	{
		// copy texture to GPU
		// sample texture in fragment shader (i.e. render texture fullscreen)

		// NOTE: ticking will likely happen at least as..
		//       .. frequently as presenting, hence.. every..
		//       .. frame syncing with CPU will happen.
		//       if only one queue can be processed at a time..
		//       .. double buffering will not be effective..
		//       .. (syncronisation with CPU will happen..
		//       .. between two view updates always)
		// NOTE: double buffering would be 3 images..? (i.e...
		//       .. two render targets, one back buffer)
		//       currently.. single buffering (i.e. rendering..
		//       .. a "fullscreen quad" should be faster than..
		//       .. "ticking"..?)

		// acquire image will wait implicitly?
		uint32_t a;
		vkAcquireNextImageKHR(vulkan.d, cachedViewSettings.swapchain.a, UINT64_MAX, cachedViewSettings.d, VK_NULL_HANDLE, &a);

		// NOTE: wait until other jobs are done (i.e. stage is available?)..?
		//VkPipelineStageFlags c = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkPipelineStageFlags c = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		VkSubmitInfo b;
		b.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		b.pNext = NULL;
		b.waitSemaphoreCount = 1;
		b.pWaitSemaphores = &cachedViewSettings.d; //< i.e. after "top of pipe"
		b.pWaitDstStageMask = &c;
		b.commandBufferCount = 1;
		b.pCommandBuffers = &cachedViewSettings.swapchain.d[a];
		b.signalSemaphoreCount = 1;
		b.pSignalSemaphores = &cachedViewSettings.e;

		// NOTE: wait for "top of pipe" (i.e. for all other..
		//       .. jobs to be done..?)
		if(vkQueueSubmit(vulkan.graphicsQueue.a, 1, &b, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			return;
		}

		VkPresentInfoKHR d;
		d.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		d.pNext = NULL;
		d.waitSemaphoreCount = 1;
		d.pWaitSemaphores = &cachedViewSettings.e;
		d.swapchainCount = 1;
		d.pSwapchains = &cachedViewSettings.swapchain.a;
		d.pImageIndices = &a;
		d.pResults = NULL;

		// NOTE: wait for queue to be done before presenting
		if(vkQueuePresentKHR(vulkan.presentQueue.a, &d) != VK_SUCCESS)
		{
			return;
		}
	}
#endif

	/*
	int disarm_view_for_xlib()
	{
		//...
	}
	int arm_view_for_xlib()
	{
		//...
	}
	*/
}

//************************ forwarding to graphics APIs ************************

namespace
{
	int disarm_view()
	{
		switch(ptpf_presentation_api)
		{
		case EPTPFPresentationAPI_Vulkan:
#ifdef PTPF_USEVULKAN
			return disarm_view_for_vsp();
#else
			return -1;
#endif
		}

		return -1;
	}
	int arm_view()
	{
		switch(ptpf_presentation_api)
		{
		case EPTPFPresentationAPI_Vulkan:
#ifdef PTPF_USEVULKAN
			return arm_view_for_vsp();
#else
			return -1;
#endif
		}

		return -1;
	}

	// NOTE: graphicsFrame must be in format corresponding to..
	//       .. ptpf_view.pixelFormat
	// NOTE: graphicsFrame must be..
	//       .. ptpf_view.widthInPixels wide
	//       .. ptpf_view.heightInPixels high
	void set_graphics_frame(void* graphicsFrame)
	{
		// blend ui texture onto frame on CPU

		switch(ptpf_presentation_api)
		{
		case EPTPFPresentationAPI_Vulkan:
#ifdef PTPF_USEVULKAN
			set_graphics_frame_for_vsp(graphicsFrame);
#endif
		}
		// else //< i.e. if xlib/xcb/win32
		    // bit blit texture to window
	}
}

// NOTE: this function is temporary (i.e. set_graphics_frame can be called..
//       .. from ptpf_builtin_on_receive upon receiving..
//       .. ptpf_requestrenderedgraphicsframe_out_t message)
extern "C" void ptpf_set_graphics_frame(void* graphicsFrame)
{
	set_graphics_frame(graphicsFrame);
}

//************************* direct access to vulkan ***************************

void ptpf_builtin_on_provide_vkinstance(VkInstance a)
{
	vulkan.a = a;

	VkSurfaceKHR c;

#if defined(_WIN32)
	struct VkWin32SurfaceCreateInfoKHR b;
	b.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	b.pNext = NULL;
	b.flags = 0;
	b.hinstance = ptpf_view.window.b;
	b.hwnd = ptpf_view.window.a;

	if(vkCreateWin32SurfaceKHR(vulkan.a, &b, NULL, &c) != VK_SUCCESS)
	{
		return;
	}
#else
	struct VkXlibSurfaceCreateInfoKHR b;
	b.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	b.pNext = NULL;
	b.flags = 0;
	b.dpy = (Display*)ptpf_view.window.display;
	b.window = ptpf_view.window.a;

	if(vkCreateXlibSurfaceKHR(vulkan.a, &b, NULL, &c) != VK_SUCCESS)
	{
		return;
	}
#endif

	cachedViewSettings.surface.a = c;
}
void ptpf_builtin_on_take_vkinstance()
{
	vkDestroySurfaceKHR(vulkan.a, cachedViewSettings.surface.a, NULL);

	vulkan.a = VK_NULL_HANDLE;

	//<plugin name>_on_provide_vkinstance
}

void ptpf_builtin_on_provide_vkdevice(VkDevice b)
{
	if(cachedViewSettings.surface.a == VK_NULL_HANDLE)
	{
		return;
	}

	vulkan.d = b;

	vkGetDeviceQueue(b, vulkan.presentQueue.b, 0, &vulkan.presentQueue.a);

	if(arm_view() != 1)
	{
		return; //< view functionality not available (caught in main after vsp_load)
	}

	cachedViewSettings.bIsArmed = 1;

	//<plugin name>_on_provide_vkdevice
}
void ptpf_builtin_on_take_vkdevice()
{
	//<plugin name>_on_take_vkdevice

	disarm_view();

	cachedViewSettings.bIsArmed = 0;

	vulkan.d = VK_NULL_HANDLE;
}

//*****************************************************************************

void ptpf_builtin_on_tick_ended()
{
	//...
}

void ptpf_builtin_on_receive(struct tm_message_t* message, int a)
{
	// NOTE: for known message types, validate
	switch(message->type)
	{
	/*
	case EMessageType_Buffer:
		// assure that "size of message + size of buffer" == a
		break;
	*/
	};

	if(ptpf_on_receive != NULL)
	{
		ptpf_on_receive(message, a);
	}

	switch(message->type)
	{
		//...
	}
}

int ptpf_builtin_main(int argc, char** argv)
{
	if(ptpf_first_called_function != NULL && ptpf_first_called_function(argc, argv) == 0)
	{
		return 1;
	}

#if defined(_WIN32)
	WSADATA d;
	if(WSAStartup(MAKEWORD(2,2), &d) != 0)
	{
		fputs("failed to initialize windows sockets 2\n", stdout);
		return 1;
	}
#endif

	switch(ptpf_scriptpipeline_api)
	{
	case EPTPFScriptPipelineAPI_Vulkan:
#ifdef PTPF_USEVULKAN
		vsp_set_on_provide_vkinstance(ptpf_builtin_on_provide_vkinstance);
		vsp_set_on_take_vkinstance(ptpf_builtin_on_take_vkinstance);
		vsp_set_on_provide_vkdevice(ptpf_builtin_on_provide_vkdevice);
		vsp_set_on_take_vkdevice(ptpf_builtin_on_take_vkdevice);
		vsp_set_on_provide_graphics_queue(my_on_provide_graphics_queue);
		vsp_set_on_take_graphics_queue(my_on_take_graphics_queue);

		vsp_set_check_physical_device(my_check_physical_device);

		if(vsp_load() != 1) //< will call arm_view "if successful"
		{
			return 1;
		}
		if(cachedViewSettings.surface.a == VK_NULL_HANDLE)
		{
			fputs("error: view functionality is not availabe\n", stdout);
			return 1;
		}
		if(cachedViewSettings.bIsArmed == 0)
		{
			fputs("error: view was not armed\n", stdout);
			return 1;
		}
#endif
		break;
	}

	//if(arm_view() != 1)
	//{
	//	return 1;
	//}

	TM_SET_ON_RECEIVE(ptpf_builtin_on_receive);

	int a;
	a = ptpf_main(argc, argv);

	TM_UNSET_ON_RECEIVE();

	//disarm_view();

	switch(ptpf_scriptpipeline_api)
	{
	case EPTPFScriptPipelineAPI_Vulkan:
#ifdef PTPF_USEVULKAN
		vsp_unload(); //< will call disarm_view

		vsp_unset_check_physical_device();

		vsp_unset_on_take_graphics_queue();
		vsp_unset_on_provide_graphics_queue();
		vsp_unset_on_take_vkdevice();
		vsp_unset_on_provide_vkdevice();
		vsp_unset_on_take_vkinstance();
		vsp_unset_on_provide_vkdevice();
#endif
		break;
	}

#if defined(_WIN32)
	WSACleanup();
#endif

	if(ptpf_last_called_function != NULL)
	{
		ptpf_last_called_function();
	}

	if(a == 1)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

#if defined(PTPF_USEWINMAIN)
#if defined(_WIN32)
int WinMain(HINSTANCE a, HINSTANCE b, LPSTR c, int d)
{
	LPWSTR e = GetCommandLineW();
	int argc;
	LPWSTR* g = CommandLineToArgvW(e, &argc);

	char** argv = (char**)new char[argc * sizeof(char*)];
	for(int i = 0; i < argc; ++i)
	{
		int h = WideCharToMultiByte(CP_UTF8, 0, g[i], -1, NULL, 0, NULL, NULL);

		argv[i] = new char[h];

		WideCharToMultiByte(CP_UTF8, 0, g[i], -1, argv[i], h, NULL, NULL);
	}

	int f = ptpf_builtin_main(argc, argv);

	for(int i = 0; i < argc; ++i)
	{
		delete argv[i];
	}
	delete argv;

	return f;
}
#else
#error "winmain is only supported on windows"
#endif
#else
extern "C" int main(int argc, char** argv)
{
	return ptpf_builtin_main(argc, argv);
}
#endif
