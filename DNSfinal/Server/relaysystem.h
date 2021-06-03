#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

#define TRANSLATEPAHT "dnsrelay.txt"
#define DEF_DNS_ADDRESS "10.3.9.45" //外部DNS服务器地址
#define DNS_PORT 53                 //进行DNS服务的53端口
#define ENTRYCOUNT 1000
#define IDCOUNT 1000
#define BUFSIZE 512
#define QSIZE 2000
#define IDMAX 2000
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
    int idlb, idub, entrynumber, index;
    Entry entryTable[ENTRYCOUNT];
};

typedef struct system System;

typedef struct resourcerecord RR;

void entryInit(System *system);

int entrySearch(char *domain, Entry *entryTable);

void IDConvert(System *system, IDevent *event);

void domainConvert(char *qname, char *res);

void makeResponse(System *system, IDevent *event, int iRecv);

int popQueue(System *system, IDevent *event);

void update(char *ip, IDevent *event, System *sys);

void block(IDevent *event);
