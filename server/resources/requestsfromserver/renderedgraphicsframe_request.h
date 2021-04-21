// NOTE: this file is to be replicated on the loader side, hence it needs to comply to conventions of the C language

#ifndef PTPF_RENDEREDGRAPHICSFRAME_REQUEST_H
#define PTPF_RENDEREDGRAPHICSFRAME_REQUEST_H

#include "../request_message.h"
//#include PTPF_PATH "rendering/pixel_format.h"
#include "../../rendering/pixel_format.h"


enum
{
	EPTPFRequestFromServer_RenderedGraphicsFrame = 0
};

// NOTE: a request for a rendered graphics frame can "come from an object" or "from the CPU object" (both must be possible)
//       requesting a rendered graphics frame "from a network object" should not be allowed
// NOTE: if no view is set, a rendered graphics frame will never be sent back
struct ptpf_renderedgraphicsframe_request_in_t
{
	int type;
	int request;
	int requestInfo;
} ptpf_renderedgraphicsframe_request_in_default = { EMessageType_RequestFromServer, EPTPFRequestFromServer_RenderedGraphicsFrame, EPTPFRequestFromServerInfo_In };

struct ptpf_renderedgraphicsframe_request_out_t
{
	int type;
	int request;
	int requestInfo;
	// NOTE: if a camera is not set up (i.e. set up a camera by sending ptpf_updatecamerafortransform_attempt_try_t),..
	//       .. widthInPixels will be 0 and heightInPixels will be 0 and pixelFormat will be arbitrary (and as a..
	//       .. result a will not be present)
	// NOTE: widthInPixels, heightInPixels and pixelFormat are supplied so that..
	//       .. even if e.g. a ptpf_setpixelformat_message_t was sent while the..
	//       .. server was handling this request, the actual settings that were..
	//       .. used to render this frame (which might now be outdated) can be..
	//       .. inspected.
	int widthInPixels;
	int heightInPixels;
	int pixelFormat;
	//char a[]; //< the rendered graphics frame
} ptpf_renderedgraphicsframe_request_out_default = { EMessageType_RequestFromServer, EPTPFRequestFromServer_RenderedGraphicsFrame, EPTPFRequestFromServerInfo_Out };

#endif
