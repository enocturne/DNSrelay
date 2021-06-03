#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>

#define TRANSLATEPAHT "dnsrelay.txt"
#define ENTRYCOUNT 1000
#define NANAME "0.0.0.0"
#define IDCOUNT 1000
#define BUFSIZE 512
#define QSIZE 2000

void domainConvert(char *qname, char *res);
typedef unsigned char BUF[BUFSIZE];
typedef unsigned short octects;
enum STATUS
{
    EMPTY = 0,
    WAIT = 1,
    RELAY = 2,
    READY = 3,
};

struct entry
{
    char ip[20];
    char domain[100];
};

typedef struct entry Entry;

struct idevent
{
    BUF message;
    enum STATUS status; //wait,relay
    int entry;
    int iRecv;
    SOCKADDR_IN client;
};
typedef struct idevent IDevent;

struct system
{

    IDevent *eventQueue[QSIZE];
    int idlb, idub;
    Entry entryTable[1000];
};

typedef struct system System;

typedef struct header Header;

struct question
{
    octects QType;
    octects QClass;
};
typedef struct question Question;
struct resourcerecord
{
    octects name;
    octects type;
    octects class;
    unsigned int ttl;
    octects RDlength;
    unsigned int Rdata;
};

typedef struct resourcerecord RR;

void entryInit(Entry *entryTable)
{
    FILE *fp = fopen(TRANSLATEPAHT, "r");
    //Entry *entrylist;
    for (int i = 0; !feof(fp); i++)
    {
        Entry item;
        fscanf(fp, "%s", item.ip);
        fscanf(fp, "%s", item.domain);

        if (strlen(item.domain) > 64)
        {
            //printf("???%d", strlen(item.domain));
            //printf("%s::%s\n", item.ip, item.domain);
        }
        entryTable[i] = item;
        //printf("\n%s\n", item.domain);
        //printf("%d----%s\n",i, item.ip);
    }
    fclose(fp);
}

int entrySearch(char *domain, Entry *entryTable)
{
    for (int i = 0; i < ENTRYCOUNT; i++)
    {
        int res = strncmp(domain, entryTable[i].domain, strlen(domain));
        //printf("\n%s\n", entryTable[i].domain);
        //printf("%s\n",domain);
        //printf("%d %d\n", i,strlen(domain));
        if (res == 0)
        {
            if(strncmp("0.0.0.0",entryTable[i].ip,7)==0)
            {
                return -2;
            }
            return i;
        }
    }
    return -1;
}

/**
 * 对队列中处于等待的事件进行解析：
 *  解析成功的需要修改问答类型为R，状态标记为READY，等待构造响应报文发送
 *  解析失败的需要发送给远端服务器，状态标记为RELAY，等待直接发送
 */

void IDConvert(System *system, IDevent *event)
{
    char *qname = &event->message[12];
    char res[100];
    domainConvert(qname, res);
    int num = entrySearch(res, system->entryTable);
    if (num >= 0)
    {
        event->status = READY;
        event->entry = num;
        printf("deal result : READY\n");

        //system->eventQueue[system->idub] = event;
        /*
        for (int i = 0; i < QSIZE; i++)
        {
            if (system->eventQueue[(system->idub + i) % QSIZE]->status == EMPTY)
            {
                system->idub = (system->idub + i) % QSIZE;
                printf("idub change to %d", system->eventQueue[system->idub]);
                break;
            }
        }
        */
        octects a = htons(0x8180); //1000 0001 1000 0000
        memcpy(&event->message[2], &a, sizeof(octects));
    }
    else if(num == -1)
    {
        event->status = RELAY;
        event->entry = num;

        printf("deal result : RELAY\n");
        unsigned char *id = event->message;
        (*((octects *)id))++;
    }
    else if(num == -2)
    {
        event->status = EMPTY; //memcpy(&event->message[2],)
    }
}

void domainConvert(char *qname, char *res)
{
    //char res[100];
    memset(res, 1, (strlen(qname) + 1) * sizeof(char));
    for (int i = 0; qname[i] != 0; i++)
    {
        int j = i + 1;
        int ub = (unsigned char)qname[i] + i;
        for (; j <= ub; j++, i++)
        {
            res[i] = qname[j];
            //printf(" %x ", res[i]);
            memcpy(res + i, qname + j, 1);
        }
        if (qname[j] != 0)
            res[i] = '.';
        else
        {
            res[i] = 0;
            break;
        }
    }
    printf("\ndomian:%s\n", res);
}

/**
 * 对在解析表中找到的请求，构造响应报文
 */
void makeResponse(System *system, IDevent *event, int iRecv)
{
    char response[16];
    octects Name = htons(0xc00c);
    memcpy(response, &Name, sizeof(octects));

    octects Type = htons(0x0001);
    memcpy(response + 2, &Type, sizeof(octects));

    octects Class = htons(0x0001);
    memcpy(response + 4, &Class, sizeof(octects));

    unsigned long TTL = htonl(0x7b);
    memcpy(response + 6, &TTL, sizeof(unsigned long));
    //TODO 修改保存时间

    octects IPLen = htons(0x0004);
    memcpy(response + 10, &IPLen, sizeof(octects));

    unsigned long IP = 0;
    IP = (unsigned long)inet_addr(system->entryTable[event->entry].ip);
    //IP = (unsigned long)inet_addr(IP);
    //TODO IP-->octets
    octects answer = 0;
    answer = htons(0x0001);
    memcpy(&event->message[6], &answer, sizeof(octects));
    memcpy(response + 12, &IP, sizeof(unsigned long));
    memcpy(event->message + iRecv, response, 16);
    event->iRecv += 16;
}

int popQueue(System* system, IDevent* event)
{
    unsigned char* qr = &event->message[2];
    if (*qr == 0x01)
    {

        unsigned char* type = &event->message[12];
        while (*type++ != 0)
        {
            if (*type <= 20)
            {
                printf(".");
            }
            else
            {
                printf("%c", *type);
            }
        }
        printf("\n");
        unsigned typevalue = ntohs(*(octects*)type);
        // >> 8;
        if (typevalue == 1)
        {
            if (event->status == EMPTY)
            {
                event->status = WAIT;
                return 1;
            }
            else
            {
                printf("unexpected event:%d!!!\n", event->status);
                return 0;
            }
        }
        /*
        else if(typevalue == 28)
        {
            if (event->status == EMPTY)
            {
                event->status = RELAY;
                printf("IPV6 is not supported !!!\n");
                return 1;
            }
            else
            {
                printf("unexpected event:%d!!!\n", event->status);
                return 0;
            }
        }

        else
        {
            printf("unexpected type: %d\n", typevalue);
            return 1;
        }

    }
    */
        else
        {
            printf("invalid message!\n");
            return 0;
        }
    }
}
    //* TODO: 1,接收函数 2，发送函数 3，处理流程
