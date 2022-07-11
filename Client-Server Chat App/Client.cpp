#include "Client.h"
#include <stdio.h>

#pragma region Forward Declarations
bool ParseIPAddress(std::string ipAddrStr, unsigned short* dest);

#pragma endregion

// Text Art Generated at https://patorjk.com/

void ClientApp::Run()
{
	std::cout 
		<< "____ _    _ ____ _  _ ___    _  _ ____ ___  ____   \n"
		<< "|    |    | |___ |\\ |  |     |\\/| |  | |  \\ |___ . \n"
		<< "|___ |___ | |___ | \\|  |     |  | |__| |__/ |___ . \n"
		<< "                                                   \n";

	gSocketInfo = QueryTCPSocketInfo(); // TODO Listen over UDP for server connection

	// Create comm socket
	gComSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (gComSocket == INVALID_SOCKET)
	{
		std::cerr << ">> ERROR: Unable to create communication socket!\n";
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
		return;
	}
	else
	{
		std::cout << ">> Successfully connected to: " << gSocketInfo.ipAddrStr << "\n";
	}

	FD_SET(gComSocket, &gMasterSet);
	while (true)
	{
		gReadySet = gMasterSet;
		int rc = select(0, &gReadySet, NULL, NULL, &gTimeout);

		if (rc != 0)
		{
			for (int i = 0; i < gReadySet.fd_count; i++)
			{
				if (gReadySet.fd_array[i] == _fileno(stdin))
					QueryUserAction();
				ReceiveServerMessage(gReadySet.fd_array[i]);
			}
		}
		else
		{
			if (gClientState == NONE)
			{
				std::cout
					<< "\n          Select Action         \n"
					<< "_________________________________\n"
					<< "1) Set Username\n"
					<< "2) Send Message\n"
					<< "3) Get List of Users\n"
					<< "4) Get Logs\n"
					<< "5) Exit\n";
				gClientState = QUERY_ACTION;
			}
		}
	}
}

void ClientApp::LeaveServer()
{
	FD_CLR(gComSocket, &gMasterSet);
	closesocket(gComSocket);
}

void ClientApp::ReceiveServerMessage(SOCKET socket)
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
			return;
		}

		gServerMessage.messageSize = (uint8_t)msgLength;

		if (gServerMessage.messageSize < 1)
		{
			std::cerr << "\n>> ERROR: Invalid message size read in on socket "
				<< socket << " (" << gServerMessage.messageSize << ")!\n";
			LeaveServer();
			return;
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
			return;
		}
		if (result == SOCKET_ERROR)
		{
			std::cerr << "\n>> ERROR: Unable to read client message on socket "
				<< socket << " (SOCKET_ERROR)!\n";
			LeaveServer();
			return;
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
		}
	}
}

bool ClientApp::HandleServerMessage()
{
	std::string serverMessage(gServerMessage.messageBuffer);

	std::string msgType = serverMessage.find(" ") == std::string::npos
		? serverMessage
		: serverMessage.substr(0, serverMessage.find(" "));

	if (msgType.compare("SV_FULL") == 0)
	{
		std::cout << "\n>> ERROR: Server is full!\n";
		return true;
	}
	else if (msgType.compare("SV_USERNAME_TAKEN") == 0)
	{
		std::cout << "\n>> ERROR: Username already taken!\n";
		return true;
	}
	else if (msgType.compare("SV_REGISTER_SUCCESS") == 0)
	{
		std::cout << "\n>> Registered as '" << gServerMessage.clientUsername << "'!\n";
		return true;
	}
	else if (msgType.compare("SV_CLIENT_MESSAGE") == 0)
	{
		std::cout << "\n__Incoming Message__\n>"
			<< serverMessage.substr(serverMessage.find(" ")+1);
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

void ClientApp::QueryUserAction()
{
	int choice = -1;
	do
	{
		std::cin >> choice;
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	} while (choice < 1 || choice > 5);
	switch (choice)
	{
	case 1:
		RegisterClient(QueryUsername());
		break;
	case 2:
		SendMessage(QueryClientMessage());
	default:
		break;
	}
}

void ClientApp::SendMessage(std::string message)
{
	int result, bytesSent = 0;
	size_t msgSize = message.size();

	while (bytesSent < 1)
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
	std::string registerMsg = "$register ";
	SendMessage(registerMsg.append(username));
}

void ClientApp::Exit(bool sendExitMessage /* = true */)
{
	if (sendExitMessage)
	{
		// TODO Send Exit Message
	}

	LeaveServer();
	exit(0);
}

std::string ClientApp::QueryClientMessage()
{
	static const size_t MAX_MSG_SIZE = 1000;
	char msgBuffer[MAX_MSG_SIZE];
	bool messageAcquired = false;

	do
	{
		std::cout
			<< "\n\n>> Enter Message: ";
		std::cin.getline(msgBuffer, MAX_MSG_SIZE);

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

	return std::string(msgBuffer);
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

