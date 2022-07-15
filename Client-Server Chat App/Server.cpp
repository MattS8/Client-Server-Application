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
		std::cerr << "\n>> ERROR: Unable to create communication socket!\n";
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
		std::cerr << "\n>> ERROR: Failed to bind server socket!\n";
		return;
	}

	//Listen
	result = listen(gListenSocket, 1);
	if (result == SOCKET_ERROR)
	{
		std::cerr << "\n>> ERROR: Unable to listen on server socket!\n";
		return;
	}

	// Prepare sets
	FD_ZERO(&gMasterSet);
	FD_ZERO(&gReadReadySet);
	FD_SET(gListenSocket, &gMasterSet);

	// Select statement
	do 
	{
		gReadReadySet = gMasterSet;
		int rc = select(0, &gReadReadySet, NULL, NULL, &gTimeout);

		// Timeout
		if (rc == 0)
		{
			if (gShowListeningStatus)
				ShowListeningStatus();
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
		if (FD_ISSET(gListenSocket, &gReadReadySet))
		{
			AcceptNewClient();
		}

		// Check client communication

		for (int i = 0; i < gReadReadySet.fd_count; i++)
		{
			if (gReadReadySet.fd_array[i] == gListenSocket)
				continue;
			ReceiveClientMessage(gReadReadySet.fd_array[i]);
		}
	} while (true);

}

