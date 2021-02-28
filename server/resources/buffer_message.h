// NOTE: this file is to be replicated on the loader side, hence it needs to comply to conventions of the C language

#ifndef PTPF_BUFFERMESSAGE_H
#define PTPF_BUFFERMESSAGE_H

#include "../ptpf_config.h"

#include <tcp_mini.h>

enum
{
	EMessageType_Buffer = 1
};

/*
 * reserved enum indices..
 * EPTPFBufferFormat_None = 0
 * EPTPFBufferFormat_Ints = 1
 * EPTPFBufferFormat_Floats = 2
 * EPTPFBufferFormat_Chars = 3
 */
enum EPTPFBufferFormat
{
	EPTPFBufferFormat_None = 0 //< for yet uninterpreted buffers
};

struct ptpf_buffer_message_t //< .._message_t may not be sent, but can be used for interpretation
{
	int type;
	int format;
} ptpf_buffer_message_default = {EMessageType_Buffer};

struct ptpf_unformattedbuffer_t //< .._t must have a size as the last argument, which described the size of the "buffer"
{
	int type;
	int format;
	int size;
} ptpf_unformattedbuffer_default = {EMessageType_Buffer, EPTPFBufferFormat_None};

#endif

