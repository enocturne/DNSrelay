#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")  //加载 ws2_32.dll
#define BUF_SIZE 200




int main() {

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    //服务器地址信息
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));  //每个字节都用0填充
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(9527);
    //11 0101
     
    //不断获取用户输入并发送给服务器，然后接受服务器数据
    struct sockaddr fromAddr;
    int addrLen = sizeof(fromAddr);
    while (1) {
        unsigned char buffer[BUF_SIZE] = { 0 };
        printf("Input a string: ");
        gets(buffer);
        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*)&servAddr, sizeof(servAddr));
        int iRecv = recvfrom(sock, buffer, BUF_SIZE, 0, &fromAddr, &addrLen);
        buffer[iRecv] = 0;
        //printf("Message form server: %s\n", buffer);

        if (iRecv > 4)
        {
            printf("%s\n", buffer);
        }
        else
        {
            for (int i = 0; i < iRecv; i++)
            {
                printf("%d", buffer[i]);
                if (i != iRecv - 1)
                {
                    printf(".");
                }
            }
            printf("\n");
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}