// NOTE: this file is replicated from the server side

#ifndef PTPF_BUFFER_FLOATFORMAT_H
#define PTPF_BUFFER_FLOATFORMAT_H

#include "../buffer_message.h"
//#include PTPF_PATH "units/float.h"
#include "../../units/float.h"


enum
{
	EPTPFBufferFormat_Floats = 2
};

struct ptpf_floatbuffer_t
{
	int type;
	int format;
	int size;
	//PTPF_FLOAT buffer[];
} ptpf_floatbuffer_default = {EMessageType_Buffer, EPTPFBufferFormat_Floats, 0};

#endif
