// NOTE: this file is to be replicated on the loader side, hence it needs to comply to conventions of the C language

#ifndef PTPF_BUFFER_INTFORMAT_H
#define PTPF_BUFFER_INTFORMAT_H

#include "../buffer_message.h"
//#include PTPF_PATH "units/int.h"
#include "../../units/int.h"


enum
{
  EPTPFBufferFormat_Ints = 1
};

struct ptpf_intbuffer_t
{
  int type;
  int format;
  int size;
  //PTPF_INT buffer[];
} ptpf_intbuffer_default = {EMessageType_Buffer, EPTPFBufferFormat_Ints, 0};

#endif
