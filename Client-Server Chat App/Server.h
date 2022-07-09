#pragma once
#define _CRT_SECURE_NO_WARNINGS                 // turns of deprecated warnings
#define _WINSOCK_DEPRECATED_NO_WARNINGS         // turns of deprecated warnings for winsock
#include <winsock2.h>
//#include <ws2tcpip.h>                         // only need if you use inet_pton
#pragma comment(lib,"Ws2_32.lib")
#include <map>
#include <string>
#include "Communcation.h"

class ServerApp
{
	const uint8_t MaxServerCapacity = 10;

	SOCKET gListenSocket;
	fd_set gMasterSet;
	fd_set gReadySet;
	timeval gTimeout = {
		1,	// Seconds
		0	// Microseconds
	};

	std::map<SOCKET, CSCA::ClientConnection*> gConnectedUsers;
	std::map<std::string, SOCKET> gUsernames;

	/** Displays '.' update to console upon select timeout. */
	void ShowListeningStatus();

	/** Adds new client socket to set. */
	void AcceptNewClient();

	void ReceiveClientMessage(SOCKET socket);

	bool HandleClientMessage(SOCKET socket, 
		CSCA::ClientConnection* clientConnection);

	bool HandleRegisterClient(SOCKET socket,
		CSCA::ClientConnection* clientConnection,
		std::string message);

	void RegisterClient(SOCKET socket, std::string username);

	void RemoveClient(SOCKET clientSocket);

	void LogEvent(std::string eventStr);

	void SendClientMessage(SOCKET socket, std::string message);

public:
	void Run();
};