// NOTE: this file is to be replicated on the loader side, hence it needs to comply to conventions of the C language

#ifndef PTPF_REQUEST_MESSAGE_H
#define PTPF_REQUEST_MESSAGE_H

#include "../ptpf_config.h"


// NOTE: for a request from one specific loader..
//       .. EMessageType_RequestFromOtherLoader
//       .. EPTPFRequestFromOtherLoaderInfo
//       .. ptpf_requestfromotherloader_message_t
//       .. ptpf_requestfromotherloader_message_default
//       for a request from multiple loaders..
//       .. EMessageType_RequestFromOtherLoaders
//       .. EPTPFRequestFromOtherLoadersInfo
//       .. ptpf_requestfromotherloaders_message_t
//       .. ptpf_requestfromotherloaders_message_default
enum
{
	EMessageType_RequestFromServer = 4,
	EMessageType_RequestFromOtherLoader = 5
};

#define EMessageType_RequestFromOtherLoaders EMessageType_RequestFromOtherLoader

/*
 * reserved enum indices..
 * EPTPFRequestFromServer_RenderedGraphicsFrame = 0
 */
enum /*EPTPFRequestFromServer*/
{
};
/*
 * reserved enum indices..
 */
enum /*EPTPFRequestFromOtherLoader(s)*/
{
};

enum /*EPTPFRequestFromServerInfo*/
{
	EPTPFRequestFromServerInfo_In = 0,
	EPTPFRequestFromServerInfo_Out
};
enum /*EPTPFRequestFromOtherLoader(s)Info*/
{
	EPTPFRequestFromOtherLoaderInfo_In = 0,
	EPTPFRequestFromOtherLoaderInfo_IntoOtherLoader,
	EPTPFRequestFromOtherLoaderInfo_OutFromOtherLoader,
	EPTPFRequestFromOtherLoaderInfo_Out
};

#define EPTPFRequestFromOtherLoadersInfo_In EPTPFRequestFromOtherLoaderInfo_In
#define EPTPFRequestFromOtherLoadersInfo_Out EPTPFRequestFromOtherLoaderInfo_Out

// NOTE: should be send by loaders only
struct ptpf_requestfromserver_message_t
{
	int type;
	int request; //< one of EPTPFRequestFromServer
	int requestInfo; //< one of EPTPFRequestFromServerInfo
} ptpf_request_message_default = { EMessageType_RequestFromServer };

// NOTE: should be send by loaders only
struct ptpf_requestfromotherloader_message_t
{
	int type;
	int request; //< one of EPTPFRequestFromOtherLoader(s)
	int requestInfo; //< one of EPTPFRequestFromOtherLoader(s)Info
} ptpf_requestfromotherloader_message_default = { EMessageType_RequestFromOtherLoader };

#define ptpf_requestfromotherloaders_message_t ptpf_requestfromotherloader_message_t
#define ptpf_requestfromotherloaders_message_default ptpf_requestfromotherloader_message_default

#endif
