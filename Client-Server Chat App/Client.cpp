#include "Client.h"
#include <stdio.h>

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

	gClientState = QUERY_ACTION;
	FD_SET(gComSocket, &gMasterSet);
	while (true)
	{
		if (gClientState == CONNECT_TO_SERVER)
			ConnectToServer();

		gReadySet = gMasterSet;
		int rc = select(0, &gReadySet, NULL, NULL, &gTimeout);

		for (int i = 0; i < gReadySet.fd_count; i++)
		{
			if (!ReceiveServerMessage(gReadySet.fd_array[i]))
				continue;
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
	FD_CLR(gComSocket, &gMasterSet);
	closesocket(gComSocket);
	gClientState = CONNECT_TO_SERVER;
	gIsConnectedToServer = false;
	gIsRegisteredWithServer = false;
}

bool ClientApp::ReceiveServerMessage(SOCKET socket)
{

	int result;
	if (gServerMessage.messageSize == 0)
	{
		// Read in Size
		char msgLength;
		result = recv(socket, &msgLength, 1, 0);
		if (result < 1)
		{
			std::cerr << "\n>> ERROR: Unable to read message length for socket "
				<< socket << "!\n";
			LeaveServer();
			return false;
		}

		gServerMessage.messageSize = (uint8_t)msgLength;

		if (gServerMessage.messageSize < 1)
		{
			std::cerr << "\n>> ERROR: Invalid message size read in on socket "
				<< socket << " (" << gServerMessage.messageSize << ")!\n";
			LeaveServer();
			return false;
		}
		gServerMessage.messageBuffer = new char[gServerMessage.messageSize + 1];
		gServerMessage.bytesRead = 0;
	}
	else
	{
		// Read in messages
		result = recv(socket, gServerMessage.messageBuffer + gServerMessage.bytesRead,
			gServerMessage.messageSize - gServerMessage.bytesRead, 0);

		if (result == 0)
		{
			LeaveServer();
			return false;
		}
		if (result == SOCKET_ERROR)
		{
			std::cerr << "\n>> ERROR: Unable to read client message on socket "
				<< socket << " (SOCKET_ERROR)!\n";
			LeaveServer();
			return false;
		}

		gServerMessage.bytesRead += result;

		if (gServerMessage.bytesRead >= gServerMessage.messageSize)
		{
			gServerMessage.messageBuffer[gServerMessage.messageSize] = '\0';
			// Handle full message received
			if (HandleServerMessage())
			{
				delete[] gServerMessage.messageBuffer;
				gServerMessage.messageBuffer = nullptr;
				gServerMessage.messageSize = 0;
			}
			gClientState = QUERY_ACTION;
		}
	}

	return true;
}

bool ClientApp::HandleServerMessage()
{
	std::string serverMessage(gServerMessage.messageBuffer);

	std::string msgType = serverMessage.find(" ") == std::string::npos
		? serverMessage
		: serverMessage.substr(0, serverMessage.find(" "));

	if (msgType.compare(CSCA::SV_FULL) == 0)
	{
		std::cout << "\n>> ERROR: Server is full!\n";
		return true;
	}
	else if (msgType.compare(CSCA::SV_USERNAME_TAKEN) == 0)
	{
		std::cout << "\n>> ERROR: Username already taken!\n";
		return true;
	}
	else if (msgType.compare(CSCA::SV_REGISTER_SUCCESS) == 0)
	{
		std::cout << "\n>> Registered as '" << gServerMessage.clientUsername << "'!\n";
		gIsRegisteredWithServer = true;
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
		return true;
	}
	else if (msgType.compare(CSCA::SV_CLIENT_MESSAGE) == 0)
	{
		std::cout << "\n> "
			<< serverMessage.substr(serverMessage.find(" ")+1)
			<< "\n_________________________________\n\n";
		return true;
	}
	else
	{
		std::cout << "\n>> ERROR: Received unknown message type from server: "
			<< msgType << "!\n";
		return true;
	}
}

static std::string getChoice()
{
	int choice;
	std::cin >> choice;
	return std::to_string(choice);
}

bool ClientApp::WithinRange(int choice)
{
	if (gIsRegisteredWithServer)
		return choice >= 1 && choice <= 6;
	else
		return choice >= 1 && choice <= 2;
}

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
		}
		else
		{
			Exit();
			return;
		}
		break;
	case 3:
		SendMessage(CSCA::SV_GET_CLIENT_LIST);
		gClientState = GETTING_CLIENT_LIST;
		return;
	case 5:
		WaitForMessages();
		return;
	case 6:
		Exit();
		return;
	default:
		break;
	}

	gClientState = QUERY_ACTION;
}

void ClientApp::WaitForMessages()
{
	gClientState = WAIT_FOR_MESSAGES;
	std::cout
		<< "\n   Listening For Message...    \n"
		<< "_________________________________\n";
}

void ClientApp::SendMessage(std::string message)
{
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
			std::cerr << "\n>> ERROR: Unable to send message to client on socket "
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
	if (sendExitMessage && gIsConnectedToServer)
	{
		SendMessage(CSCA::SV_EXIT);
	}

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
			<< "3) Get List of Users\n"
			<< "4) Get Logs\n"
			<< "5) Wait for Message\n"
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


#pragma region Helper Functions


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

