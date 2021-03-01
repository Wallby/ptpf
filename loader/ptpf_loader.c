#include "ptpf_loader.h"

#if defined(_WIN32)
#include <winsock2.h>
#endif

#include <stdio.h>

extern int ptpf_main(int, char**);

int main(int argc, char** argv)
{
#if defined(_WIN32)
	WSADATA d;
	if(WSAStartup(MAKEWORD(2,2), &d) != 0)
	{
		fputs("failed to initialize windows sockets 2\n", stdout);
		return 1;
	}
#endif

	int a;
	a = ptpf_main(argc, argv);

#if defined(_WIN32)
	WSACleanup();
#endif

	if(a == 1)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
