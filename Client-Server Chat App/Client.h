#pragma once
#include "Communcation.h"

class ClientApp
{
	CSCA::SocketInfo gSocketInfo;

	CSCA::SocketInfo GetTCPSocketInfo();
public:
	void Run(CSCA::SocketInfo socketInfo = { 0,0 });
};