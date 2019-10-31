#include<iostream>
#include<cstring>
#include<filesystem>
#include<thread>
#include<future>
#include <Ws2tcpip.h>
#include<WinSock2.h>
#include<fstream>
#pragma comment(lib,"Ws2_32.lib")
#include <chrono>
#include"FileManagement.h"
#include"Config.h"
#include"QueryFlooding.h"
constexpr auto BUF_SIZE = 128;

void initiazer(std::promise<int> &promiseObj) {
	std::cout << "Inside thread: " << std::this_thread::get_id() << std::endl;
	//std::this_thread::sleep_for(std::chrono::seconds(2));
	promiseObj.set_value_at_thread_exit(35);
}
using namespace std;



int main() {
	CConfig configtest;
	if(configtest.loadConfigFile());
	cout << configtest.getSelfPeerID() << endl;
	cout << configtest.getIP() << endl;
	cout << configtest.getDataPort() << endl;
	cout << configtest.getCommandPort() << endl;
	auto peersocket = configtest.getConnectPeerSocket();
	for (auto p : peersocket)
	{
		for (auto m : p)
		{
			try 
			{
				any_cast<int>(m.second);
				cout << m.first << ":" << any_cast<int>(m.second) << endl;
			}
			catch (...)
			{
				cout << m.first << ":" << any_cast<string>(m.second) << endl;
			}
		}
	}
	system("pause");
	return 0;
}