#pragma comment(lib, "ws2_32.lib")
#include "Client.h"

Client* Client::_clientpointer = nullptr;


Client * Client::getInstance()
{
	if (Client::_clientpointer == nullptr)
	{
		_clientpointer = new Client();
	}
	return _clientpointer;
}


bool Client::WakeUpSocket()
{
	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup function failed with error: %d\n", iResult);
		exit(0);
	}

	// Create a SOCKET for connecting to server
	_socket = socket(AF_INET, SOCK_STREAM, NULL);
	if (_socket == INVALID_SOCKET) {
		wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		exit(1);
	}
	return true;
}

bool Client::Connect()
{
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.
	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = htonl((ULONG)IPSERVER);
	clientService.sin_port = htons(PORT);

	int iResult = connect(_socket, (SOCKADDR *)& clientService, sizeof(clientService));
	if (iResult == SOCKET_ERROR) {
		wprintf(L"connect function failed with error: %ld\n", WSAGetLastError());
		iResult = closesocket(_socket);
		if (iResult == SOCKET_ERROR)
			wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		getchar();
		exit(1);
	}
	wprintf(L"Connected to server.\n");

	return true;
}

bool Client::sendString(string & message)
{
	if (!SendPacketType(P_ChatMessage)) //Send packet type: Chat Message, If sending packet type fails...
		return false; //Return false: Failed to send string
	int32_t bufferlength = message.size(); //Find string buffer length
	if (!Sendint32_t(bufferlength)) //Send length of string buffer, If sending buffer length fails...
		return false; //Return false: Failed to send string buffer length
	if (!sendall((char*)message.c_str(), bufferlength)) //Try to send string buffer... If buffer fails to send,
		return false; //Return false: Failed to send string buffer
	return true; //Return true: string successfully sent
}

bool Client::SendPacketType(Packet _packettype)
{
	if (!Sendint32_t(_packettype)) //Try to send packet type... If packet type fails to send
		return false; //Return false: packet type not successfully sent
	return true; //Return true: packet type successfully sent
}

bool Client::Sendint32_t(int32_t _int32_t)
{
	_int32_t = htonl(_int32_t); //Convert long from Host Byte Order to Network Byte Order
	if (!sendall((char*)&_int32_t, sizeof(int32_t))) //Try to send int... If int fails to send
		return false; //Return false: int not successfully sent
	return true; //Return true: int successfully sent
}

bool Client::sendall(char * data, int totalbytes)
{
	int bytessent = 0; //Holds the total bytes sent
	while (bytessent < totalbytes) //While we still have more bytes to send
	{
		int RetnCheck = send(_socket, data + bytessent, totalbytes - bytessent, NULL); //Try to send remaining bytes
		if (RetnCheck == SOCKET_ERROR) //If there is a socket error while trying to send bytes
			return false; //Return false - failed to sendall
		bytessent += RetnCheck; //Add to total bytes sent
	}
	return true; //Success!
}

bool Client::GetPacketType(Packet & _packettype)
{
	int packettype;
	if (!Getint32_t(packettype)) //Try to receive packet type... If packet type fails to be recv'd
	{
		//cout << "Error when receiv packettype" << endl;
		return false; //Return false: packet type not successfully received
	}
	_packettype = (Packet)packettype;
	return true;//Return true if we were successful in retrieving the packet type
}

bool Client::Getint32_t(int32_t & _int32_t)
{
	if (!recvall((char*)&_int32_t, sizeof(int32_t))) //Try to receive int... If int fails to be recv'd
		return false; //Return false: Int not successfully received
	_int32_t = ntohl(_int32_t); //Convert long from Network Byte Order to Host Byte Order
	return true;//Return true if we were successful in retrieving the int
}

