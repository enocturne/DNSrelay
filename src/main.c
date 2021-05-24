#include "relaysystem.h"
#include <winsock2.h>
#include <windows.h>
#include <math.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEF_DNS_ADDRESS "192.168.146.2" //外部DNS服务器地址
#define LOCAL_ADDRESS "127.0.0.1" //本地DNS服务器地址
#define DNS_PORT 53               //进行DNS服务的53端口

int main()
{
    System DNScontrol;
    WSADATA wsaData;
    SOCKET socketServer, socketLocal;              //本地DNS和外部DNS两个套接字
    SOCKADDR_IN serverName, clientName, localName; //本地DNS、外部DNS和请求端三个网络套接字地址

}