#define NOMINMAX
#include "Server.h"
#include <iostream>

// Text Art Generated at https://patorjk.com/

void ServerApp::Run()
{
	std::cout
		<< "____ ____ ____ _  _ ____ ____    _  _ ____ ___  ____   \n"
		<< "[__  |___ |__/ |  | |___ |__/    |\\/| |  | |  \\ |___ . \n"
		<< "___] |___ |  \\  \\/  |___ |  \\    |  | |__| |__/ |___ . \n"
		<< "                                                       \n";

	// Create socket
	gListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (gListenSocket == INVALID_SOCKET)
	{
		std::cerr << ">> ERROR: Unable to create communication socket!\n";
		return;
	}

	// Bind socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(31337);

	int result = bind(gListenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (result == SOCKET_ERROR)
	{
		std::cerr << ">> ERROR: Failed to bind server socket!\n";
		return;
	}

	//Listen
	result = listen(gListenSocket, 1);
	if (result == SOCKET_ERROR)
	{
		std::cerr << ">> ERROR: Unable to listen on server socket!\n";
		return;
	}

	// Prepare sets
	FD_ZERO(&gMasterSet);
	FD_ZERO(&gReadySet);
	FD_SET(gListenSocket, &gMasterSet);

	ShowListeningStatus();

	// Select statement
	do 
	{
		gReadySet = gMasterSet;
		int rc = select(0, &gReadySet, NULL, NULL, &gTimeout);

		// Timeout
		if (rc == 0)
		{
			static int dotCount = 0;
			if (dotCount >= 30)
			{
				std::cout << "\n\t\t\t\t .";
				dotCount = 0;
			}
			else
			{
				std::cout << ".";
			}
			++dotCount;
			continue;
		}

		// Check listen socket
		if (FD_ISSET(gListenSocket, &gReadySet))
		{
			AcceptNewClient();
		}

		// Check client communication

		for (int i = 0; i < gReadySet.fd_count; i++)
		{
			if (gReadySet.fd_array[i] == gListenSocket)
				continue;
			ReceiveClientMessage(gReadySet.fd_array[i]);
		}
	} while (true);

}

void ServerApp::ReceiveClientMessage(SOCKET socket)
{
	auto itConnection = gConnectedUsers.find(socket);

	if (itConnection == gConnectedUsers.end())
	{
		std::cerr << ">> ERROR: Attempted to receive connection on socket "
			<< socket << " but no ClientConnection was found!\n";
		return;
	}

	CSCA::ClientConnection* connection = itConnection->second;

	int result;
	if (connection->messageSize == 0)
	{
		// Read in Size
		char msgLength;
		result = recv(socket, &msgLength, 1, 0);
		if (result < 1)
		{
			std::cerr << ">> ERROR: Unable to read message length for socket "
				<< socket << "!\n";
			RemoveClient(socket);
			return;
		}

		connection->messageSize = (uint8_t)msgLength;

		if (connection->messageSize < 1)
		{
			std::cerr << ">> ERROR: Invalid message size read in on socket "
				<< socket << " (" << connection->messageSize << ")!\n";
			RemoveClient(socket);
			return;
		}
		connection->messageBuffer = new char[connection->messageSize];
		connection->bytesRead = 0;
	}
	else
	{
		// Read in messages
		result = recv(socket, connection->messageBuffer + connection->bytesRead,
			connection->messageSize - connection->bytesRead, 0);
	
		if (result == 0)
		{
			RemoveClient(socket);
			return;
		}
		if (result == SOCKET_ERROR)
		{
			std::cerr << ">> ERROR: Unable to read client message on socket "
				<< socket << " (SOCKET_ERROR)!\n";
			RemoveClient(socket);
			return;
		}

		connection->bytesRead + result;

		if (connection->bytesRead >= connection->messageSize)
		{
			// Handle full message received
			if (HandleClientMessage(socket, connection))
			{
				delete[] connection->messageBuffer;
				connection->messageBuffer = nullptr;
				connection->messageSize = 0;
			}
		}
	}
}

bool ServerApp::HandleRegisterClient(SOCKET socket, 
	CSCA::ClientConnection* clientConnection,
	std::string message)
{
	// Handle SV_FULL
	if (gConnectedUsers.size() >= MaxServerCapacity)
	{
		SendClientMessage(socket, "SV_FULL");
		LogEvent("$register - Refused client due to max server capacity.");
		RemoveClient(socket);
		return false;
	}

	size_t startOfUsername = message.find(" ");
	if (startOfUsername == std::string::npos)
	{
		std::cerr << ">> ERROR: Register event received however "
			<< "no username was accompanying it!\n";
		LogEvent("$register - No username associated with register event.");
		RemoveClient(socket);
		return false;
	}

	std::string username = message.substr(startOfUsername + 1);
	if (gUsernames.find(username) != gUsernames.end())
	{
		std::cerr << ">> ERROR: Client tried to register with a username that "
			<< "has already been claimed!\n";
		LogEvent("$register - Duplicate username registration.");
		RemoveClient(socket);
		return false;
	}
	
	clientConnection->clientUsername = username;
	gUsernames.insert(std::pair<std::string, SOCKET>(username, socket));
	LogEvent("$register - User registered.");
	SendClientMessage(socket, "SV_SUCCESS");

	return true;
}

bool ServerApp::HandleClientMessage(SOCKET socket, CSCA::ClientConnection* clientConnection)
{
	std::string message(clientConnection->messageBuffer);
	// Handle Commands
	if (message[0] == '$')
	{
		std::string command = message.find(" ") == std::string::npos
			? message.substr(0, message.find(" "))
			: message;
		if (command.compare("$register") == 0)
		{
			return HandleRegisterClient(socket, clientConnection, message);
		}
		else if (command.compare("$getlist") == 0)
		{

		}
		else if (command.compare("$getlog") == 0)
		{

		}
		else if (command.compare("$exit") == 0)
		{
			RemoveClient(socket);
			return false;
		}
		else
		{
			std::cerr << ">> ERROR: Unknown client command received: "
				<< command << "!\n";
			return;
		}
	}
	else
	{
		// Handle Normal Message
	}
}

void ServerApp::AcceptNewClient()
{
	// Listen for clients
	SOCKET commSocket = accept(gListenSocket, NULL, NULL);
	if (commSocket == INVALID_SOCKET)
	{
		std::cerr << ">> ERROR: Failed to accept on listenSocket!\n";
		return;
	}
	else
	{
		std::cout << "> Received new connection!\n";
		FD_SET(commSocket, &gMasterSet);
		if (gConnectedUsers.find(commSocket) == gConnectedUsers.end())
		{
			CSCA::ClientConnection* newClientConnection = new CSCA::ClientConnection;
			newClientConnection->bytesRead = 0;
			newClientConnection->messageBuffer = nullptr;
			newClientConnection->messageSize = 0;
			newClientConnection->clientUsername = "";
			gConnectedUsers.insert(std::pair<SOCKET, CSCA::ClientConnection*>(
				commSocket,
				newClientConnection));
		}
		else
		{
			std::cerr << ">> ERROR: Connection on socket " << commSocket
				<< " was already established!\n";
		}
	}
}

void ServerApp::ShowListeningStatus()
{
	std::cout << "\tListening for actions... ";
}