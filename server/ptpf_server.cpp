#include "ptpf_server.h"

#include "resources/contracts/graphicsrendering_contract.h"
#include "resources/attempts/updatecamera_attempt.h"

#include <cstdio>
#include <cstring>

#include <pthread.h>
// NOTE: tcp-mini only supports windows and linux, hence we only need to handle these two platforms (if tcp-mini doesn't compile,..
//       .. you can't compile ptpf-server)
#if defined(_WIN32)
#include <winsock2.h> //< output of mingw contains "Please include winsock2.h before windows.h [-Wcpp]" if put after window.h include

#ifdef PTPF_USEDIRECTX11
#define D11GFPS_DIRECT_ACCESS //< direct access to directx11 stuff
#include <directx11_graphics_for_ptpf_server.h>
#include <dxgi.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#ifdef PTPF_USEDIRECTX11
#error "directx11 is only supported on windows"
#undef PTPF_USEDIRECTX11
#endif

//#include <X11/Xlib.h>
#endif


namespace
{
  // NOTE: don't use directly (used as template for other functions of same name)
#define _TM_GET_INDEX_OF_FIRST_IN_BITFIELD(a, b) \
	int c = 0;\
	while(a != 0) \
	{ \
		b d = a & 1; \
		if(d != 0) \
		{ \
			return c; \
		} \
		++c; \
		a = a >> 1; \
	} \
	return sizeof(b) * 8; //< not in bitfield

  template<typename T>
  int get_index_of_first_in_bitfield(T a)
  {
	  _TM_GET_INDEX_OF_FIRST_IN_BITFIELD(a, T);
  }

  // a means index here
  template<typename T>
  T remove_from_bitfield(int a, T b)
  {
	  T c = 1 << a;
	  return b & ~c;
  }
}

//*****************************************************************************

extern int ptpf_graphics_api;
extern int (*ptpf_first_called_function)(int argc, char** argv);
extern int ptpf_main();
extern void (*ptpf_last_called_function)();

extern void (*ptpf_on_receive)(tm_message_t*, int);

#ifdef PTPF_ENABLE_PREVIEW_FUNCTIONALITY
extern int ptpf_enable_preview;
extern ptpf_preview_t ptpf_preview;
#endif

//*****************************************************************************

#ifdef PTPF_ENABLE_PREVIEW_FUNCTIONALITY
namespace
{
	struct cached_preview_settings_t
	{
	#ifdef PTPF_USEDIRECTX11
		IDXGISwapChain* a;
	#endif
		int camera;
		int widthInPixels;
		int heightInPixels;
		int gfps;
		struct
		{
			float minX;
			float minY;
			float maxX;
			float maxY;
		} viewport;
		int pixelFormat; //< one of EPTPFPixelFormat
	};
	cached_preview_settings_t cachedPreviewSettings;
}
#endif

#ifdef PTPF_USEDIRECTX11
	int pixel_format_to_d11gfps_pixel_format(int a, int* b)
	{
		switch(ptpf_preview.pixelFormat)
		{
		case EPTPFPixelFormat_RGBAInInt:
			*b = ED11GFPSPixelFormat_R8G8B8A8_Unsigned;
			return 1;
		case EPTPFPixelFormat_RGBAAsFourFloats:
			*b = ED11GFPSPixelFormat_R32G32B32A32_Float;
			return 1;
		default:
			return -1;
		}
	}

