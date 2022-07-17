#define _CRT_SECURE_NO_DEPRECATE
#include "Client.h"
#include <stdio.h>

#include <fstream>
#include <sstream>

#pragma region Forward Declarations
bool ParseIPAddress(std::string ipAddrStr, unsigned short* dest);

#pragma endregion


// Text Art Generated at https://patorjk.com/

void ClientApp::ConnectToServer()
{
	gSocketInfo = QueryTCPSocketInfo(); // TODO Listen over UDP for server connection

// Create comm socket
	gComSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (gComSocket == INVALID_SOCKET)
	{
		std::cerr << ">> ERROR: Unable to create communication socket!\n";
		exit(-1);
		return;
	}

	// Connect to server
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr(gSocketInfo.ipAddrStr.c_str());
	serverAddr.sin_port = htons(gSocketInfo.port);

	// Check connection result
	int result = connect(gComSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (result == SOCKET_ERROR)
	{
		std::cerr << ">> ERROR: Failed to connect to: " << gSocketInfo.ipAddrStr << "\n";
		Exit();
		return;
	}
	else
	{
		std::cout << ">> Successfully connected to: " << gSocketInfo.ipAddrStr << "\n";
	}
	gIsConnectedToServer = true;
}

void ClientApp::Run()
{
	std::cout 
		<< "____ _    _ ____ _  _ ___    _  _ ____ ___  ____   \n"
		<< "|    |    | |___ |\\ |  |     |\\/| |  | |  \\ |___ . \n"
		<< "|___ |___ | |___ | \\|  |     |  | |__| |__/ |___ . \n"
		<< "                                                   \n";

	ConnectToServer();
	gServerMessage.readMessageBuffer = nullptr;
	gServerMessage.readBuffSize = 0;
	gServerMessage.readBytesProcessed = 0;
	gServerMessage.sendMessageBuffer = nullptr;
	gServerMessage.sendBuffSize = 0;
	gServerMessage.sendBytesProcessed = 0;
	gClientState = QUERY_ACTION;
	FD_SET(gComSocket, &gMasterSet);
	while (true)
	{
		gReadReadySet = gMasterSet;
		gSendReadySet = gMasterSet;
		int rc = select(0, &gReadReadySet, &gSendReadySet, NULL, &gTimeout);


		if (gServerMessage.sendMessageBuffer != nullptr)
		{
			for (int i = 0; i < gSendReadySet.fd_count; i++)
			{
				SendClientMessage(gSendReadySet.fd_array[i]);
			}
		}

		for (int i = 0; i < gReadReadySet.fd_count; i++)
		{
			if (!ReceiveServerMessage(gReadReadySet.fd_array[i]))
				continue;
		}

		if (gClientState == WAIT_FOR_MESSAGES && gReadReadySet.fd_count == 0)
		{
			gClientMessages.append("\n_________________________________\n\n");
			std::cout << gClientMessages;
			gClientMessages.clear();
			gClientState = QUERY_ACTION;
		}

		if (gClientState == QUERY_ACTION)
		{
			DisplayOptions();
			QueryUserAction();
		}
	}
}

void ClientApp::LeaveServer()
{
	if (gServerMessage.readMessageBuffer != nullptr)
		delete[] gServerMessage.readMessageBuffer;
	if (gServerMessage.sendMessageBuffer != nullptr)
		delete[] gServerMessage.sendMessageBuffer;

	FD_CLR(gComSocket, &gMasterSet);
	closesocket(gComSocket);
	gIsConnectedToServer = false;
	gIsRegisteredWithServer = false;
}

bool ClientApp::ReceiveServerMessage(SOCKET socket)
{
	int result;
	if (gServerMessage.readBuffSize == 0)
	{
		// Read in Size
		char msgLength;
		result = recv(socket, &msgLength, 1, 0);
		if (result < 1)
		{
			std::cerr << "\n>> ERROR: Unable to read message length for socket "
				<< socket << "!\n";
			Exit(false);
			return false;
		}

		gServerMessage.readBuffSize = (uint8_t)msgLength;

		if (gServerMessage.readBuffSize < 1)
		{
			std::cerr << "\n>> ERROR: Invalid message size read in on socket "
				<< socket << " (" << gServerMessage.readBuffSize << ")!\n";
			Exit(false);
			return false;
		}
		gServerMessage.readMessageBuffer = new char[gServerMessage.readBuffSize + 1];
		gServerMessage.readBytesProcessed = 0;
	}
	else
	{
		// Read in messages
		result = recv(socket, gServerMessage.readMessageBuffer + gServerMessage.readBytesProcessed,
			gServerMessage.readBuffSize - gServerMessage.readBytesProcessed, 0);

		if (result == 0)
		{
			Exit(false);
			return false;
		}
		if (result == SOCKET_ERROR)
		{
			std::cerr << "\n>> ERROR: Unable to read client message on socket "
				<< socket << " (SOCKET_ERROR)!\n";
			Exit(false);
			return false;
		}

		gServerMessage.readBytesProcessed += result;

		if (gServerMessage.readBytesProcessed >= gServerMessage.readBuffSize)
		{
			gServerMessage.readMessageBuffer[gServerMessage.readBuffSize] = '\0';
			// Handle full message received
			if (HandleServerMessage())
			{
				delete[] gServerMessage.readMessageBuffer;
				gServerMessage.readMessageBuffer = nullptr;
				gServerMessage.readBuffSize = 0;
			}
		}
	}

	return true;
}

bool ClientApp::HandleServerMessage()
{
	std::string serverMessage(gServerMessage.readMessageBuffer);

	std::string msgType = serverMessage.find(" ") == std::string::npos
		? serverMessage
		: serverMessage.substr(0, serverMessage.find(" "));

	if (msgType.compare(CSCA::SV_FULL) == 0)
	{
		std::cout << "\n>> ERROR: Server is full!\n";
		gClientState = QUERY_ACTION;
		return true;
	}
	else if (msgType.compare(CSCA::SV_USERNAME_TAKEN) == 0)
	{
		std::cout << "\n>> ERROR: Username already taken!\n";
		gClientState = QUERY_ACTION;
		return true;
	}
	else if (msgType.compare(CSCA::SV_REGISTER_SUCCESS) == 0)
	{
		std::cout << "\n>> Registered as '" << gServerMessage.clientUsername << "'!\n";
		gIsRegisteredWithServer = true;
		gClientState = QUERY_ACTION;
		return true;
	}
	else if (msgType.compare(CSCA::SV_CLIENT_LIST) == 0)
	{
		std::cout
			<< "\n        List of Users:         \n"
			<< "_________________________________\n\n";
		std::string userList = serverMessage.substr(serverMessage.find(" ")+1);
		size_t endPos = userList.find("\n");

		while (true)
		{
			std::string username = userList;
			username = username.substr(0, endPos+1);
			std::cout << "> " << username;
			endPos = userList.find("\n");
			if (endPos == std::string::npos || (endPos+1) >= userList.size())
				break;
			userList = userList.substr(endPos + 1);
		}
		std::cout << "_________________________________\n";
		gClientState = QUERY_ACTION;
		return true;
	}
	else if (msgType.compare(CSCA::SV_GET_LOGS) == 0) {
		std::string logs = serverMessage.substr(serverMessage.find(" ") + 1);
		std::string clientLogFilename = "ClientLogs.log";
		FILE* logFile = fopen(clientLogFilename.c_str(), "w");
		if (logFile == NULL)
		{
			std::cout << "\n>> ERROR: Unable to open log file!\n";
			return true;
		}

		fprintf(logFile, "%s", logs.c_str());

		fclose(logFile);

		std::cout << "\n> Log file received!\n";
		gClientState = QUERY_ACTION;
		return true;
	}
	else if (msgType.compare(CSCA::SV_CLIENT_MESSAGE) == 0)
	{
		gClientMessages.append("\n> ");
		gClientMessages.append(serverMessage.substr(serverMessage.find(" ") + 1));
		return true;
	}
	else
	{
		std::cout << "\n>> ERROR: Received unknown message type from server: "
			<< msgType << "!\n";
		gClientState = QUERY_ACTION;
		return true;
	}
}

bool ClientApp::SendClientMessage(SOCKET socket)
{
	int result;

	if (!gSentSize)
	{
		// Send Size
		size_t msgSize = gServerMessage.sendBuffSize;
		result = send(gComSocket, (const char*)&msgSize, 1, 0);
		if (result == 0)
		{
			Exit(false);
			return false;
		}
		if (result == SOCKET_ERROR)
		{
			std::cerr << "\n>> ERROR: Unable to send message to server on socket "
				<< gComSocket << " (SOCKET_ERROR)!\n";
			Exit(false);
			return false;
		}
		else
		{
			gSentSize = true;
			gServerMessage.sendBytesProcessed = 0;
			return true;
		}
	}
	else
	{
		// Send Message
		result = send(gComSocket, (const char*)gServerMessage.sendMessageBuffer + gServerMessage.sendBytesProcessed,
			gServerMessage.sendBuffSize - gServerMessage.sendBytesProcessed, 0);
		if (result < 0)
		{
			std::cerr << "\n>> ERROR: Unable to send message to client on socket "
				<< gComSocket << " (SOCKET_ERROR)!\n";
			gSentSize = false;
			Exit(false);
			return false;
		}
		gServerMessage.sendBytesProcessed += result;

		if (gServerMessage.sendBytesProcessed == gServerMessage.sendBuffSize)
		{
			//std::cout << "\n> Message sent!\n";
			delete[] gServerMessage.sendMessageBuffer;
			gServerMessage.sendMessageBuffer = nullptr;
			gServerMessage.sendBuffSize = 0;
			gSentSize = false;
			if (gClientState == SENDING_MESSAGE)
				gClientState = QUERY_ACTION;

			return true;
		}

		return false;
	}
}

void ClientApp::SendMessage(std::string message)
{
	if (gServerMessage.sendMessageBuffer != nullptr)
	{
		std::cout << "\n>> ERROR: Previous message to the server has not gone out yet!\n";
		gClientState = QUERY_ACTION;
		return;
	}

	gServerMessage.sendMessageBuffer = new char[message.size()];
	gServerMessage.sendBuffSize = message.size();
	memcpy(gServerMessage.sendMessageBuffer, message.c_str(), message.size());
	gSentSize = false;
	return;

	// BLOCKING IMPLEMENTATION

	int result, bytesSent = 0;
	size_t msgSize = message.size() + 1;

	while (bytesSent < 1 && msgSize > 0)
	{
		// Send message length
		result = send(gComSocket, (const char*)&msgSize, 1, 0);
		if (result == 0)
		{
			Exit(false);
			return;
		}
		if (result == SOCKET_ERROR)
		{
			std::cerr << "\n>> ERROR: Unable to send message to server on socket "
				<< gComSocket<< " (SOCKET_ERROR)!\n";
			Exit(false);
			return;
		}

		// Send actual message
		bytesSent = 0;
		while (bytesSent < msgSize)
		{
			result = send(gComSocket, (const char*)message.c_str() + bytesSent,
				msgSize - bytesSent, 0);
			if (result < 0)
			{
				std::cerr << "\n>> ERROR: Unable to send message to client on socket "
					<< gComSocket<< " (SOCKET_ERROR)!\n";
				Exit(false);
				return;
			}
			bytesSent += result;
		}
	}
}

void ClientApp::RegisterClient(std::string username)
{
	gServerMessage.clientUsername = username;
	std::string registerMsg = CSCA::SV_REGISTER_USERNAME;
	registerMsg.append(" ");
	SendMessage(registerMsg.append(username));
}

void ClientApp::Exit(bool sendExitMessage /* = true */)
{
	if (gServerMessage.readMessageBuffer != nullptr)
		delete[] gServerMessage.readMessageBuffer;
	if (gServerMessage.sendMessageBuffer != nullptr)
		delete[] gServerMessage.sendMessageBuffer;

	if (sendExitMessage && gIsConnectedToServer)
	{
		SendMessage(CSCA::SV_EXIT);
	}
	std::cout << "\n\n> Press ENTER to exit.\n";
	std::cin.ignore();
	std::cin.get();

	LeaveServer();
	exit(0);
}

void ClientApp::DisplayOptions(bool setState /* = true */)
{
	if (gIsRegisteredWithServer)
	{
		std::cout
			<< "\n          Select Action         \n"
			<< "_________________________________\n"
			<< "1) Set Username\n"
			<< "2) Send Message\n"
			<< "3) Check for Messages\n"
			<< "4) Get List of Users\n"
			<< "5) Get Logs\n"
			<< "6) Exit\n";
	}
	else
	{
		std::cout
			<< "\n          Select Action         \n"
			<< "_________________________________\n"
			<< "1) Set Username\n"
			<< "2) Exit\n";
	}

	if (setState)
		gClientState = QUERY_ACTION;
}

#pragma region Query Functions
void ClientApp::QueryUserAction()
{
	int choice = -1;
	do
	{
		std::cin >> choice;
		if (std::cin.fail())
		{
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}

	} while (!WithinRange(choice));

	switch (choice)
	{
	case 1:
		RegisterClient(QueryUsername());
		gClientState = REGISTER_USERNAME;
		return;
	case 2:
		if (gIsRegisteredWithServer)
		{
			SendMessage(QueryClientMessage());
			gClientState = SENDING_MESSAGE;
			return;
		}
		else
		{
			Exit();
			return;
		}
		break;
	case 3:
		if (gClientMessages.size() > 0)
		{
			std::string temp(gClientMessages.c_str());
			gClientMessages = "\n       Unread Messages:        \n";
			gClientMessages.append("_________________________________\n");
			gClientMessages.append(temp);
		}
		else
		{
			gClientMessages = "\n       Unread Messages:        \n";
			gClientMessages.append("_________________________________\n");
		}
		gClientState = WAIT_FOR_MESSAGES;
		return;
	case 4:
		SendMessage(CSCA::SV_GET_CLIENT_LIST);
		gClientState = GETTING_CLIENT_LIST;
		return;
	case 5:
		SendMessage(CSCA::SV_GET_LOGS);
		gClientState = GETTING_LOGS;
		return;
	case 6:
		Exit();
		return;
	default:
		break;
	}

	gClientState = QUERY_ACTION;
}

std::string ClientApp::QueryClientMessage()
{
	static const size_t MAX_MSG_SIZE = 1000;

	bool messageAcquired = false;
	std::string message;

	do
	{
		std::cout
			<< "\n\n>> Enter Message: ";
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::getline(std::cin, message);

		if (std::cin.fail())
		{
			std::cerr << "\n>> ERROR: Message was too long!\n";
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}
		else
		{
			messageAcquired = true;
		}
	} while (!messageAcquired);

	return message;
}

std::string ClientApp::QueryUsername()
{
	std::string username;
	std::cout
		<< "\n\n>> Set Username: ";
	std::cin >> username;
	if (std::cin.fail())
	{
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return username;
}

CSCA::SocketInfo ClientApp::QueryTCPSocketInfo()
{
	std::cout
		<< "         TCP Socket Info         \n"
		<< "_________________________________\n";

	CSCA::SocketInfo socketInfo;
	/*****DEBUG****/
	socketInfo.ipAddrStr = "127.0.0.1";
	socketInfo.port = 31337;
	return socketInfo;

	// Get IP Address
	do
	{
		printf("Enter IP Address (IPv4): ");
		std::cin >> socketInfo.ipAddrStr;
	} while (!ParseIPAddress(socketInfo.ipAddrStr, socketInfo.ipAddr));

	// Get Port Number
	printf("Enter Port Number: ");
	std::cin >> socketInfo.port;
	printf("\n\n");

	return socketInfo;
}
#pragma endregion

#pragma region Helper Functions
bool ClientApp::WithinRange(int choice)
{
	if (gIsRegisteredWithServer)
		return choice >= 1 && choice <= 6;
	else
		return choice >= 1 && choice <= 2;
}

bool ParseIPAddress(std::string ipAddrStr, unsigned short* dest)
{
	int start = 0;
	int dotPos = 0;
	for (int i = 0; i < 3; i++)
	{
		dotPos = ipAddrStr.find('.', start);
		if (dotPos == std::string::npos)
			return false;

		dest[i] = std::stoi(ipAddrStr.substr(start, dotPos - start));
		start = dotPos + 1;
	}

	dest[3] = std::stoi(ipAddrStr.substr(start));

	return true;
}

#pragma endregion

