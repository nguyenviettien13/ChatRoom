#include "Server.h"
#include <iostream>
#include <windows.h>
Server* Server::_serverPointer = nullptr;


Server::Server()
{
}


Server * Server::getServerInstance()
{
	if (_serverPointer == nullptr)
	{
		_serverPointer = new Server();
	}
	return _serverPointer;
}

bool Server::Initiate()
{
	_ConnectionSet = new SOCKET[MAXCONNECTION];
	_ConnectionMonitor = new bool[MAXCONNECTION];
	for (int i = 0; i < MAXCONNECTION; i++)
	{
		_ConnectionMonitor[i] = true; // true mean socket is available;
	}
	_connectionCounter = 0;
	return true;
}


bool Server::WakeUp()
{
	// Initialize Winsock.
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %ld\n", iResult);
		exit(0);
	}

	//----------------------
	// Create a SOCKET for listening for
	// incoming connection requests.
	_listenSocket = socket(AF_INET, SOCK_STREAM, NULL);
	if (_listenSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		exit(1);
	}

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = htonl(INADDR_ANY);
	service.sin_port = htons(PORT);

	if (bind(_listenSocket,
		(SOCKADDR *)& service, sizeof(service)) == SOCKET_ERROR)
	{
		wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
		closesocket(_listenSocket);
		WSACleanup();
		return 1;
	}
	//----------------------
	// Listen for incoming connection requests.
	// on the created socket
	if (listen(_listenSocket, 1) == SOCKET_ERROR) {
		wprintf(L"listen failed with error: %ld\n", WSAGetLastError());
		closesocket(_listenSocket);
		WSACleanup();
		exit(1);
	}
	return true;
}


void CommunicateWithOneClientHandler(Server::Param * p)
{
	//nhan messenger from client
	int index = p->_index;
	Server * serverPointer = p->_pointer;
	Packet PacketType;
	while (true)
	{
		if (!serverPointer->GetPacketType(index, PacketType)) //Get packet type
		{
			break; //If there is an issue getting the packet type, exit this loop
		}
		if (!serverPointer->ProcessPacket(index, PacketType)) //Process packet (packet type)
			break; //If there is an issue processing the packet, exit this loop
	}
	std::cout << "Lost connection to client ID: " << index << std::endl;
	serverPointer->_ConnectionMonitor[index] = true;
	closesocket(serverPointer->_ConnectionSet[index]);
	return;
}


void Server::ProcessRequest()
{
	while (true)
	{
		//Find the one Socket available
		int index = findIndexOfAvailableSocket();
		_ConnectionSet[index] = accept(_listenSocket, NULL, NULL);
		if (_ConnectionSet[index] == INVALID_SOCKET) {
			wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
			closesocket(_listenSocket);
			WSACleanup();
			exit(1);
		}
		_ConnectionMonitor[index] = false;
		cout << "New client Connected" << endl;

		//Gui loi chao tu server den client
		string greeting = "Welcome to Facebook messenger!!! ahihi";
		int sizeOfGreeting = greeting.size();
		if (send(this->_ConnectionSet[index], (char*)&sizeOfGreeting, sizeof(int), NULL) == SOCKET_ERROR)
		{
			std::cout << "My error 1: Gui loi chao den client : " << index << " bi loi" << endl;
		}
		else
		{
			//cout << "Gui thanh cong kich thuc cua greeting!" << sizeOfGreeting << endl;
		}

		if (send(this->_ConnectionSet[index], (char*)greeting.c_str(), greeting.size(), NULL) == SOCKET_ERROR)
		{
			std::cout << "My error 2: Gui loi chao den client : " << index << " bi loi" << endl;
		}
		else
		{
			//cout << "Gui thanh cong greeting " << greeting << endl;
		}
		//Tao rieng mot thread thuc hien giao tiep voi client

		Param* p = new Param(index, this);
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)(CommunicateWithOneClientHandler), (LPVOID)(p), NULL, NULL);
	}
}




bool Server::GetPacketType(int ID, Packet & _packettype)
{
	int packettype;
	if (!Getint32_t(ID, packettype)) //Try to receive packet type... If packet type fails to be recv'd
		return false; //Return false: packet type not successfully received
	_packettype = (Packet)packettype;
	return true;//Return true if we were successful in retrieving the packet type
}

bool Server::Getint32_t(int ID, int32_t & _int32_t)
{
	if (!recvall(ID, (char*)&_int32_t, sizeof(int32_t))) //Try to receive long (4 byte int)... If int fails to be recv'd
		return false; //Return false: Int not successfully received
	_int32_t = ntohl(_int32_t); //Convert long from Network Byte Order to Host Byte Order
	return true;//Return true if we were successful in retrieving the int
}

