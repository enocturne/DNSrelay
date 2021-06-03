#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "relaysystem.h"
#include <math.h>
#include <stdio.h>
//#pragma comment(lib, "Ws2_32.lib")

#define DEF_DNS_ADDRESS "192.168.146.2" //外部DNS服务器地址
#define LOCAL_ADDRESS "127.0.0.1"       //本地DNS服务器地址
#define DNS_PORT 53                     //进行DNS服务的53端口

int main()
{
    System DNScontrol;
    System *sys_ptr = &DNScontrol;

    entryInit(DNScontrol.entryTable);
    char domain[64] = "www.baidu.com";
    int res = entrySearch(domain,sys_ptr->entryTable);
    printf("%d\n", res);
    return 1;


    for (int i = 0; i < IDCOUNT; i++)
    {
        //printf("%d:%s\n", i,sys_ptr->entryTable[i].domain);
    }
    sys_ptr->idlb = 0;
    sys_ptr->idub = 1;

    for (int i = 0; i < QSIZE; i++)
    {
        IDevent tmp;
        memset(tmp.message, 0, BUFSIZE * sizeof(char));
        sys_ptr->eventQueue[i] = &tmp;
        sys_ptr->eventQueue[i]->status = EMPTY;   
    }
    
    IDevent recvEvent;
    IDevent *recvEvent_ptr = &recvEvent;

    memset(&(recvEvent_ptr->client), 0, sizeof(SOCKADDR_IN));
    memset(recvEvent_ptr->message, 0, BUFSIZE * sizeof(char));
    recvEvent.status = WAIT;
    int recvNum;
    

    /*
0000   00 74 9c 7d fa db a4 c3 f0 70 1c 82 08 00 45 00
0010   00 3b cd 80 00 00 40 11 09 47 0a 15 86 a6 0a 03
0020   09 2d da 5b 00 35 00 27 30 2e !!!! 79 f5 01 00 00 01
0030   00 00 00 00 00 00 03 77 77 77 05 62 61 69 64 75
0040   03 63 6f 6d 00 00 1c 00 01

    */
    
    FILE *fp = fopen("query1","rb");
    int irecv = 0;
    for (; irecv < BUFSIZE && !feof(fp); irecv++)
    {
        fread(&recvEvent_ptr->message[irecv], sizeof(char), 1, fp);
    }
    fclose(fp);
    
    for (int i = 0; i<irecv;i++)
    {
        printf("%x ", recvEvent_ptr->message[i]);
    }


    //printf("%s",domainConvert(&recvEvent.message[12]));

    IDConvert(sys_ptr, recvEvent_ptr);
    printf("\nidub:%d\n", sys_ptr->idub);
    printf("entry:%d\n", recvEvent_ptr->entry);
    for (int i = 0; i < 47; i++)
    {
        //printf("%d-%x__",i,sys_ptr->eventQueue[1]->message[i]);
    }
    makeResponse(sys_ptr,sys_ptr->eventQueue[1],31);
    for (int i = 0;i<47;i++)
    {
        printf("%x ", sys_ptr->eventQueue[1]->message[i]);
    }
    /*
    while (1)
    {
        


        popQueue(sys_ptr, recvEvent_ptr);

        /**
         * 对于队列中的等待事件进行处理，处理完成后根据是否成功解析，标记为待发送READY和待中继RELAY两种状态
         */
    /*
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
    */
}