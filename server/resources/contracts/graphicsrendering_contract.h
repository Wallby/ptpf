// NOTE: this file is to be replicated on the loader side, hence it needs to comply to conventions of the C language

#ifndef PTPF_GRAPHICSRENDERING_CONTRACT_H
#define PTPF_GRAPHICSRENDERING_CONTRACT_H

#include "../contract_message.h"
//#include PTPF_PATH "rendering/pixel_format.h"
#include "../../rendering/pixel_format.h"


enum
{
	EPTPFContract_GraphicsRendering = 0
};

struct ptpf_signup_for_graphicsrendering_contract_t
{
	int type;
	int contract;
	int modifier;
	int gfps;
	int widthInPixels;
	int heightInPixels;
	int pixelFormat;
} ptpf_signup_for_graphicsrendering_contract_default = { EMessageType_Contract, EPTPFContract_GraphicsRendering, EPTPFContractModifier_Signup };

struct ptpf_disband_graphicsrendering_contract_t
{
	int type;
	int contract;
	int modifier;
} ptpf_disband_graphicsrendering_contract_default = { EMessageType_Contract, EPTPFContract_GraphicsRendering, EPTPFContractModifier_Disband };

#endif