#ifdef PTPF_ENABLE_PREVIEW_FUNCTIONALITY
	int arm_preview_for_d11gfps()
	{
		if(ptpf_preview.window == NULL)
		{
			fputs("warning: tried to enable preview, but ptpf_preview.window is not set (stopping...)\n", stdout);
			return -1;
		}

		ID3D11Device* b;
		d11gfps_get_d3d11device(&b);

		int m;
		DXGI_FORMAT l;
		// NOTE: this should be okay because each condition is checked in order from left to right (right?)
		if(pixel_format_to_d11gfps_pixel_format(ptpf_preview.pixelFormat, &m) != 1 || d11gfps_pixel_format_to_dxgi_format(m, &l) != 1)
		{
			fputs("error: invalid pixel format set for preview window\n", stdout);
			return 0;
		}

		// create swap chain for preview, with output window set to ptpf_preview.window
		DXGI_SWAP_CHAIN_DESC c;
		c.BufferDesc.Width = ptpf_preview.widthInPixels;
		c.BufferDesc.Height = ptpf_preview.heightInPixels;
		c.BufferDesc.RefreshRate.Numerator = ptpf_preview.gfps;
		c.BufferDesc.RefreshRate.Denominator = 1;
		c.BufferDesc.Format = l;
		c.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		c.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		c.SampleDesc.Count = 1;
		c.SampleDesc.Quality = 0;
		c.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		c.BufferCount = 2; //< i.e. double buffering..?
		c.OutputWindow = (HWND)ptpf_preview.window;
		c.Windowed = TRUE;
		c.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		c.Flags = 0;

		IDXGIDevice* f;
		b->QueryInterface(__uuidof(IDXGIDevice), (void**)&f);

		IDXGIAdapter* g;
		f->GetParent(__uuidof(IDXGIAdapter), (void**)&g);

		IDXGIFactory* e;
		g->GetParent(__uuidof(IDXGIFactory), (void**)&e);

		printf("e %p\n", e);
		printf("g %p\n", g);
		printf("f %p\n", f);

		IDXGISwapChain* i;
		int j = e->CreateSwapChain(b, &c, &i) >= 0 ? 1 : 0;
		f->Release();
		g->Release();
		e->Release();
		if(j == 0)
		{
			fputs("error: failed to create swap chain for preview window\n", stdout);
			i->Release();
			return 0;
		}

		ID3D11Texture2D* h;
		i->GetBuffer(0, __uuidof(ID3D11Texture2D*), (void**)&h);

		int k;
		if(d11gfps_set_preview_camera(ptpf_preview.gfps, h, &k) != 1)
		{
			// NOTE: maybe d11gfps should not log any errors, but set error codes that we can use to log here.
			return 0;
		}

		// NOTE: not sure if this should be kept as is (maybe make viewport a value that only gets read for ptpf_update_preview_if_needed with EPTPFUpdatePreview_IfViewportHasChanged)
		if(!(ptpf_preview.viewport.minX == 0.0f && ptpf_preview.viewport.minY == 0.0f && ptpf_preview.viewport.maxX == 0.0f && ptpf_preview.viewport.maxY == 0.0f))
		{
			// check if viewport values are valid
			d11gfps_set_viewport_for_preview_camera(ptpf_preview.viewport.minX, ptpf_preview.viewport.minY, ptpf_preview.viewport.maxX, ptpf_preview.viewport.maxY);
		}

		cachedPreviewSettings.a = i; // NOTE: from now on, preview is armed
		cachedPreviewSettings.camera = k;
		cachedPreviewSettings.widthInPixels = ptpf_preview.widthInPixels;
		cachedPreviewSettings.heightInPixels = ptpf_preview.heightInPixels;
		cachedPreviewSettings.gfps = ptpf_preview.gfps; // NOTE: will this always work..? (i.e. will CreateSwapChain obey c.BufferDesc.RefreshRate.Numerator?)
		cachedPreviewSettings.viewport.minX = ptpf_preview.viewport.minX;
		cachedPreviewSettings.viewport.minY = ptpf_preview.viewport.minY;
		cachedPreviewSettings.viewport.maxX = ptpf_preview.viewport.maxX;
		cachedPreviewSettings.viewport.maxY = ptpf_preview.viewport.maxY;
		cachedPreviewSettings.pixelFormat = ptpf_preview.pixelFormat;

		return 1;
	}

	void disarm_preview_for_d11gfps()
	{
		d11gfps_remove_camera(cachedPreviewSettings.camera);

		cachedPreviewSettings.a->Release();
		cachedPreviewSettings.a = NULL;

		// NOTE: shouldn't be necessary (just make sure that whenever you arm the preview window, you set these)
		//ptpf_cached_preview_settings.gfps = 0;
		//ptpf_cached_preview_settings.widthInPixels = 0;
		//ptpf_cached_preview_settings.heightInPixels = 0;
	}

	int resize_preview_for_d11gfps(int widthInPixels, int heightInPixels)
	{
		// NOTE: all references to back buffers of swapchain must be released before calling ResizeBuffers (see https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers)

		if(cachedPreviewSettings.a->ResizeBuffers(1, cachedPreviewSettings.widthInPixels, cachedPreviewSettings.heightInPixels, DXGI_FORMAT_UNKNOWN, 0) == S_OK)
		{
			return 1;
		}

		// old back buffer should be restored here...
		return 0;
	}
