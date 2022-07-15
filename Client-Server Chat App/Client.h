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
	enum ClientState { 
		NONE, CONNECT_TO_SERVER, REGISTER_USERNAME, QUERY_ACTION, WAIT_FOR_MESSAGES, GETTING_CLIENT_LIST, SENDING_MESSAGE, GETTING_LOGS
	};

	ClientState gClientState = NONE;

	bool gIsRegisteredWithServer = false;
	bool gIsConnectedToServer = false;

	CSCA::SocketInfo gSocketInfo;
	SOCKET gComSocket;

	fd_set gMasterSet;
	fd_set gReadReadySet;
	fd_set gSendReadySet;

	timeval gTimeout = {
		0,	// Seconds
		50000	// Microseconds
	};

	CSCA::ClientConnection gServerMessage;
	bool gSentSize = false;
	CSCA::SocketInfo QueryTCPSocketInfo();
	std::string gClientMessages;

	void DisplayOptions(bool bSetState = true);
	void ConnectToServer();
	bool WithinRange(int choice);
public:
	void Run();

	void RegisterClient(std::string username);

	void SendMessage(std::string message);

	bool SendClientMessage(SOCKET socket);

	bool ReceiveServerMessage(SOCKET socket);

	bool HandleServerMessage();

	void Exit(bool sendExitMessage = true);

	void LeaveServer();

	void QueryUserAction();

	std::string QueryClientMessage();

	std::string QueryUsername();


};