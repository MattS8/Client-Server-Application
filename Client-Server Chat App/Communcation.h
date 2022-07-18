#pragma once
#include <iostream>

namespace CSCA
{
	const std::string SV_REGISTER_USERNAME = "$register";
	const std::string SV_GET_CLIENT_LIST = "$getlist";
	const std::string SV_USERNAME_TAKEN = "SV_USERNAME_TAKEN";
	const std::string SV_REGISTER_SUCCESS = "SV_REGISTER_SUCCESS";
	const std::string SV_CLIENT_MESSAGE = "SV_CLIENT_MESSAGE";
	const std::string SV_FULL = "SV_FULL";
	const std::string SV_CLIENT_LIST = "SV_CLIENT_LIST";
	const std::string SV_GET_LOGS = "$getlog";
	const std::string SV_GET_LOGS_END = "$getlog END\n";
	const std::string SV_EXIT = "SV_EXIT";

	struct SocketInfo
	{
		unsigned short ipAddr[4];
		unsigned short port;
		std::string ipAddrStr;
	};

	struct ClientConnection
	{
		std::string clientUsername;
		char* readMessageBuffer;
		uint8_t readBuffSize;
		uint8_t readBytesProcessed;

		char* sendMessageBuffer;
		uint8_t sendBuffSize;
		uint8_t sendBytesProcessed;
		bool sendSizeSent;
		bool clientLeaving;
	};
}