bool Client::recvall(char * data, int totalbytes)
{
	int bytesreceived = 0; //Holds the total bytes received
	while (bytesreceived < totalbytes) //While we still have more bytes to recv
	{
		int RetnCheck = recv(this->_socket, data + bytesreceived, totalbytes - bytesreceived, NULL); //Try to recv remaining bytes
		if (RetnCheck == SOCKET_ERROR) //If there is a socket error while trying to recv bytes
			return false; //Return false - failed to recvall
		bytesreceived += RetnCheck; //Add to total bytes received
	}
	return true; //Success!
}

bool Client::ProcessPacket(Packet _packettype)
{
	switch (_packettype)
	{
	case P_ChatMessage: //If packet is a chat message packet
	{
		std::string Message; //string to store our message we received
		if (!GetString(Message)) //Get the chat message and store it in variable: Message
			return false; //If we do not properly get the chat message, return false
		std::cout << Message << std::endl; //Display the message to the user
		break;
	}
	default: //If packet type is not accounted for
		std::cout << "Unrecognized packet: " << _packettype << std::endl; //Display that packet was not found
		break;
	}
	return true;
}

bool Client::GetString(std::string & _string)
{
	int32_t bufferlength; //Holds length of the message
	if (!Getint32_t(bufferlength)) //Get length of buffer and store it in variable: bufferlength
		return false; //If get int fails, return false
	char * buffer = new char[bufferlength + 1]; //Allocate buffer
	buffer[bufferlength] = '\0'; //Set last character of buffer to be a null terminator so we aren't printing memory that we shouldn't be looking at
	if (!recvall(buffer, bufferlength)) //receive message and store the message in buffer array. If buffer fails to be received...
	{
		delete[] buffer; //delete buffer to prevent memory leak
		return false; //return false: Fails to receive string buffer
	}
	_string = buffer; //set string to received buffer message
	delete[] buffer; //Deallocate buffer memory (cleanup to prevent memory leak)
	return true;//Return true if we were successful in retrieving the string
}


bool Client::CloseConnection()
{
	if (closesocket(this->_socket) == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAENOTSOCK) //If socket error is that operation is not performed on a socket (This happens when the socket has already been closed)
			return true; //return true since connection has already been closed

		std::string ErrorMessage = "Failed to close the socket. Winsock Error: " + std::to_string(WSAGetLastError()) + ".";
		MessageBoxA(NULL, ErrorMessage.c_str(), "Error", MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}
bool receiverHandler(Client * clientptr)
{
	//Nhan loi chao tu server
	int sizeOfGreeting;
	recv(clientptr->_socket, (char*)&sizeOfGreeting, sizeof(int), NULL);
	cout << "sizeOfGreeting: " << sizeOfGreeting << endl;

	char* greeting = new char[sizeOfGreeting + 1];
	greeting[sizeOfGreeting] = '\0';
	recv(clientptr->_socket, greeting, sizeOfGreeting, NULL);
	cout << greeting << endl;

	Packet PacketType;
	while (true)
	{
		if (!clientptr->GetPacketType(PacketType)) //Get packet type
			break; //If there is an issue getting the packet type, exit this loop
		if (!clientptr->ProcessPacket(PacketType)) //Process packet (packet type)
			break; //If there is an issue processing the packet, exit this loop
	}
	std::cout << "Lost connection to the server." << std::endl;
	if (clientptr->CloseConnection()) //Try to close socket connection..., If connection socket was closed properly
	{
		std::cout << "Socket to the server was closed successfuly." << std::endl;
	}
	else //If connection socket was not closed properly for some reason from our function
	{
		std::cout << "Socket was not able to be closed." << std::endl;
	}
	return false;
}

bool Client::receive()
{
	return CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)receiverHandler, (LPVOID)this, NULL, NULL);
}



Client::~Client()
{
}


int main()
{
	Client * client = Client::getInstance();
	client->WakeUpSocket();
	client->Connect();
	client->receive();
	Sleep(3000);
	while (true)
	{
		Sleep(200);
		string userinput;
		std::getline(std::cin, userinput);
		client->sendString(userinput);
	}
	return 1;
}