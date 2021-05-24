#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
};
typedef struct idevent IDevent;

struct system
{

    IDevent* idQueue[QSIZE];
    IDevent* relayQueue[QSIZE];
    int idlb, idub, relaylb, relayub;
    Entry entryTable[1000];
};

typedef struct system System;

struct header
{
    octects ID;
    octects Flags;
    octects QuestNum;
    octects AnswerNum;
    octects AuthorNum;
    octects AdditionNum;
};

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
            printf("row:%d::%s\n", i, entryTable[i].ip);
            if (strncmp(entryTable[i].ip, "0.0.0.0", 7) != 0)
                return 1;
            else
                return -1;
        }
    }
    return 0;
}

void IDConvert(System* system,IDevent* event)
{
    char *qname = event->message[12];
    char domain[100] = Resolve(qname);
    int num = entrySearch(domain, system->entryTable);
    if (num)
    {
        event->status = READY;
        event->entry = num;
        system->idQueue[system->idub] = event;
        if(system->idQueue[system->idub]==NULL)
        {
            system->idub++;
        }
    }
    else
    {
        event->status = RELAY;
        system->relayQueue[system->relayub] = event;
        if(system->idQueue[system->relayub]==NULL)
        {
            system->relayub++;
        }
    }
}

void IDGenerate()
{
    ;
}

char* Resolve(char *qname)
{
    char res[100];
    for (int i = 0; ;i++)
    {
        unsigned char length = *(unsigned char *)res[i];
        int j = i + 1;
        for (;j<=i+length;j++,i++)
        {
            res[i] = qname[j];
        }
        if(qname[j]!=0)
            res[i] = '.';
        else
            break;
    }
    return res;
}