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
	ifstream test;
	test.open("D:/C++programs/CSnetworkProject/CSnetworkProject/test.txt");
	
	char file[10];
	file[9] = '\0';
	while (test.read(file, 9))
		cout << file << endl;
	test.seekg(0, std::ios::end);
	int length = test.tellg();
	CFileManagement filem;
	auto target = filem.findFile("test.txt");
	cout << target.first.FileSize << endl;
	cout << length << endl;
	system("pause");
	return 0;
}