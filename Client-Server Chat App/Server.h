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
	std::string LOG_FILENAME = "serverlog.log";
	bool gShowListeningStatus = true;

	int MaxServerCapacity = 10;

	SOCKET gListenSocket;
	fd_set gMasterSet;
	fd_set gReadReadySet;
	fd_set gSendReadySet;
	timeval gTimeout = {
		0,	// Seconds
		500	// Microseconds
	};

	std::map<SOCKET, CSCA::ClientConnection*> gConnectedUsers;
	std::map<std::string, SOCKET> gUsernames;

	double GetDeltaTime();

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

	bool HandleGetLog(SOCKET socket,
		CSCA::ClientConnection* clientConnection);

	void RemoveClient(SOCKET clientSocket, 
		CSCA::ClientConnection* clientConnection, bool logRemoval = true);

	void LogEvent(std::string eventStr);

	void LogRegister(CSCA::ClientConnection* clientConnection);

	void LogDisconnect(CSCA::ClientConnection* clientConnection);

	void LogExit(CSCA::ClientConnection* clientConnection);

	bool SendToAllClients(std::string message, std::string username);

	void SendBytesToClient(SOCKET socket,
		CSCA::ClientConnection* clientConnection);

	void SendClientMessage(SOCKET socket, 
		CSCA::ClientConnection* clientConnection, std::string message);

public:
	void Run();
};