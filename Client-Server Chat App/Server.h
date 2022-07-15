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
private:
	bool gShowListeningStatus = true;

	const uint8_t MaxServerCapacity = 10;

	SOCKET gListenSocket;
	fd_set gMasterSet;
	fd_set gReadReadySet;
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

	bool HandleGetUserList(SOCKET socket,
		CSCA::ClientConnection* clientConnection);

	void RemoveClient(SOCKET clientSocket, 
		CSCA::ClientConnection* clientConnection, bool logRemoval = true);

	void LogEvent(std::string eventStr);

	void LogRegister(CSCA::ClientConnection* clientConnection);

	void LogDisconnect(CSCA::ClientConnection* clientConnection);

	void LogExit(CSCA::ClientConnection* clientConnection);

	bool SendToAllClients(std::string message, std::string username);

	void SendClientMessage(SOCKET socket, 
		CSCA::ClientConnection* clientConnection, std::string message);

public:
	void Run();
};