#include<iostream>
#include<string>
#include <Ws2tcpip.h>
#include<WinSock2.h>
#pragma comment(lib,"Ws2_32.lib")

constexpr auto BUF_SIZE = 128;

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	SOCKET ClientSock = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in SockAddr;
	memset(&SockAddr, 0, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;
	InetPton(AF_INET, "127.0.0.1", &SockAddr.sin_addr);
	SockAddr.sin_port = htons(9999);
	connect(ClientSock, reinterpret_cast<sockaddr*>(&SockAddr), sizeof(SockAddr));

	std::string BufferSend;
	std::cin >> BufferSend;
	send(ClientSock, const_cast<char *>(BufferSend.c_str()), BufferSend.size(),0);

	char BufferRev[BUF_SIZE];
	recv(ClientSock,BufferRev,BUF_SIZE,0);
	std::string StrRev = BufferRev;
	std::cout << StrRev.data() << std::endl;

	closesocket(ClientSock);
	WSACleanup();

	system("pause");


}