void ServerApp::ReceiveClientMessage(SOCKET socket)
{
	auto itConnection = gConnectedUsers.find(socket);

	if (itConnection == gConnectedUsers.end())
	{
		std::cerr << "\n>> ERROR: Attempted to receive connection on socket "
			<< socket << " but no ClientConnection was found!\n";
		gShowListeningStatus = true;;
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
			//std::cerr << "\n>> ERROR: Unable to read message length for socket "
			//	<< socket << "!\n";
			gShowListeningStatus = true;;
			RemoveClient(socket, connection);
			return;
		}

		connection->messageSize = (uint8_t)msgLength;

		if (connection->messageSize < 1)
		{
			std::cerr << "\n>> ERROR: Invalid message size read in on socket "
				<< socket << " (" << connection->messageSize << ")!\n";
			gShowListeningStatus = true;;
			RemoveClient(socket, connection);
			return;
		}
		connection->messageBuffer = new char[connection->messageSize + 1];
		connection->bytesProcessed = 0;
	}
	else
	{
		// Read in messages
		result = recv(socket, connection->messageBuffer + connection->bytesProcessed,
			connection->messageSize - connection->bytesProcessed, 0);
	
		if (result == 0)
		{
			RemoveClient(socket, connection);
			return;
		}
		if (result == SOCKET_ERROR)
		{
			std::cerr << "\n>> ERROR: Unable to read client message on socket "
				<< socket << " (SOCKET_ERROR)!\n";
			gShowListeningStatus = true;;
			RemoveClient(socket, connection);
			return;
		}

		connection->bytesProcessed += result;

		if (connection->bytesProcessed >= connection->messageSize)
		{
			connection->messageBuffer[connection->messageSize] = '\0';
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
	size_t startOfUsername = message.find(" ");
	if (startOfUsername == std::string::npos)
	{
		std::cerr << "\n>> ERROR: Register event received however "
			<< "no username was accompanying it!\n";
		gShowListeningStatus = true;;
		LogEvent("$register - ERROR: No username associated with register event.");
		RemoveClient(socket, clientConnection, false);
		return false;
	}
	std::string username = message.substr(startOfUsername + 1);

	if (clientConnection->clientUsername.compare("") == 0)
	{
		// Handle SV_FULL
		if (gConnectedUsers.size() >= MaxServerCapacity)
		{
			SendClientMessage(socket, clientConnection, CSCA::SV_FULL);
			std::cout << "> Refused client due to max server capacity!\n";
			gShowListeningStatus = true;
			LogEvent("$register - Refused client due to max server capacity.");
			RemoveClient(socket, clientConnection, false);
			return false;
		}
	}
	else
	{
		gUsernames.erase(clientConnection->clientUsername);
		std::cout << "\n> Changing client username from '" << clientConnection->clientUsername
			<< "' to '" << username << ".\n";
	}

	if (gUsernames.find(username) != gUsernames.end())
	{
		std::cerr << "\n>> ERROR: Client tried to register with a username that "
			<< "has already been claimed!\n";
		gShowListeningStatus = true;;
		std::string logMsg = "$register ERROR: - Duplicate username registration (";
		logMsg.append(username); logMsg.append(")");
		LogEvent(logMsg);
		SendClientMessage(socket, clientConnection, CSCA::SV_USERNAME_TAKEN);
		return true;
	}
	
	clientConnection->clientUsername = username;
	gUsernames.insert(std::pair<std::string, SOCKET>(username, socket));
	LogRegister(clientConnection);
	SendClientMessage(socket, clientConnection, CSCA::SV_REGISTER_SUCCESS);

	std::cout << "\n> Registered client under '" << username << "'!\n";
	gShowListeningStatus = true;;

	return true;
}

bool ServerApp::HandleGetUserList(SOCKET socket, CSCA::ClientConnection* clientConnection)
{
	std::string clientList = "";

	for (auto itter = gUsernames.begin(); itter != gUsernames.end(); itter++)
	{
		clientList.append(itter->first);
		clientList.append("\n");
	}

	std::string message = CSCA::SV_CLIENT_LIST + " ";
	message.append(clientList);
	SendClientMessage(socket, clientConnection, message);

	std::string getUserLog = "User ";
	getUserLog.append(clientConnection->clientUsername);
	getUserLog.append(" requested user list.\n");
	LogEvent(getUserLog);

	std::cout << "\n> User '" << clientConnection->clientUsername << "' requested user list!\n";

	return true;
}

bool ServerApp::HandleClientMessage(SOCKET socket, CSCA::ClientConnection* clientConnection)
{
	std::string message(clientConnection->messageBuffer);
	// Handle Commands
	if (message[0] == '$')
	{
		std::string command = message.find(" ") == std::string::npos
			? message
			: message.substr(0, message.find(" "));
		if (command.compare("$register") == 0)
		{
			return HandleRegisterClient(socket, clientConnection, message);
		}
		else if (command.compare("$getlist") == 0)
		{
			return HandleGetUserList(socket, clientConnection);
		}
		else if (command.compare("$getlog") == 0)
		{
			return true;
		}
		else if (command.compare("$exit") == 0)
		{
			LogExit(clientConnection);
			RemoveClient(socket, clientConnection, false);
			return false;
		}
		else
		{
			std::cerr << "\n>> ERROR: Unknown client command received: "
				<< command << "!\n";
			gShowListeningStatus = true;;
			return true;
		}
	}
	else
	{
		// Handle Normal Message
		return SendToAllClients(message, clientConnection->clientUsername);
	}
}

void ServerApp::AcceptNewClient()
{
	// Listen for clients
	SOCKET commSocket = accept(gListenSocket, NULL, NULL);
	if (commSocket == INVALID_SOCKET)
	{
		std::cerr << "\n>> ERROR: Failed to accept on listenSocket!\n";
		return;
	}
	else
	{
		std::cout << "\n> Received new connection!\n";
		gShowListeningStatus = true;;
		FD_SET(commSocket, &gMasterSet);
		if (gConnectedUsers.find(commSocket) == gConnectedUsers.end())
		{
			CSCA::ClientConnection* newClientConnection = new CSCA::ClientConnection;
			newClientConnection->bytesProcessed = 0;
			newClientConnection->messageBuffer = nullptr;
			newClientConnection->messageSize = 0;
			newClientConnection->clientUsername = "";
			gConnectedUsers.insert(std::pair<SOCKET, CSCA::ClientConnection*>(
				commSocket,
				newClientConnection));
		}
		else
		{
			std::cerr << "\n>> ERROR: Connection on socket " << commSocket
				<< " was already established!\n";
		}
	}
}

bool ServerApp::SendToAllClients(std::string message, std::string username)
{
	std::cout << "\n> Sending message from " << username << " to all clients!\n";

	std::string realMessage = CSCA::SV_CLIENT_MESSAGE;
	realMessage.append(" ");
	realMessage.append(username);
	realMessage.append(": ");
	realMessage.append(message);
	LogEvent(realMessage);
	for (auto it = gConnectedUsers.begin(); it != gConnectedUsers.end(); it++)
	{
		if (it->second->clientUsername.compare(username) != 0)
			SendClientMessage(it->first, it->second, realMessage);
	}

	return true;
}

void ServerApp::SendClientMessage(SOCKET socket, 
	CSCA::ClientConnection* clientConnection, std::string message)
{
	int result, bytesSent = 0;
	size_t msgSize = message.size();

	// Send message length
	while (bytesSent < 1)
	{
		
		result = send(socket, (const char*)&msgSize, 1, 0);
		if (result == 0)
		{
			RemoveClient(socket, clientConnection);
			return;
		}
		if (result == SOCKET_ERROR)
		{
			std::cerr << "\n>> ERROR: Unable to send message to client on socket "
				<< socket << " (SOCKET_ERROR)!\n";
			RemoveClient(socket, clientConnection);
			return;
		}
		bytesSent += result;
	}

	// Send actual message
	bytesSent = 0;
	while (bytesSent < msgSize)
	{
		result = send(socket, (const char*)message.c_str() + bytesSent,
			msgSize - bytesSent, 0);
		if (result < 0)
		{
			std::cerr << "\n>> ERROR: Unable to send message to client on socket "
				<< socket << " (SOCKET_ERROR)!\n";
			RemoveClient(socket, clientConnection);
			return;
		}
		bytesSent += result;
	}
}

void ServerApp::RemoveClient(SOCKET clientSocket, 
	CSCA::ClientConnection* clientConnection, bool logRemoval)
{
	if (logRemoval)
	{
		LogDisconnect(clientConnection);
	}

	if (clientConnection != nullptr)
	{
		std::cout << "\n> " << clientConnection->clientUsername << " disconnected from the server!\n";
		gUsernames.erase(clientConnection->clientUsername);
		if (clientConnection->messageBuffer != nullptr)
			delete[] clientConnection->messageBuffer;
		delete clientConnection;
	}

	FD_CLR(clientSocket, &gMasterSet);
	gConnectedUsers.erase(clientSocket);
	closesocket(clientSocket);
}

void ServerApp::LogDisconnect(CSCA::ClientConnection* clientConnection)
{
	if (clientConnection != nullptr)
	{
		std::string logMsg = clientConnection->clientUsername;
		logMsg.append(" disconnected from the server.");
		LogEvent(logMsg);
	}
}

void ServerApp::LogExit(CSCA::ClientConnection* clientConnection)
{
	if (clientConnection != nullptr)
	{
		std::string logMsg = "$exit - ";
		logMsg.append(clientConnection->clientUsername);
		LogEvent(logMsg);
	}
}

void ServerApp::LogRegister(CSCA::ClientConnection* clientConnection)
{
	if (clientConnection != nullptr)
	{
		std::string logMsg = "$register - ";
		logMsg.append(clientConnection->clientUsername);
		LogEvent(logMsg);
	}
}

void ServerApp::LogEvent(std::string eventStr)
{

}

void ServerApp::ShowListeningStatus()
{
	std::cout << "\tListening for actions... ";
	gShowListeningStatus = false;
}