#endif
#endif

//*****************************************************************************

namespace
{
	using ip_t = char[16];
	struct info_about_scout_t
	{
		ip_t ip;
		int bHasGraphicsRenderingContract;
		struct
		{
			int camera;
			//int gfps;
			int widthInPixels;
			int heightInPixels;
			int pixelFormat; //< one of EPTPFPixelFormat
			union
			{
				//PTPF_FLOAT[]
				//PTPF_INT[]
			} cachedFrame;
			// NOTE: make sure that whenever you are reading from cached frame, you lock this mutex (so that if e.g. the graphics rendering contract gets disbanded, you can gracefully stop reading before it does)
			struct
			{
				pthread_mutex_t a;
				int b;
			} bIsReadingFromCachedFrame;
			// NOTE: bShouldNoLongerReadFromCachedFrame == 1 when graphics rendering contract is about to be disbanded
			struct
			{
				pthread_mutex_t a;
				int b;
			} bShouldNoLongerReadFromCachedFrame;
		} graphicsRendering;
	};
	struct
	{
		pthread_mutex_t a;
		int b = 0;
	} numConnectedScouts;
	info_about_scout_t infoAboutScouts[TM_MAXCONNECTIONS];

#ifdef PTPF_ENABLE_PREVIEW_FUNCTIONALITY
	// NOTE: returns 0 if preview is not armed (i.e. arm_preview has not completed)
	//       returns 1 if preview is armed
	// NOTE: internally, some sort of reference to the buffer used for rendering (if GPU API..
	//       .. is EPTPFGPUAPI_DirectX11, this is the swap chain) is used to determine whether..
	//       .. the preview window is armed or not (ptpf_preview.bIsArmed is only for..
	//       .. use outside of ptpf-server
	int is_preview_armed()
	{
		switch(ptpf_graphics_api)
		{
		case EPTPFGraphicsAPI_DirectX11:
#ifdef PTPF_USEDIRECTX11
			return cachedPreviewSettings.a != NULL;
#else
			return 0;
#endif
		}

		return 0;
	}

	// NOTE: arm preview with working setup for rendering to it
	// NOTE: returns -1 if no code was executed
	//       returns 0 if something went wrong (i.e. preview window was not armed/has been disarmed)
	//       returns 1 if succeeded
	int arm_preview()
	{
		if(is_preview_armed() == 1)
		{
			fputs("warning: tried to arm preview, but is is already armed\n", stdout);
			return -1;
		}

		switch(ptpf_graphics_api)
		{
		case EPTPFGraphicsAPI_DirectX11:
#ifdef PTPF_USEDIRECTX11
			return arm_preview_for_d11gfps();
#else
			break;
#endif
		}

		return -1;
	}

	int disarm_preview()
	{
		if(is_preview_armed() == 0)
		{
			return -1;
		}

		int bWasPreviewDisarmed = 0;

		switch(ptpf_graphics_api)
		{
		case EPTPFGraphicsAPI_DirectX11:
#ifdef PTPF_USEDIRECTX11
			disarm_preview_for_d11gfps();
			bWasPreviewDisarmed = 1;
#else
			return -1;
#endif
		}

		if(bWasPreviewDisarmed == 0)
		{
			return -1;
		}

		ptpf_preview.bIsArmed = 0;
		return 1;
	}

