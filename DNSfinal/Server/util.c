#include "relaysystem.h"

void entryInit(System *system)
{

    FILE *fp = fopen(TRANSLATEPAHT, "r");
    //Entry *entrylist;
    for (int i = 0; !feof(fp) && i < ENTRYCOUNT; i++)
    {
        Entry item;
        fscanf(fp, "%s", item.ip);
        fscanf(fp, "%s", item.domain);

        //printf("%d %s\n", i, item.ip);
        system->entryTable[i] = item;
        system->entrynumber++;
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
            if (strncmp("0.0.0.0", entryTable[i].ip, 7) == 0)
            {
                printf("find blocked domain\n");
                return -2;
            }
            printf("find valid request %d\n", i);
            return i;
        }
    }
    return -1;
}

void IDConvert(System *system, IDevent *event)
{
    char *qname = &event->message[12];
    char res[100];
    domainConvert(qname, res);
    printf("%d::%s\n", strlen(res), res);
    int num = entrySearch(res, system->entryTable);
    if (num >= 0)
    {
        event->status = READY;
        event->entry = num;
        printf("deal result : READY\n");
        octects a = htons(0x8180); //1000 0001 1000 0000
        memcpy(&event->message[2], &a, sizeof(octects));
    }
    else if (num == -1)
    {
        event->status = RELAY;
        event->entry = num;

        printf("deal result : RELAY\n");
        unsigned char *id = event->message;
        (*((octects *)id))++;
    }
    else if (num == -2)
    {
        event->status = READY; //memcpy(&event->message[2],)
        event->entry = num;
        printf("deal result: READY none\n");
    }
}

void domainConvert(char *qname, char *res)
{
    memset(res, 0, (strlen(qname) + 1) * sizeof(char));
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
    //printf("\ndomian:%s\n", res);
}

void makeResponse(System *system, IDevent *event, int iRecv)
{
    char response[16];
    octects Name = htons(0xc00c);
    memcpy(response, &Name, sizeof(octects));

    octects Type = htons(0x0001);
    memcpy(response + 2, &Type, sizeof(octects));

    octects Class = htons(0x0001);
    memcpy(response + 4, &Class, sizeof(octects));

    unsigned long TTL = htonl(0x55);
    memcpy(response + 6, &TTL, sizeof(unsigned long));

    octects IPLen = htons(0x0004);
    memcpy(response + 10, &IPLen, sizeof(octects));

    unsigned long IP = 0;
    IP = (unsigned long)inet_addr(system->entryTable[event->entry].ip);
    octects answer = 0;
    answer = htons(0x0001);
    memcpy(&event->message[6], &answer, sizeof(octects));
    memcpy(response + 12, &IP, sizeof(unsigned long));
    memcpy(event->message + iRecv, response, 16);
    event->iRecv += 16;
}

int popQueue(System *system, IDevent *event)
{
    unsigned char *qr = &event->message[2];
    if (*qr == 0x01)
    {

        unsigned char *type = &event->message[12];
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
        unsigned typevalue = ntohs(*(octects *)type);
        if (typevalue == 1)
        {
            if (event->status == EMPTY)
            {
                event->status = WAIT;
                printf("deal result: WAIT\n");
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
            printf("invalid message!\n");
            return 0;
        }
    }
}

void update(char *ip, IDevent *event, System *sys)
{
    char domain[100];
    domainConvert(&event->message[12], domain);
    FILE *fp = fopen("dnsrelay.txt", "a+");
    char *ip_str = inet_ntoa(*(struct in_addr *)ip);
    char space = ' ';
    char line = '\n';
    fwrite(ip_str, 1, strlen(ip_str), fp);
    fwrite(&space, 1, 1, fp);
    fwrite(domain, 1, strlen(domain), fp);
    fwrite(&line, 1, 1, fp);
    fclose(fp);
    sys->entrynumber++;
    memcpy(sys->entryTable[sys->entrynumber].domain, domain, strlen(domain));
    memcpy(sys->entryTable[sys->entrynumber].ip, ip_str, strlen(ip_str));
    sys->entryTable[sys->entrynumber].domain[strlen(domain)] = 0;
    sys->entryTable[sys->entrynumber].ip[strlen(ip_str)] = 0;
    printf("update:%d, domain:%s,ip:%s", sys->entrynumber, sys->entryTable[sys->entrynumber].domain, sys->entryTable[sys->entrynumber].ip);
}

void block(IDevent *event)
{
    char *p = &event->message[2];
    *((octects *)p) = htons(0x8185);
    p += 4;
    *((octects *)p) = htons(0x0000);
    event->iRecv -= 16;
}