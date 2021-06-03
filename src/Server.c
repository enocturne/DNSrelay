#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "relaysystem.h"
#include <math.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEF_DNS_ADDRESS "10.3.9.45" //外部DNS服务器地址
#define LOCAL_ADDRESS "127.0.0.1"   //本地DNS服务器地址
#define DNS_PORT 53                 //进行DNS服务的53端口

int main()
{
    System DNScontrol;
    System *sys_ptr = &DNScontrol;

    WSADATA wsaData;
    SOCKET socketServer, socketLocal;              //本地DNS和外部DNS两个套接字
    SOCKADDR_IN serverName, clientName, localName; //本地DNS、外部DNS和请求端三个网络套接字地址
    entryInit(DNScontrol.entryTable);
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sys_ptr->idlb = 0;
    sys_ptr->idub = 1;

    for (int i = 0; i < QSIZE; i++)
    {
        IDevent tmp;
        IDevent *tmp_ptr = &tmp;
        sys_ptr->eventQueue[i] = tmp_ptr;
        sys_ptr->eventQueue[i]->status = EMPTY;
        memset(&sys_ptr->eventQueue[i]->client, 0, sizeof(sys_ptr->eventQueue[i]->client));
    }

    //recvEvent.status = WAIT;
    //int iRecv;
    char outerDns[16] = DEF_DNS_ADDRESS;

    //创建本地DNS和外部DNS套接字
    socketServer = socket(AF_INET, SOCK_DGRAM, 0);
    socketLocal = socket(AF_INET, SOCK_DGRAM, 0);

    //设置本地DNS和外部DNS两个套接字
    localName.sin_family = AF_INET;
    localName.sin_port = htons(DNS_PORT);
    localName.sin_addr.s_addr = htonl(ADDR_ANY);

    serverName.sin_family = AF_INET;
    serverName.sin_port = htons(DNS_PORT);
    serverName.sin_addr.s_addr = inet_addr(outerDns);

    unsigned value = 1;
    setsockopt(socketLocal, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

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

        IDevent *recvEvent_ptr = (IDevent *)malloc(sizeof(IDevent));

        if (sys_ptr->eventQueue[sys_ptr->idub]->status != EMPTY)
        {

            for (int i = 1; i < QSIZE; i++)
            {
                if (sys_ptr->eventQueue[(sys_ptr->idub + i) % QSIZE]->status == EMPTY)
                {
                    sys_ptr->idub = (sys_ptr->idub + i) % QSIZE;
                    printf("\nnew event in %d\n", sys_ptr->idub);
                    break;
                }
            }
        }

        memset(&(recvEvent_ptr->client), 0, sizeof(SOCKADDR_IN));
        memset(recvEvent_ptr->message, 0, BUFSIZE * sizeof(char));
        
        recvEvent_ptr->iRecv = recvfrom(socketLocal, recvEvent_ptr->message, BUFSIZ * sizeof(char), 0, (SOCKADDR *)&(recvEvent_ptr->client), &clien_Len);
        sys_ptr->eventQueue[sys_ptr->idub] = recvEvent_ptr;
        recvEvent_ptr->status = EMPTY;
        
        if (recvEvent_ptr->iRecv == SOCKET_ERROR)
        {
            printf("Recvfrom Failed: %d\n", WSAGetLastError());
            recvEvent_ptr->status = EMPTY;
            continue;
        }
        else if (recvEvent_ptr->iRecv == 0)
        {
            printf("recV none\n");
            recvEvent_ptr->status = EMPTY;
            continue;
        }
        else
        {
            printf("recv:%d\n", recvEvent_ptr->iRecv);

            for (int i = 0; i < recvEvent_ptr->iRecv; i++)
            {
                printf("%x ", recvEvent_ptr->message[i]);
            }
            printf("\n");
            if (popQueue(sys_ptr, recvEvent_ptr) == 0)
            {
                continue;
            }

            recvEvent_ptr->status = WAIT;
        }

        /**
         * 对于队列中的等待事件进行处理，处理完成后根据是否成功解析，标记为待发送READY和待中继RELAY两种状态
         */
        for (int i = (sys_ptr->idlb + 1) % QSIZE; i != (sys_ptr->idub + 1) % QSIZE; i = (i + 1) % QSIZE)
        {

            printf("----------------------------\ndeal with event %d.status:%d", i, sys_ptr->eventQueue[i]->status);
            switch (sys_ptr->eventQueue[i]->status)
            {
            case EMPTY:
                break;
            case WAIT:
            {
                IDConvert(sys_ptr, sys_ptr->eventQueue[i]);
                if (sys_ptr->eventQueue[i]->status == READY)
                {
                    printf("find! entry number is---%d----\n", sys_ptr->eventQueue[i]->entry);
                }
            }
            break;
            case READY:
            {
                makeResponse(sys_ptr, sys_ptr->eventQueue[i], sys_ptr->eventQueue[i]->iRecv);
                //TODO 发送长度
                int iSend = sendto(socketLocal, sys_ptr->eventQueue[i]->message, BUFSIZE, 0, (SOCKADDR *)&sys_ptr->eventQueue[i]->client, sizeof(sys_ptr->eventQueue[i]->client));
                if (iSend == SOCKET_ERROR)
                {
                    printf("sendto Failed:%d\n", WSAGetLastError());
                }
                else if (iSend == 0)
                {
                    printf("\nsend none\n");
                }
                else
                {
                    sys_ptr->eventQueue[i]->status = EMPTY;
                    printf("send %d succeess\n", iSend);
                }
            }
            break;
            case RELAY:
            {
                printf("\nRelaying\n");
                unsigned char *id = sys_ptr->eventQueue[i]->message;
                (*((octects *)id))++;

                int iSend = sendto(socketServer, sys_ptr->eventQueue[i]->message, sys_ptr->eventQueue[i]->iRecv, 0, (SOCKADDR *)&serverName, sizeof(serverName));
                printf("relay send %d:\n", iSend);
                if (iSend == SOCKET_ERROR)
                {
                    printf("sendto Failed: %d\n", WSAGetLastError());
                }
                else if (iSend == 0)
                {
                    printf("\nrelaysend none\n");
                }

                //接收来自外部DNS服务器的响应报文
                IDevent relayEvent;
                relayEvent.entry = sys_ptr->eventQueue[i]->entry;
                relayEvent.client = sys_ptr->eventQueue[i]->client;
                relayEvent.status = RELAY;
                IDevent *relayEvent_ptr = &relayEvent;

                relayEvent.iRecv = recvfrom(socketServer, relayEvent_ptr->message, BUFSIZE * sizeof(char), 0, (SOCKADDR *)&relayEvent_ptr->client, &clien_Len);
                printf("relay from:%d\n", relayEvent.iRecv);
                if (relayEvent.iRecv <= 0)
                {
                    printf("\nrecvfrom() Error #%d\n", WSAGetLastError());
                    break;
                }
                unsigned char *idrelay = relayEvent_ptr->message;
                (*((octects *)idrelay))--;

                char *new_ptr = &relayEvent_ptr->message[12];
                char newdomain[80];
                int newindex = 0;
                while (*new_ptr++ != 0)
                {
                    if (*new_ptr > 0 && *new_ptr <= 20)
                    {
                        newdomain[newindex++] = '.';
                    }
                    else
                    {
                        newdomain[newindex++] = *new_ptr;
                    }
                }
                printf("%s\n", newdomain);
                new_ptr += 6;
                if(*((octects*)new_ptr)==1)
                {
                    new_ptr += 10;
                    unsigned char newip[30];
                    for (int i = 0; i < 4; i++)
                    {
                        newip[i] = *(new_ptr + i);
                        printf("--%d--", newip[i]);
                    }
                    printf("\n");
                }
                else
                {
                    printf("unexpectd relay type\n");
                }
                memcpy(sys_ptr->eventQueue[i]->message, relayEvent_ptr->message, relayEvent_ptr->iRecv);
                unsigned char *idsend = sys_ptr->eventQueue[i]->message;
                (*((octects *)idsend))--;
                int relaySend = sendto(socketLocal, sys_ptr->eventQueue[i]->message, relayEvent.iRecv, 0, (SOCKADDR *)&sys_ptr->eventQueue[i]->client, sizeof(sys_ptr->eventQueue[i]->client));
                if (iSend == SOCKET_ERROR)
                {
                    printf("sendto Failed: %d\n", WSAGetLastError());
                }
                else if (iSend == 0)
                {
                    printf("\nnone relaysend\n");
                }
                else
                {
                    printf("relay success\n");
                    IDevent tmp;
                    IDevent *tmp_ptr = &tmp;
                    sys_ptr->eventQueue[i] = tmp_ptr;
                    sys_ptr->eventQueue[i]->status = EMPTY;
                    memset(&sys_ptr->eventQueue[i]->client, 0, sizeof(sys_ptr->eventQueue[i]->client));
                }
            }
            break;
            default:
                break;
            }
            while (1)
            {
                if (sys_ptr->eventQueue[(sys_ptr->idlb + 1) % QSIZE]->status == EMPTY && (sys_ptr->idlb + 1) % QSIZE != sys_ptr->idub)
                {
                    sys_ptr->idlb = (sys_ptr->idlb + 1) % QSIZE;
                    printf("complete event %d\n", sys_ptr->idlb);
                }
                else
                {
                    printf("event %d is blocking\n", (sys_ptr->idlb + 1) % QSIZE);
                    break;
                }
            }
        }
    }
}