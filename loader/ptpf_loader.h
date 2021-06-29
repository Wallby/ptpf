#ifndef PTPF_LOADER_H
#define PTPF_LOADER_H

#ifdef __cplusplus
#define PTPF_FUNCTION extern "C"
#else
#define PTPF_FUNCTION
#endif

#define TCP_MINI_SCOUT
#include <tcp_mini.h>

//#include PTPF_PATH "rendering/pixel_format.h"
#include "rendering/pixel_format.h"


// ******************* 1. values that apply at compile time *******************

// NOTE: comment out APIs that you don't want to be available
#define PTPF_USEVULKAN

#if defined(_WIN32)
#define PTPF_USEWINMAIN //< i.e. use WinMain as entry point
#endif

// ******************** 2. values that apply at runtime ***********************

// "fputs format" used by ptpf
// (error:|warning:) <description>

enum
{
	EPTPFScriptPipelineAPI_None = 0 //< e.g. for sending messages manually
	, EPTPFScriptPipelineAPI_Vulkan = 1
//#if defined(_WIN32)
//	, EPTPFScriptPipelineAPI_DirectX11 = 2
//#endif
};

enum
{
	// NOTE: EPTPFPresentationAPI_Vulkan is only available if ptpf_scriptpipeline_api is EPTPFScriptPipelineAPI_Vulkann
	EPTPFPresentationAPI_Vulkan = 0
//#if defined(_WIN32)
//	// NOTE: EPTPFPresentationAPI_DirectX11 is only available if ptpf_scriptpipeline_api is EPTPFScriptPipelineAPI_DirectX11
//	, EPTPFPresentationAPI_DirectX11
//	, EPTPFPresentationAPI_Win32
//#else
//	, EPTPFPresentationAPI_Xlib
//	, EPTPFPresentationAPI_XCB
//#endif
};

//extern char* ptpf_application_name;
//extern int ptpf_scriptpipeline_api;
//extern int ptpf_presentation_api;
//extern int ptpf_which_scriptvariables;
//extern int ptpf_which_scriptvariablevariants;
// NOTE: returns 0 if "something went wrong"
//       returns 1 if all is ok
//extern int (*ptpf_first_called_function)(int argc, char** argv);
// NOTE: returns 0 if "something went wrong"
//       returns 1 if all is ok
//extern "C" int ptpf_main(int argc, char** argv);
//extern void (*ptpf_last_called_function)();

//extern void (*ptpf_on_receive)(tm_message_t* message, int a);

struct ptpf_view_t
{
	struct
	{
#if defined(_WIN32)
#ifdef PTPF_USEVULKAN
		// NOTE: "vulkan for windows" requires HINSTANCE
		void* b; //< HINSTANCE (i.e. void*/struct { int }*)
#endif
		// NOTE: on windows "window" will be a HWND (i.e. void*/struct { int }*)
		void* a; //< HWND
#else
		// NOTE: on linux w. Xlib a Display* is required..?
		void* display;
		//// NOTE: on linux w. Xlib "window" will be a Display (i.e. unsigned long)
		unsigned long a;
#endif
	} window;
	int widthInPixels;
	int heightInPixels;
	// NOTE: if gfps == -1, render ui + combine with (cached) graphics frame +..
	//       .. present after each ptpf_requestrenderedgraphicsframe_out_t
	//       if gfps == 0, will never present (i.e. disable presentation)
	//       otherwise.. will render ui + combine with cached graphics frame +..
	//       .. present gfps times per second
	// NOTE: actual gfps is min(gfps, <refresh rate of monitor (with highest..
	//       .. refresh rate)>)
	//int gfps;
	// NOTE: if you want to render UI, you may want the view to not take up..
	//       .. the entire viewport
	struct
	{
		float minX;
		float minY;
		float maxX;
		float maxY;
	} viewport;
	int pixelFormat; //< one of EPTPFPixelFormat
};

//extern struct ptpf_view_t ptpf_view;

//enum
//{
//	EPTPFUpdateView_IfWidthOrHeightHasChanged = 1,
//	//EPTPFUpdateView_IfGFPSHasChanged = 2,
//	EPTPFUpdateView_IfViewportHasChanged = 2,
//	EPTPFUpdateView_IfPixelFormatHasChanged = 4
//};

//PTPF_FUNCTION int ptpf_update_view_if_needed(int when);

PTPF_FUNCTION void ptpf_set_graphics_frame(void* graphicsFrame); //< tmp

#endif
