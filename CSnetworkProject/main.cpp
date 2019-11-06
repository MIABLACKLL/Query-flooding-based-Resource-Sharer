#pragma once
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
#include"Transfer.h"
#include"SystemCUI.h"
constexpr auto BUF_SIZE = 128;

#include <Ws2tcpip.h>
#include<WinSock2.h>
#pragma comment(lib,"Ws2_32.lib")

void initiazer(std::promise<int> &promiseObj) {
	std::cout << "Inside thread: " << std::this_thread::get_id() << std::endl;
	//std::this_thread::sleep_for(std::chrono::seconds(2));
	promiseObj.set_value_at_thread_exit(35);
}

void recv()
{

}

int main() {
	//CConfig config;

	//config.loadConfigFile();

	//CFileManagement filem;

	//std::cout << config.getSelfPeerID() << std::endl;
	//std::cout << config.getIP() << std::endl;
	//std::cout << config.getDataPort() << std::endl;
	//std::cout << config.getCommandPort() << std::endl;

	//CQueryFlooding *test1=new CQueryFlooding(&config,&filem);
	//CTransfer *transfer=new CTransfer(&config, &filem);


	//std::cout << test1->listenCommandPort() << std::endl;
	//std::cout << transfer->listenDataPort() << std::endl;
	//std::promise<SResultPacket> promise;
	//std::future<SResultPacket> future = promise.get_future();
	//std::thread t1(&CQueryFlooding::receiveBuffer,test1, std::ref(promise));
	//std::thread t2(&CTransfer::receiveDownloadRequest, transfer);
	//test1->requestQuery("test");
	//SResultPacket result = future.get();
	//t1.join();
	//std::cout << sizeof SQueryPacket << " " << sizeof SResultPacket << std::endl;
	//std::cout << result.File.FilePath << std::endl;
	//std::cout << result.RecvIP << std::endl;
	//SRequestDownloadPacket rp;
	//strcpy_s(rp.FileName,result.File.FileName);
	//rp.IsDir = result.File.IsDir;
	//transfer->sendDownloadRequest(rp, result.RecvIP, result.RecvDataPort);
	//system("pause");
	CSystemCUI cui;
	cui.runSystem();
	return 0;
}