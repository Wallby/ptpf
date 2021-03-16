// NOTE: this file is to be replicated on the loader side, hence it needs to comply to conventions of the C language

#ifndef PTPF_ATTEMPT_MESSAGE_H
#define PTPF_ATTEMPT_MESSAGE_H


enum
{
	EMessageType_Attempt = 3
};

/*
 * reserved enum indices..
 * EPTPFAttempt_ToUpdateCamera = 0
 */
enum EPTPFAttempt
{
};

enum
{
	EPTPFAttemptPiece_Try = 0,
	EPTPFAttemptPiece_ReportAfterTrying
};

struct ptpf_attempt_message_t
{
	int type;
	int what; //< one of EPTPFAttempt
	int piece; //< one of EPTPFAttemptPiece
} ptpf_attempt_message_default = { EMessageType_Attempt };

#endif