	// NOTE: to implement this for a GPU API, make sure that..
	//       .. if resizing failed, old back buffer is kept/restored
	int resize_preview_camera(int widthInPixels, int heightInPixels)
	{
		switch(ptpf_graphics_api)
		{
		case EPTPFGraphicsAPI_DirectX11:

#ifdef PTPF_USEDIRECTX11
			return resize_preview_for_d11gfps(widthInPixels, heightInPixels);
#else
			return -1;
#endif
		}

		return -1;
	}
#endif

	int resize_non_preview_camera(int camera, int widthInPixels, int heightInPixels)
	{
		switch(ptpf_graphics_api)
		{
		case EPTPFGraphicsAPI_DirectX11:
#ifdef PTPF_USEDIRECTX11
			return d11gfps_resize_non_preview_camera(camera, widthInPixels, heightInPixels);
#else
			return -1;
#endif
		}

		return -1;
	}

	// NOTE: returns -1 if scout for b already has a graphics rendering contract
	//       returns 1 if succeeded
	int try_establish_graphicsrendering_contract(ptpf_signup_for_graphicsrendering_contract_t* a, info_about_scout_t* b)
	{
		if(b->bHasGraphicsRenderingContract == 1)
		{
			return -1;
		}

		if(pthread_mutex_init(&b->graphicsRendering.bIsReadingFromCachedFrame.a, NULL) == -1 ||
		   pthread_mutex_init(&b->graphicsRendering.bShouldNoLongerReadFromCachedFrame.a, NULL) == -1)
		{
			fputs("error: failed to create mutexes for graphics rendering\n", stdout);
			return 0;
		}

		// create camera for "scout for b" with..
		// .. a->widthInPixels
		// .. a->heightInPixels
		// .. a->gfps

		// cache the above values

		// b->bHasGraphicsRenderingContract = 1;

		return 1;
	}

	// NOTE: returns -1 if scout for b doesn't have a graphics rendering contract
	//       returns 1 otherwise
	int disband_graphicsrendering_contract(ptpf_disband_graphicsrendering_contract_t* a, info_about_scout_t* b)
	{
		if(b->bHasGraphicsRenderingContract == 0)
		{
			return -1;
		}

		// wait until cached frame is not being read from
		pthread_mutex_lock(&b->graphicsRendering.bShouldNoLongerReadFromCachedFrame.a);
		b->graphicsRendering.bShouldNoLongerReadFromCachedFrame.b = 1;
		pthread_mutex_unlock(&b->graphicsRendering.bShouldNoLongerReadFromCachedFrame.a);
		while(1)
		{
			pthread_mutex_lock(&b->graphicsRendering.bIsReadingFromCachedFrame.a);
			if(b->graphicsRendering.bIsReadingFromCachedFrame.b == 0)
			{
				break;
			}
			pthread_mutex_unlock(&b->graphicsRendering.bIsReadingFromCachedFrame.a);
		}

		// "delete cached frame"

		pthread_mutex_destroy(&b->graphicsRendering.bShouldNoLongerReadFromCachedFrame.a);

		return 1;
	}

	// NOTE: returns -1 if no code was executed (i.e. when == 0, or else if none of the new values are different..
	//       .. from current values)
	//       returns 1 if succeeded
	int update_camera_from_message_if_needed(ptpf_updatecameraattempt_try_t* a, info_about_scout_t* b)
	{
		int bWasAnythingChanged = 0;
		int when = a->when;

		while(when != 0)
		{
			int a = get_index_of_first_in_bitfield(when);

			switch(a)
			{
			case 0: //< EPTPFUpdateCamera_IfWidthOrHeightIsDifferent
				break;
			case 1: //< EPTPFUpdateCamera_IfGFPSIsDifferent
				break;
			case 2: //< EPTPFUpdateCamera_IfPixelFormatIsDifferent
				break;
			}

			when = remove_from_bitfield(a, when);
		}

		if(bWasAnythingChanged == 0)
		{
			return -1;
		}

		return 1;
	}
}

//*****************************************************************************

namespace
{
	void(*before_graphics_frame_for_preview_is_presented)();
}

