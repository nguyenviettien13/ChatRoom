#pragma once
#pragma comment(lib, "Ws2_32.lib")


#include <WinSock2.h>
#include <iostream>
#include <string>

int		PORT = 27015;
int		MAXCONNECTION = 100;


using namespace std;

enum Packet {
	P_ChatMessage,
	P_test
};


class Server
{
private:
	static Server*					_serverPointer;
	int								_connectionCounter;
	SOCKET							_listenSocket;
	/*Constructor*/					Server();
	/*Copy Constructor*/			Server(const Server& other) {};
	/*Assignment Operator*/			Server& operator =(const Server& other) {};
public:
	SOCKET * _ConnectionSet;
	bool*							_ConnectionMonitor;
	class Param 
	{
	public:
		int _index;
		Server* _pointer;
		Param(int & index, Server* pointer) :
			_index(index),
			_pointer(pointer)
		{

		}
	};
	/*Initiate Socket*/
	static Server *					getServerInstance();
	bool							Initiate();
	bool							WakeUp();

	/**/
	void							ProcessRequest();
	friend void						CommunicateWithOneClientHandler(Param *p);
	bool							GetPacketType(int ID, Packet & _packettype);
	bool							Getint32_t(int ID, int32_t & _int32_t);
	bool							recvall(int ID, char * data, int totalbytes);
	bool							ProcessPacket(int ID, Packet _packettype);
	bool							GetString(int ID, std::string & _string);
	bool							SendString(int ID, std::string & _string);
	bool							SendPacketType(int ID, Packet _packettype);
	bool							Sendint32_t(int ID, int32_t _int32_t);
	bool							sendall(int ID, char * data, int totalbytes);
	/*Destructor*/					~Server();
	
protected:
	int								findIndexOfAvailableSocket();
};

