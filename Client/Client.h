/*
Author: nguyen viet tien
github: nguyenviettien13
gmail: tiennv000@gmail.com
*/


#pragma once
#include <WinSock2.h>
#include <string>
#include <iostream>


using namespace std;

UINT32		IPSERVER = 0xc0a80003;
int			PORT = 27015;


enum Packet {
	P_ChatMessage,
	P_test
};

class Client
{
private:
	static Client*					_clientpointer;
	SOCKET							_socket;

	/*Constructor*/					Client() {};
	/*Copy constructor*/			Client(const Client & Other) {};
	/*Assignment operator*/			Client&  operator = (const Client & other) {};

public:
	/*Initiate socket client*/
	static Client *					getInstance();
	bool							WakeUpSocket();
	bool							Connect();

	/*Send messenger*/
	bool							sendString(string & message);
	bool							SendPacketType(Packet _packettype);
	bool							Sendint32_t(int32_t _int32_t);
	bool							sendall(char * data, int totalbytes);
	bool							GetPacketType(Packet & _packettype);
	bool							Getint32_t(int32_t & _int32_t);
	bool							recvall(char * data, int totalbytes);
	bool							ProcessPacket(Packet _packettype);
	bool							GetString(std::string & _string);
	bool							CloseConnection();
	/*receiver messenger*/
	bool							receive();
	friend bool						receiverHandler(Client * pointer);
	~Client();
};