extern "C" void ptpf_set_before_graphics_frame_for_preview_is_presented(void(*a)())
{
	before_graphics_frame_for_preview_is_presented = a;
}
extern "C" void ptpf_unset_before_graphics_frame_for_preview_is_presented()
{
	before_graphics_frame_for_preview_is_presented = NULL;
}

#ifdef PTPF_ENABLE_PREVIEW_FUNCTIONALITY
extern "C" int ptpf_update_preview_if_needed(int when)
{
	if(when == 0)
	{
		return -1;
	}

	int bWereChangesApplied = 0;
	while(when != 0)
	{
		int a = get_index_of_first_in_bitfield(when);
		switch(a)
		{
		case 0: //< EPTPFUpdatePreview_IfWidthOrHeightHasChanged
			break;
		case 1: //< EPTPFUpdatePreview_IfGFPSHasChanged
			break;
		case 2: //< EPTPFUpdatePreview_IfViewportHasChanged
			break;
		case 3: //< EPTPFUpdatePreview_IfPixelFormatHasChanged
			// remove preview camera
			// destroy old swap chain
			// create new swap chain
			// set preview camera to render "to back buffer" of new swap chain
			break;
		}
		when = remove_from_bitfield(a, when);
	}

	if(bWereChangesApplied == 0)
	{
		return -1;
	}
	return 1;
}
#endif

void ptpf_builtin_on_receive(tm_message_t* message, int a, char* ip)
{
	if(ptpf_on_receive != NULL)
	{
		ptpf_on_receive(message, a); //< always do this before builtin stuff so that we can check if everything is still "ok" after calling this.
	}

	pthread_mutex_lock(&numConnectedScouts.a);

	// NOTE: returns -1 if ip is not a scout (which shouldn't happen because tcp-mini should only call "on receive" for valid scouts)
	int (*which_scout)(char*) = [](char* ip)
		{
		  for(int i = 0; i < numConnectedScouts.b; ++i)
		  {
			if(strcmp(infoAboutScouts[i].ip, ip) == 0)
			{
				return i;
			}
		  }
		  return -1;
		};

	switch(message->type)
	{
	case EMessageType_Contract:
		{
			//int b = which_scout(ip);
			ptpf_contract_message_t* c = (ptpf_contract_message_t*)message;
			if(c->modifier == EPTPFContractModifier_Signup)
			{
				switch(c->contract)
				{
				case EPTPFContract_GraphicsRendering:

					break;
				}
			}
			else if(c->modifier == EPTPFContractModifier_Disband)
			{

			}

		}
		break;
	case EMessageType_Attempt:
		{
			ptpf_attempt_message_t* c = (ptpf_attempt_message_t*)message;
			switch(c->what)
			{
			case EPTPFAttempt_ToUpdateCamera:
				if(c->piece == EPTPFAttemptPiece_Try)
				{
					int b = which_scout(ip);

					if(infoAboutScouts[b].bHasGraphicsRenderingContract == 0)
					{
						break;
					}

					ptpf_updatecameraattempt_try_t* d = (ptpf_updatecameraattempt_try_t*)c;
					update_camera_from_message_if_needed(d, &infoAboutScouts[b]);
				}
				else if(c->piece == EPTPFAttemptPiece_ReportAfterTrying)
				{
					//...
				}

				break;
			}
		}
		break;
	}

	pthread_mutex_unlock(&numConnectedScouts.a);
}

void ptpf_builtin_on_connected_to_us(char* ip)
{
	// NOTE: to assure that the "unknown rendering thread" can access "numConnectedScouts" and infoAboutScouts..
	//       .. lock the mutex whenever a client connects,disconnects
	//       make tcp-mini wait until we're done with the values (i.e. the "unknown rendering thread" has..
	//       .. finished doing what it needs to do right now)
	pthread_mutex_lock(&numConnectedScouts.a);

	strcpy(infoAboutScouts[numConnectedScouts.b].ip, ip);

	++numConnectedScouts.b;

	pthread_mutex_unlock(&numConnectedScouts.a);
}

