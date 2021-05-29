#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEF_DNS_ADDRESS "192.168.146.2" //外部DNS服务器地址
#define LOCAL_ADDRESS "127.0.0.1"       //本地DNS服务器地址
#define DNS_PORT 53                     //进行DNS服务的53端口

int main()
{
    WSADATA wsaData;
    char str[3]="ab";
    printf("%x\n", *(unsigned short *)str);
    *(unsigned short *)str = *(unsigned short *)str +1;
    printf("%x\n", str[0]);
    printf("%x\n", str[1]);
    
}