#pragma once
#define NOMINMAX
#include "Communcation.h"
#include <iostream>
#include <string>
#define _CRT_SECURE_NO_WARNINGS                 // turns of deprecated warnings
#define _WINSOCK_DEPRECATED_NO_WARNINGS         // turns of deprecated warnings for winsock
#include <winsock2.h>
//#include <ws2tcpip.h>                         // only need if you use inet_pton
#pragma comment(lib,"Ws2_32.lib")

class ClientApp
{
	enum ClientState { NONE, QUERY_ACTION};

	ClientState gClientState = NONE;

	CSCA::SocketInfo gSocketInfo;
	SOCKET gComSocket;

	fd_set gMasterSet;
	fd_set gReadySet;

	CSCA::ClientConnection gServerMessage;

	timeval gTimeout = {
	0,	// Seconds
	50000	// Microseconds
	};

	CSCA::SocketInfo QueryTCPSocketInfo();
public:
	void Run();

	void RegisterClient(std::string username);

	void SendMessage(std::string message);

	void ReceiveServerMessage(SOCKET socket);

	bool HandleServerMessage();

	void Exit(bool sendExitMessage = true);

	void LeaveServer();

	void QueryUserAction();

	std::string QueryClientMessage();

	std::string QueryUsername();


};