void ptpf_builtin_on_disconnected_from_us(char* ip)
{
	pthread_mutex_lock(&numConnectedScouts.a);

	for(int i = 0; i < numConnectedScouts.b; ++i)
	{
		if(strcmp(ip, infoAboutScouts[i].ip) == 0)
		{
			if(numConnectedScouts.b > 1)
			{
				int a = numConnectedScouts.b - 1; //< index of last connected scout
				memcpy(&infoAboutScouts[i], &infoAboutScouts[i + 1], a-i * sizeof(info_about_scout_t)); //< i.e. shrink to "tightest fit"
			}
			--numConnectedScouts.b;
			break;
		}
	}

	// NOTE: no checking code should be needed, as tcp-mini should only call "on disconnected from us" for valid scouts

	pthread_mutex_unlock(&numConnectedScouts.a);
}

void ptpf_builtin_on_graphics_frame_rendered(int a)
{
#ifdef PTPF_ENABLE_PREVIEW_FUNCTIONALITY
	if(ptpf_enable_preview == 1)
	{
		if(cachedPreviewSettings.camera == a)
		{
			before_graphics_frame_for_preview_is_presented();

#ifdef PTPF_USEDIRECTX11
			cachedPreviewSettings.a->Present(0, 0);
#endif
			return;
		}
	}

#endif

	// NOTE: make tcp-mini wait for us to finish copying the frame that has just been rendered into cache..
	//       .. even if the client is in the process of disconnecting (the graphics frame won't be sent..
	//       .. here anyway, i.e. it will only get copied into "a cache" where the main thread can send..
	//       .. it as soon as it figures out that a rendered frame is waiting)
	pthread_mutex_lock(&numConnectedScouts.a);

	for(int i = 0; i < numConnectedScouts.b; ++i)
	{
		if(infoAboutScouts[i].bHasGraphicsRenderingContract && infoAboutScouts[i].graphicsRendering.camera == a)
		{
			// copy frame into infoAboutScouts[i].graphicsRendering.cachedFrame.b
			// notify main thread that frame is ready to be shipped
		}
	}

	pthread_mutex_unlock(&numConnectedScouts.a);
}

void start_render_thread()
{
	switch(ptpf_graphics_api)
	{
	case EPTPFGraphicsAPI_DirectX11:
#ifdef PTPF_USEDIRECTX11
		d11gfps_start_render_thread();
#endif
		break;
	}
}
void stop_render_thread()
{
	switch(ptpf_graphics_api)
	{
	case EPTPFGraphicsAPI_DirectX11:
#ifdef PTPF_USEDIRECTX11
		d11gfps_stop_render_thread();
#endif
		break;
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

	TM_SET_ON_RECEIVE(ptpf_builtin_on_receive);

	switch(ptpf_graphics_api)
	{
	case EPTPFGraphicsAPI_DirectX11:
#ifdef PTPF_USEDIRECTX11
		if(d11gfps_load() != 1)
		{
			fputs("error: failed to load \"directx11 graphics\"\n", stdout);

			return 1;
		}
		d11gfps_set_on_graphics_frame_rendered(ptpf_builtin_on_graphics_frame_rendered);
		break;
#else
		fputs("error: selected directx11 is GPU API, but ptpf_server has not been compiled with PTPF_USEDIRECTX11\n", stdout);
		return 1;
#endif
	default:
		fputs("error: GPU API selection is not recognised\n", stdout);
		return 1;
	}

	if(ptpf_enable_preview == 1)
	{
		arm_preview();
	}

	start_render_thread();

	int a;
	a = ptpf_main();

	stop_render_thread();

	if(ptpf_enable_preview == 1)
	{
		disarm_preview();
	}

	switch(ptpf_graphics_api)
	{
	case EPTPFGraphicsAPI_DirectX11:
#ifdef PTPF_USEDIRECTX11
		d11gfps_unset_on_graphics_frame_rendered();
		d11gfps_unload();
#endif
		break;
	}

	TM_UNSET_ON_RECEIVE();

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
int main(int argc, char** argv)
{
	return ptpf_builtin_main(argc, argv);
}
#endif
