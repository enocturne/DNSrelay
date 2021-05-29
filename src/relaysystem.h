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

typedef char BUF[BUFSIZE];
typedef unsigned short octects;

struct entry
{
    char ip[20];
    char domain[100];
};

typedef struct entry Entry;

struct idevent
{
    BUF message;
    enum
    {
        WAIT = 0,
        RELAY = 1,
        READY = 2
    } status; //wait,relay
    int entry;
    SOCKADDR_IN client;
};
typedef struct idevent IDevent;

struct system
{

    IDevent *eventQueue[QSIZE];
    //IDevent* eventQueue[QSIZE];
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
    Entry *entrylist;
    for (int i = 0; !feof(fp); i++)
    {
        Entry item;
        fscanf(fp, "%s", item.ip);
        fscanf(fp, "%s", item.domain);
        if (strlen(item.domain) > 64)
        {
            printf("???%d", strlen(item.domain));
            printf("%s::%s\n", item.ip, item.domain);
        }
        entryTable[i] = item;
    }
    fclose(fp);
}

int entrySearch(char *domain, Entry *entryTable)
{
    for (int i = 0; i < ENTRYCOUNT; i++)
    {
        if (strncmp(domain, entryTable[i].domain, strlen(domain)) == 0)
        {
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
    int num = entrySearch(domainConvert(qname), system->entryTable);
    if (num!=-1)
    {
        event->status = READY;
        event->entry = num;
        system->eventQueue[system->idub] = event;
        if (system->eventQueue[(system->idub + 1) % QSIZE] == NULL)
        {
            system->idub = (system->idub + 1) % QSIZE;
        }
        octects a = htons(0x8180); //1000 0001 1000 0000
        memcpy(&event->message[2], &a, sizeof(octects));
    }
    else
    {
        event->status = RELAY;
        unsigned char *id = event->message;
        (*((octects *)id))++;
        system->eventQueue[system->idub] = event;
        if (system->eventQueue[(system->idub + 1) % QSIZE] == NULL)
        {
            system->idub = (system->idub + 1) % QSIZE;
        }
    }
}


char *domainConvert(char *qname)
{
    char res[100];
    for (int i = 0;; i++)
    {
        unsigned char length = *(unsigned char *)res[i];
        int j = i + 1;
        for (; j <= i + length; j++, i++)
        {
            res[i] = qname[j];
        }
        if (qname[j] != 0)
            res[i] = '.';
        else
            break;
    }
    return res;
}


/**
 * 对在解析表中找到的请求，构造响应报文
 */ 
void makeResponse(System *system, IDevent *event)
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

    octects answer = 0;
    unsigned long IP;

    answer = htons(0x0001);
    IP = (unsigned long)inet_addr(system->entryTable[event->entry].ip);
    unsigned long IP = (unsigned long)inet_addr(IP);
    //TODO IP-->octets

    memcpy(&event->message[6], &answer, sizeof(octects));
    memcpy(response + 12, &IP, sizeof(unsigned long));
    memcpy(event->message + strlen(event->message), response, 16);
}

void popQueue(System *system, IDevent *event)
{
    unsigned char *qr = event->message[2];
    if (*qr == 0x01)
    {
        unsigned short *ID = (unsigned short *)event->message;

        for (int i = system->idub;; i = (i + 1) % QSIZE)
        {
            if (system->idlb == system->idub)
            {
                break;
            }
            else if (system->eventQueue[i] == NULL)
            {
                system->idub = i;
                break;
            }
            else
            {
                system->idub = (system->idub + 1) % QSIZE;
            }
        }

        if (system->eventQueue[system->idub] == NULL)
        {
            event->status = WAIT;
            system->eventQueue[system->idub] = event;
        }
        else
        {
            printf("buffer is full\n");
        }
    }
    else
    {
        printf("invalid message!\n");
    }
}
//* TODO: 1,接收函数 2，发送函数 3，处理流程

