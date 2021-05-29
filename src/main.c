#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "relaysystem.h"

#include <math.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEF_DNS_ADDRESS "192.168.146.2" //外部DNS服务器地址
#define LOCAL_ADDRESS "127.0.0.1"       //本地DNS服务器地址
#define DNS_PORT 53                     //进行DNS服务的53端口

int main()
{
    System DNScontrol;
    System *sys_ptr = &DNScontrol;

    WSADATA wsaData;
    SOCKET socketServer, socketLocal;              //本地DNS和外部DNS两个套接字
    SOCKADDR_IN serverName, clientName, localName; //本地DNS、外部DNS和请求端三个网络套接字地址
    entryInit(DNScontrol.entryTable);

    sys_ptr->idlb = 1;
    sys_ptr->idub = 0;

    for (int i = 0; i < QSIZE; i++)
    {
        sys_ptr->eventQueue[i]->status = WAIT;
    }
    IDevent recvEvent;
    IDevent *recvEvent_ptr = &recvEvent;
    memset(&(recvEvent_ptr->client), 0, sizeof(SOCKADDR_IN));
    memset(recvEvent_ptr->message, 0, BUFSIZE * sizeof(char));
    recvEvent.status = WAIT;
    int recvNum;
    char outerDns[16] = DEF_DNS_ADDRESS;

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    //创建本地DNS和外部DNS套接字
    socketServer = socket(AF_INET, SOCK_DGRAM, 0);
    socketLocal = socket(AF_INET, SOCK_DGRAM, 0);

    //设置本地DNS和外部DNS两个套接字
    localName.sin_family = AF_INET;
    localName.sin_port = htons(DNS_PORT);
    localName.sin_addr.s_addr = inet_addr(LOCAL_ADDRESS);

    serverName.sin_family = AF_INET;
    serverName.sin_port = htons(DNS_PORT);
    serverName.sin_addr.s_addr = inet_addr(outerDns);
    if (bind(socketLocal, (SOCKADDR *)&localName, sizeof(localName)))
    {
        printf("Binding Port 53 failed.\n");
        exit(1);
    }
    else
    {
        printf("Binding Port 53 succeed.\n");
    }

    int clien_Len = sizeof(clientName);
    while (1)
    {
        recvNum = recvfrom(socketLocal, recvEvent.message, BUFSIZ * sizeof(char), 0, (SOCKADDR *)&(recvEvent_ptr->client), &clien_Len);

        if (recvNum == SOCKET_ERROR)
        {
            printf("Recvfrom Failed: %d", WSAGetLastError());
            continue;
        }
        else if (recvNum == 0)
        {
            break;
        }
        else
        {
            popQueue(sys_ptr, recvEvent_ptr);
        }

        /**
         * 对于队列中的等待事件进行处理，处理完成后根据是否成功解析，标记为待发送READY和待中继RELAY两种状态
         */
        for (int i = sys_ptr->idlb; i != sys_ptr->idub; i = (i + 1) % QSIZE)
        {
            switch (sys_ptr->eventQueue[i]->status)
            {
            case WAIT:
            {
                IDConvert(sys_ptr, sys_ptr->eventQueue[i]);
            }
            break;
            case READY:
            {
                makeResponse(sys_ptr, recvEvent_ptr);
                int iSend = sendto(socketLocal, sys_ptr->eventQueue[i]->message, strlen(sys_ptr->eventQueue[i]->message), 0, (SOCKADDR *)&sys_ptr->eventQueue[i]->client, sizeof(sys_ptr->eventQueue[i]->client));
                if (iSend == SOCKET_ERROR)
                {
                    printf("sendto Failed:%d\n", WSAGetLastError());
                    //continue;
                }
                else if (iSend == 0)
                    ; //break;
            }
            break;
            case RELAY:
            {
                unsigned char *id = sys_ptr->eventQueue[i]->message;
                (*((octects *)id))++;
                int iSend = sendto(socketServer, sys_ptr->eventQueue[i], strlen(sys_ptr->eventQueue[i]->message), 0, (SOCKADDR *)&serverName, sizeof(serverName));
                if (iSend == SOCKET_ERROR)
                {
                    printf("sendto Failed: %d\n",WSAGetLastError());
                    //continue;
                }
                else if (iSend == 0)
                    ; //break;

                //free(pID); //释放动态分配的内存

                //接收来自外部DNS服务器的响应报文
                IDevent relayEvent;
                relayEvent.entry = sys_ptr->eventQueue[i]->entry;
                relayEvent.client = sys_ptr->eventQueue[i]->client;
                relayEvent.status = RELAY;
                IDevent *relayEvent_ptr = &relayEvent;
                int iRecv = recvfrom(socketServer, relayEvent_ptr, BUFSIZE *sizeof(char), 0, (SOCKADDR *)&relayEvent_ptr->client, sizeof(relayEvent_ptr->client));
                unsigned char *idrelay = relayEvent_ptr->message;
                (*((octects *)idrelay))--;
                //*TODO 确定DNS请求者的socket
                memcpy(sys_ptr->eventQueue[i]->message, relayEvent_ptr->message, strlen(relayEvent_ptr->message));
                unsigned char *idsend = sys_ptr->eventQueue[i]->message;
                (*((octects *)idsend))--;
                int relaySend;
                int relaySend = sendto(socketLocal, sys_ptr->eventQueue[i]->message, strlen(sys_ptr->eventQueue[i]->message), 0, (SOCKADDR *)&sys_ptr->eventQueue[i]->client, sizeof(sys_ptr->eventQueue[i]->client));
                if (iSend == SOCKET_ERROR)
                {
                    printf("sendto Failed: %d", WSAGetLastError());
                }
                else if (iSend == 0)
                    ;//break;
            }
                break;
            default:
                break;
            }

        }
    }
}