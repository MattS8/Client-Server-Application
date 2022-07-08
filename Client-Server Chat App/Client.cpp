#include "Client.h"
#include <winsock2.h>
#include <iostream>
#include <string>

#pragma region Forward Declarations
bool ParseIPAddress(std::string ipAddrStr, unsigned short* dest);

#pragma endregion


void ClientApp::Run()
{
	gSocketInfo = GetTCPSocketInfo(); // TODO Listen over UDP for server connection

	// Create comm socket
	SOCKET comSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (comSocket == INVALID_SOCKET)
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
	int result = connect(comSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (result == SOCKET_ERROR)
	{
		std::cerr << ">> ERROR: Failed to connect to: " << gSocketInfo.ipAddrStr << "\n";
		return;
	}
	else
	{
		std::cout << ">> Successfully connected to: " << gSocketInfo.ipAddrStr << "\n";
		printf("DEBUG// I used the Connect function\n");
	}
}

CSCA::SocketInfo ClientApp::GetTCPSocketInfo()
{
	std::cout
		<< "         TCP Socket Info         \n"
		<< "_________________________________\n";

	CSCA::SocketInfo socketInfo;

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

