// NOTE: this file is replicated from the server side

#ifndef PTPF_CONTRACT_MESSAGE_H
#define PTPF_CONTRACT_MESSAGE_H

#include "../ptpf_config.h"


enum
{
	EMessageType_Contract = 2
};

/* reserved enum indices..
 * EPTPFContract_GraphicsRendering = 0
 */
enum EPTPFContract
{
};

enum
{
	EPTPFContractModifier_Signup = 0,
	EPTPFContractModifier_Disband
};

struct ptpf_contract_message_t
{
	int type;
	int contract; //< one of EPTPFContract
	int modifier; //< one of EPTPFContractModifier
};

#endif
