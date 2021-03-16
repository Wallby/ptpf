#ifndef PTPF_SERVER_H
#define PTPF_SERVER_H

#ifdef __cplusplus
#define PTPF_FUNCTION extern "C"
#else
#define PTPF_FUNCTION
#endif

#define TCP_MINI_MATCH
#include <tcp_mini.h>


// ***************** 1. values that apply at compile time ******************

// NOTE: comment out GPU API's that you don't want to be available
#if defined(_WIN32)
#define PTPF_USEDIRECTX11
#endif

#if defined(_WIN32)
#define PTPF_USEWINMAIN //< i.e. use WinMain as entry point
#endif

#define PTPF_ENABLE_PREVIEW_FUNCTIONALITY

// ***************** 2. values that apply at runtime ***********************

// "fputs format" used by ptpf
// (error:|warning:) <description>

enum
{
	EPTPFGraphicsAPI_None = 0,
	EPTPFGraphicsAPI_DirectX11 = 1
};

//extern int ptpf_graphics_api;
// NOTE: returns 0 if "something went wrong"
//       returns 1 if all is ok
//extern int (*ptpf_first_called_function)(int argc, char** argv);
// NOTE: returns 0 if "something went wrong"
//       returns 1 if all is ok
//extern int ptpf_main(int argc, char** argv);
//extern void (*ptpf_last_called_function)();

//extern void (*ptpf_on_receive)(tm_message_t* message, int a);

#ifdef PTPF_USEDIRECTX11
#ifdef PTPF_DIRECT_ACCESS_TO_DIRECTX11
#include <d3d11.h>
// NOTE: will return -1 if no code was executed (and thus a was not written to)
// NOTE: will always return -1 if ptpf_gpu_api != EPTPFGPUAPI_DirectX11
// NOTE: an example usecase for this, may be to do another render pass in..
//       .. on_frame_for_preview_window_rendered (e.g. to render UI)
PTPF_FUNCTION int ptpf_get_d3d11device(ID3D11Device** a);
#endif
#endif

#ifdef PTPF_ENABLE_PREVIEW_FUNCTIONALITY
//#include PTPF_PATH "rendering/pixel_format.h"
#include "rendering/pixel_format.h"


// NOTE: only opportunity to enable preview is the ..
//       .. first_called_function.
//extern int ptpf_enable_preview; //< set to 1 if rendering a preview

struct ptpf_preview_t
{
	// NOTE: on linux w. Xlib *window will be a Display
	// NOTE: on windows *window will be a HWND
	void* window;
	int widthInPixels;
	int heightInPixels;
	int gfps;
	// NOTE: if you want to render UI, you may want the preview to not take up the entire viewport
	struct
	{
		float minX;
		float minY;
		float maxX;
		float maxY;
	} viewport;
	int pixelFormat; //< one of EPTPFPixelFormat
	// NOTE: when preview is successfully armed with setup for rendering to it, bIsArmed will be set to 1
	int bIsArmed; /* = 0;*/
};

// NOTE: before_graphics_frame_for_preview_is_presented will be called from an unknown thread (i.e. make sure that if you use any variables of your own, there are mutexes for them)
//void (*before_graphics_frame_for_preview_is_presented)();
PTPF_FUNCTION void ptpf_set_before_graphics_frame_for_preview_is_presented(void(*a)());
PTPF_FUNCTION void ptpf_unset_before_graphics_frame_for_preview_is_presented();

enum /*EPTPFUpdatePreview*/
{
	EPTPFUpdatePreview_IfWidthOrHeightHasChanged = 1,
	EPTPFUpdatePreview_IfGFPSHasChanged = 2,
	EPTPFUpdatePreview_IfViewportHasChanged = 4,
	EPTPFUpdatePreview_IfPixelFormatHasChanged = 8
};

// NOTE: when -> one or more of EPTPFUpdatePreviewWindow "orred" together
// NOTE: returns 0 if something went wrong while applying the new values
//       returns 1 if succeeded
//       returns -1 of no code was executed (i.e. of when == 0, or when ..
//       .. corresponding members of ptpf_preview_window are not different ..
//       .. from old values
// NOTE: if something went wrong while applying new values, old values ..
//       .. will be kept and written back to corresponding members of ..
//       .. ptpf_preview_window
PTPF_FUNCTION int ptpf_update_preview_if_needed(int when);
#endif

#endif
