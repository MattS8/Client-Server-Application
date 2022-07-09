#pragma once
#include <iostream>

namespace CSCA
{
	struct SocketInfo
	{
		unsigned short ipAddr[4];
		unsigned short port;
		std::string ipAddrStr;
	};

	struct ClientConnection
	{
		std::string clientUsername;
		char* messageBuffer;
		uint8_t messageSize;
		uint8_t bytesRead;
	};
}
