#define NOMINMAX
#include "Server.h"
#include <iostream>

#include <chrono>

// Text Art Generated at https://patorjk.com/

double ServerApp::GetDeltaTime()
{
	static std::chrono::time_point<std::chrono::high_resolution_clock> last_time = std::chrono::high_resolution_clock::now();

	std::chrono::time_point<std::chrono::high_resolution_clock> new_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = new_time - last_time;
	last_time = new_time;

	return std::min(1.0 / 15.0, elapsed_seconds.count());
}

void ServerApp::Run()
{
	static float timeoutDelay = 0;
	static float MaxTimeout = 1;
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
		gSendReadySet = gMasterSet;
		int rc = select(0, &gReadReadySet, &gSendReadySet, NULL, &gTimeout);
		bool dataSentToClient = false;
		// Check to send messages out
		for (int i = 0; i < gSendReadySet.fd_count; i++)
		{
			CSCA::ClientConnection* cc = gConnectedUsers[gSendReadySet.fd_array[i]];
			if (cc != nullptr && cc->sendMessageBuffer != nullptr)
			{
				SendBytesToClient(gSendReadySet.fd_array[i], cc);
				dataSentToClient = true;
			}
		}

		// Check listen socket
		if (FD_ISSET(gListenSocket, &gReadReadySet))
		{
			AcceptNewClient();
		}

		// Check for client messages
		for (int i = 0; i < gReadReadySet.fd_count; i++)
		{
			if (gReadReadySet.fd_array[i] == gListenSocket)
				continue;
			ReceiveClientMessage(gReadReadySet.fd_array[i]);
		}

		timeoutDelay += GetDeltaTime();
		// Timeout
		if (gReadReadySet.fd_count == 0 && !dataSentToClient && timeoutDelay > MaxTimeout)
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
			timeoutDelay = 0;
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
	if (connection->readBuffSize == 0)
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

		connection->readBuffSize = (uint8_t)msgLength;

		if (connection->readBuffSize < 1)
		{
			std::cerr << "\n>> ERROR: Invalid message size read in on socket "
				<< socket << " (" << connection->readBuffSize << ")!\n";
			gShowListeningStatus = true;;
			RemoveClient(socket, connection);
			return;
		}
		connection->readMessageBuffer = new char[connection->readBuffSize + 1];
		connection->readBytesProcessed = 0;
	}
	else
	{
		// Read in messages
		result = recv(socket, connection->readMessageBuffer + connection->readBytesProcessed,
			connection->readBuffSize - connection->readBytesProcessed, 0);
	
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

		connection->readBytesProcessed += result;

		if (connection->readBytesProcessed >= connection->readBuffSize)
		{
			connection->readMessageBuffer[connection->readBuffSize] = '\0';
			// Handle full message received
			if (HandleClientMessage(socket, connection))
			{
				delete[] connection->readMessageBuffer;
				connection->readMessageBuffer = nullptr;
				connection->readBuffSize = 0;
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

	gShowListeningStatus = true;

	return true;
}

bool ServerApp::HandleClientMessage(SOCKET socket, CSCA::ClientConnection* clientConnection)
{
	std::string message(clientConnection->readMessageBuffer);
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
			newClientConnection->readBytesProcessed = 0;
			newClientConnection->readMessageBuffer = nullptr;
			newClientConnection->readBuffSize = 0;
			newClientConnection->clientUsername = "";
			newClientConnection->sendBytesProcessed = 0;
			newClientConnection->sendMessageBuffer = nullptr;
			newClientConnection->sendBuffSize = 0;
			newClientConnection->sendSizeSent = false;
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

	gShowListeningStatus = true;
	return true;
}

void ServerApp::SendBytesToClient(SOCKET socket, CSCA::ClientConnection* clientConnection)
{
	int result;
	if (!clientConnection->sendSizeSent)
	{
		// Send message size
		result = send(socket, (const char*)&clientConnection->sendBuffSize, 1, 0);
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

		clientConnection->sendSizeSent = true;
	}
	else
	{
		// Send message
		result = send(socket, (const char*)clientConnection->sendMessageBuffer + clientConnection->sendBytesProcessed,
			clientConnection->sendBuffSize - clientConnection->sendBytesProcessed, 0);
		if (result < 0)
		{
			std::cerr << "\n>> ERROR: Unable to send message to client on socket "
				<< socket << " (SOCKET_ERROR)!\n";
			RemoveClient(socket, clientConnection);
			return;
		}
		clientConnection->sendBytesProcessed += result;

		// Remove message buffer on complete
		if (clientConnection->sendBytesProcessed == clientConnection->sendBuffSize)
		{
			delete[] clientConnection->sendMessageBuffer;
			clientConnection->sendMessageBuffer = nullptr;
			clientConnection->sendBuffSize = 0;
			clientConnection->sendBytesProcessed = 0;
			clientConnection->sendSizeSent = false;
		}
	}
}

void ServerApp::SendClientMessage(SOCKET socket, 
	CSCA::ClientConnection* clientConnection, std::string message)
{
	if (clientConnection->sendMessageBuffer != nullptr)
	{
		std::cout << "\n>> ERROR: tried sending new message to "
			<< clientConnection->clientUsername
			<< " however a message is currently being sent!\n\t"
			<< "The following message will be dropped: '"
			<< message << "'\n";
		return;
	}

	clientConnection->sendBuffSize = message.size();
	clientConnection->sendMessageBuffer = new char[clientConnection->sendBuffSize];
	clientConnection->sendBytesProcessed = 0;
	memcpy(clientConnection->sendMessageBuffer, message.c_str(), message.size());
	clientConnection->sendSizeSent = false;
	return;
	// OLD Implementation

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
		if (clientConnection->readMessageBuffer != nullptr)
			delete[] clientConnection->readMessageBuffer;
		if (clientConnection->sendMessageBuffer != nullptr)
			delete[] clientConnection->sendMessageBuffer;
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