// NOTE: this file is replicated from the server side

#ifndef PTPF_UPDATECAMERA_ATTEMPT_H
#define PTPF_UPDATECAMERA_ATTEMPT_H

#include "../attempt_message.h"
//#include PTPF_PATH "rendering/pixel_format.h"
#include "../../rendering/pixel_format.h"


enum
{
	EPTPFAttempt_ToUpdateCamera = 0
};

enum
{
	EPTPFUpdateCamera_IfWidthOrHeightIsDifferent = 1,
	EPTPFUpdateCamera_IfGFPSIsDifferent = 2,
	EPTPFUpdateCamera_IfPixelFormatIsDifferent = 4
};

struct ptpf_updatecameraattempt_try_t
{
	int type;
	int what;
	int piece;
	int widthInPixels;
	int heightInPixels;
	int gfps;
	int pixelFormat;
	int when; //< one or more if EPTPFUpdateCamera "orred" together
} ptpf_updatecameraattempt_try_default = { EMessageType_Attempt, EPTPFAttempt_ToUpdateCamera, EPTPFAttemptPiece_Try };

struct ptpf_updatecameraattempt_report_t
{
	int type;
	int what;
	int piece;
	int flagsThatCausedFailure; //< any of all bits that were set in when member of try piece
} ptpf_updatecameraattempt_report_default = { EMessageType_Attempt, EPTPFAttempt_ToUpdateCamera, EPTPFAttemptPiece_ReportAfterTrying };

#endif
