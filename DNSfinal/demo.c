#include <stdio.h>
#include <string.h>

int main(void)
{
    char domain[30] = "www.123.com";
    char p[30];
    for (int i = 0; i < strlen(domain);)
    {
        for (int j = i + 1;; j++)
        {
            if (domain[j-1] == '.'||domain[j-1]==0)
            {
                p[i] = j - i-1;
                i = j;
                break;
            }
            else
                p[j] = domain[j - 1];
        }
    }
    for (int i = 0; i < strlen(p);i++)
    {
        if(p[i]<=20)
        {
            printf("%d_", p[i]);
        }
        else
        {
            printf("%c ", p[i]);
        }
    }
} 