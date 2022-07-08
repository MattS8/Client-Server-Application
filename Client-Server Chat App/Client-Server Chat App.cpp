// Client-Server Chat App.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _CRT_SECURE_NO_WARNINGS                 // turns of deprecated warnings
#define _WINSOCK_DEPRECATED_NO_WARNINGS         // turns of deprecated warnings for winsock
#include <winsock2.h>
//#include <ws2tcpip.h>                         // only need if you use inet_pton
#pragma comment(lib,"Ws2_32.lib")

#include <iostream>
#include "Client.h"
#include "Server.h"
#include "Communcation.h"
#include <string>

#pragma region Forward Declarations
void SelectApplicationType();

bool ParseIPAddress(std::string ipAddrStr, unsigned short* dest);
#pragma endregion

namespace
{
	ClientApp gClient;
	ServerApp gServer;
}

int main()
{
	WSADATA wsadata;
	WSAStartup(WINSOCK_VERSION, &wsadata);

	std::cout
		<< "///////////////////////////////////////////////////////////////////////////////////////////////////////////////\n"
		<< "//   _____  _  _               _            _____                                  _____  _             _    //\n"
		<< "//  / ____|| |(_)             | |          / ____|                                / ____|| |           | |   //\n"
		<< "// | |     | | _   ___  _ __  | |_  ______| (___    ___  _ __ __   __ ___  _ __  | |     | |__    __ _ | |_  //\n"
		<< "// | |     | || | / _ \\| '_ \\ | __||______|\\___ \\  / _ \\| '__|\\ \\ / // _ \\| '__| | |     | '_ \\  / _` || __| //\n"
		<< "// | |____ | || ||  __/| | | || |_         ____) ||  __/| |    \\ V /|  __/| |    | |____ | | | || (_| || |_  //\n"
		<< "//  \\_____||_||_| \\___||_| |_| \\__|       |_____/  \\___||_|     \\_/  \\___||_|     \\_____||_| |_| \\__,_| \\__| //\n"
		<< "//                                                                                                           //\n"
		<< "///////////////////////////////////////////////////////////////////////////////////////////////////////////////\n\n";

	SelectApplicationType();

	return WSACleanup();
}

void SelectApplicationType()
{
	std::cout
		<< "         Application Mode        \n"
		<< "_________________________________\n";
	int choice;
	do
	{
		printf("Would you like to Create a Server or Client?\n");
		printf("1) Server\n");
		printf("2) Client\n");
		printf("> ");
		std::cin >> choice;
	} while (choice != 1 && choice != 2);

	if (choice == 1)
		gServer.Run();
	else if (choice == 2)
		gClient.Run();
}