bool Server::recvall(int ID, char * data, int totalbytes)
{
	int bytesreceived = 0; //Holds the total bytes received
	while (bytesreceived < totalbytes) //While we still have more bytes to recv
	{
		int RetnCheck = recv(_ConnectionSet[ID], data, totalbytes - bytesreceived, NULL); //Try to recv remaining bytes
		if (RetnCheck == SOCKET_ERROR) //If there is a socket error while trying to recv bytes
			return false; //Return false - failed to recvall
		bytesreceived += RetnCheck; //Add to total bytes received
	}
	return true; //Success!
}

bool Server::ProcessPacket(int ID, Packet _packettype)
{
	switch (_packettype)
	{
	case P_ChatMessage: //Packet Type: chat message
	{
		std::string Message; //string to store our message we received
		if (!GetString(ID, Message)) //Get the chat message and store it in variable: Message
			return false; //If we do not properly get the chat message, return false
						  //Next we need to send the message out to each user
		for (int i = 0; i < MAXCONNECTION; i++)
		{
			if (!_ConnectionMonitor[i] && i != ID)
			{
				if (i == ID) //If connection is the user who sent the message...
					continue;//Skip to the next user since there is no purpose in sending the message back to the user who sent it.
				if (!SendString(i, Message)) //Send message to connection at index i, if message fails to be sent...
				{
					std::cout << "Failed to send message from client ID: " << ID << " to client ID: " << i << std::endl;
				}
			}
			
		}
		std::cout << "Processed chat message packet from user ID: " << ID << std::endl;
		break;
	}

	default: //If packet type is not accounted for
	{
		std::cout << "Unrecognized packet: " << _packettype << std::endl; //Display that packet was not found
		break;
	}
	}
	return true;
}

bool Server::GetString(int ID, std::string & _string)
{
	int32_t bufferlength; //Holds length of the message
	if (!Getint32_t(ID, bufferlength)) //Get length of buffer and store it in variable: bufferlength
		return false; //If get int fails, return false
	char * buffer = new char[bufferlength + 1]; //Allocate buffer
	buffer[bufferlength] = '\0'; //Set last character of buffer to be a null terminator so we aren't printing memory that we shouldn't be looking at
	if (!recvall(ID, buffer, bufferlength)) //receive message and store the message in buffer array. If buffer fails to be received...
	{
		delete[] buffer; //delete buffer to prevent memory leak
		return false; //return false: Fails to receive string buffer
	}
	_string = buffer; //set string to received buffer message
	delete[] buffer; //Deallocate buffer memory (cleanup to prevent memory leak)
	return true;//Return true if we were successful in retrieving the string
}

bool Server::SendString(int ID, std::string & _string)
{
	if (!SendPacketType(ID, P_ChatMessage)) //Send packet type: Chat Message, If sending packet type fails...
		return false; //Return false: Failed to send string
	int32_t bufferlength = _string.size(); //Find string buffer length
	if (!Sendint32_t(ID, bufferlength)) //Send length of string buffer, If sending buffer length fails...
		return false; //Return false: Failed to send string buffer length
	if (!sendall(ID, (char*)_string.c_str(), bufferlength)) //Try to send string buffer... If buffer fails to send,
		return false; //Return false: Failed to send string buffer
	return true; //Return true: string successfully sent
}

bool Server::SendPacketType(int ID, Packet _packettype)
{
	if (!Sendint32_t(ID, _packettype)) //Try to send packet type... If packet type fails to send
		return false; //Return false: packet type not successfully sent
	return true; //Return true: packet type successfully sent
}

bool Server::Sendint32_t(int ID, int32_t _int32_t)
{
	_int32_t = htonl(_int32_t); //Convert long from Host Byte Order to Network Byte Order
	if (!sendall(ID, (char*)&_int32_t, sizeof(int32_t))) //Try to send long (4 byte int)... If int fails to send
		return false; //Return false: int not successfully sent
	return true; //Return true: int successfully sent
}

bool Server::sendall(int ID, char * data, int totalbytes)
{
	int bytessent = 0; //Holds the total bytes sent
	while (bytessent < totalbytes) //While we still have more bytes to send
	{
		int RetnCheck = send(_ConnectionSet[ID], data + bytessent, totalbytes - bytessent, NULL); //Try to send remaining bytes
		if (RetnCheck == SOCKET_ERROR) //If there is a socket error while trying to send bytes
			return false; //Return false - failed to sendall
		bytessent += RetnCheck; //Add to total bytes sent
	}
	return true; //Success!
}

Server::~Server()
{
}


int Server::findIndexOfAvailableSocket()
{
	for (int i = 0; i < MAXCONNECTION; i++)
	{
		if (_ConnectionMonitor[i])
		{
			return i;
		}
	}
	return -1;
}


int main()
{
	Server * server = Server::getServerInstance();
	server->Initiate();
	server->WakeUp();
	server->ProcessRequest();
	return 1;
}
