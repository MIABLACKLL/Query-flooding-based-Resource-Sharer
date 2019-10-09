#include<iostream>
#include<cstring>
#include <Ws2tcpip.h>
#include<WinSock2.h>
#pragma comment(lib,"Ws2_32.lib")

constexpr auto BUF_SIZE = 128;

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	SOCKET ServerSock = socket(AF_INET, SOCK_STREAM, 0);
	
	sockaddr_in SockAddr;
	memset(&SockAddr, 0, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;
	InetPton(AF_INET,"127.0.0.1",&SockAddr.sin_addr);
	SockAddr.sin_port = htons(9999);
	bind(ServerSock, reinterpret_cast<sockaddr*>(&SockAddr), sizeof(SockAddr));

	listen(ServerSock, 9999);

	SOCKADDR ClientAddr;
	int SockAddrSize = sizeof SOCKADDR;
	SOCKET ClientSock = accept(ServerSock, reinterpret_cast<sockaddr*>(&ClientAddr), &SockAddrSize);
	std::string Buffer;
	int StrLen = recv(ClientSock,const_cast<char *>(Buffer.c_str()), BUF_SIZE, 0);
	std::cout << Buffer.c_str();
	send(ClientSock, const_cast<char *>(Buffer.c_str()), StrLen, 0);

	closesocket(ClientSock);
	closesocket(ServerSock);

	WSACleanup();

	system("pause");



}