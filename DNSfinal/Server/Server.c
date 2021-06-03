#include "relaysystem.h"

int main()
{
    System DNScontrol;
    System *sys_ptr = &DNScontrol;

    WSADATA wsaData;
    SOCKET socketServer, socketLocal;              //本地DNS和外部DNS两个套接字
    SOCKADDR_IN serverName, clientName, localName; //本地DNS、外部DNS和请求端三个网络套接字地址

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sys_ptr->idlb = 0;
    sys_ptr->idub = 1;
    sys_ptr->index = 0;
    sys_ptr->entrynumber = 0;
    entryInit(sys_ptr);
    for (int i = 0; i < QSIZE; i++)
    {
        IDevent tmp;
        IDevent *tmp_ptr = &tmp;
        sys_ptr->eventQueue[i] = tmp_ptr;
        sys_ptr->eventQueue[i]->status = EMPTY;
        memset(sys_ptr->eventQueue[i]->message, 0, BUFSIZE * sizeof(char));
        memset(&sys_ptr->eventQueue[i]->client, 0, sizeof(sys_ptr->eventQueue[i]->client));
    }

    char outerDns[16] = DEF_DNS_ADDRESS;

    //创建本地DNS和外部DNS套接字
    socketServer = socket(AF_INET, SOCK_DGRAM, 0);
    socketLocal = socket(AF_INET, SOCK_DGRAM, 0);

    //设置本地DNS和外部DNS两个套接字
    localName.sin_family = AF_INET;
    localName.sin_port = htons(DNS_PORT);
    localName.sin_addr.s_addr = htonl(INADDR_ANY);

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

        for (int i = 0; i < recvEvent_ptr->iRecv; i++)
        {
            printf("%x ", recvEvent_ptr->message[i]);
        }
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
            printf("recv:%d in %d\n", recvEvent_ptr->iRecv, sys_ptr->idub);
            if (popQueue(sys_ptr, recvEvent_ptr) == 0)
            {
                continue;
            }
        }

        /**
         * 对于队列中的等待事件进行处理，处理完成后根据是否成功解析，标记为待发送READY和待中继RELAY两种状态
         */
        {
            for (int i = (sys_ptr->idlb + 1) % QSIZE; i != (sys_ptr->idub + 1) % QSIZE;)
            {

                printf("----------------------------\ndeal with event %d.status:%d\n", i, sys_ptr->eventQueue[i]->status);
                switch (sys_ptr->eventQueue[i]->status)
                {
                case EMPTY:
                    continue;
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
                    if (sys_ptr->eventQueue[i]->entry == -2)
                    {
                        block(sys_ptr->eventQueue[i]);
                    }
                    int iSend = sendto(socketLocal, sys_ptr->eventQueue[i]->message, sys_ptr->eventQueue[i]->iRecv, 0, (SOCKADDR *)&sys_ptr->eventQueue[i]->client, sizeof(sys_ptr->eventQueue[i]->client));
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

                    sys_ptr->eventQueue[i]->status = EMPTY;
                    i = (i + 1) % QSIZE;
                }
                break;
                case RELAY:
                {
                    printf("\nRelaying\n");
                    unsigned char *id = sys_ptr->eventQueue[i]->message;
                    (*((octects *)id))++;
                    int iSend = sendto(socketServer, sys_ptr->eventQueue[i]->message, sys_ptr->eventQueue[i]->iRecv, 0, (SOCKADDR *)&serverName, sizeof(serverName));
                    printf("relay send %d of %d\n", iSend, sys_ptr->eventQueue[i]->iRecv);
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

                    //relayEvent.iRecv = recvfrom(socketServer, relayEvent_ptr->message, BUFSIZE * sizeof(char), 0, (SOCKADDR*)&relayEvent_ptr->client, &clien_Len);
                    int nNetTimeout = 2000;
                    if (SOCKET_ERROR == setsockopt(socketServer, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int)))
                    {
                        printf("Set Ser_RecTIMEO error !\r\n");
                    }

                    int ret = recvfrom(socketServer, relayEvent_ptr->message, BUFSIZE * sizeof(char), 0, (SOCKADDR *)&relayEvent_ptr->client, &clien_Len);
                    if (ret < 0)
                    {
                        printf("recv timeout! %d\n", ret);
                        break;
                    }
                    else
                    {
                        relayEvent.iRecv = ret;
                    }
                    printf("relay from:%d\n", relayEvent.iRecv);
                    for (int k = 0; k < relayEvent.iRecv; k++)
                    {
                        printf("%x ", relayEvent_ptr->message[k]);
                    }
                    if (relayEvent.iRecv <= 0)
                    {
                        printf("\nrecvfrom() Error #%d\n", WSAGetLastError());
                        break;
                    }
                    unsigned char *idrelay = relayEvent_ptr->message;
                    (*((octects *)idrelay))--;
                    memcpy(sys_ptr->eventQueue[i]->message, relayEvent_ptr->message, relayEvent_ptr->iRecv);
                    unsigned char *idsend = sys_ptr->eventQueue[i]->message;
                    (*((octects *)idsend))--;
                    char *ip_ptr = &relayEvent_ptr->message[relayEvent_ptr->iRecv - 4];
                    update(ip_ptr, relayEvent_ptr, sys_ptr);
                    int relaySend = sendto(socketLocal, relayEvent_ptr->message, relayEvent_ptr->iRecv, 0, (SOCKADDR *)&sys_ptr->eventQueue[i]->client, sizeof(sys_ptr->eventQueue[i]->client));
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
                        printf("relay success %d\n", relaySend);
                        IDevent tmp;
                        IDevent *tmp_ptr = &tmp;
                        sys_ptr->eventQueue[i] = tmp_ptr;
                        sys_ptr->eventQueue[i]->status = EMPTY;
                        i = (i + 1) % QSIZE;
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
                        break;
                    }
                }
            }
        }
    }
}
