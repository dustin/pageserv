/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: getqueue.c,v 2.3 1997/03/31 23:12:00 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include <pageserv.h>
#include <tap.h>

extern struct config conf;

void runqueue(void)
{
    struct queuent *q;
    int i;

    q=listqueue("219.1805");

    for(i=0; q[i].to[0] != 0x00 ; i++)
    {
	printf("%d to %s\n", i, q[i].to);
    }
}

int gq_checkit(struct queuent q, char *number)
{
    if( (strcmp(q.u.statid, number))!=0)
    {
        return(0);
    }

    if( readytodeliver(q) == 0)
    {
	return(0);
    }

    return(1);
}

struct queuent *listqueue(char *number)
{
    DIR *dir;
    FILE *f;
    struct dirent *d;
    char buf[BUFLEN];
    struct queuent q;
    int index=0, size=4;
    struct queuent *list;

    chdir(conf.qdir);
    dir=opendir(".");

    list=(void *)malloc(size*sizeof(struct queuent));

    while( (d=readdir(dir))!=NULL)
    {
        if(d->d_name[0]=='q')
        {
            f=fopen(d->d_name, "r");

            fgets(buf, BUFLEN, f);
            sscanf(buf, "%d", &q.priority);
            fgets(q.to, TOLEN, f);
            fgets(q.message, BUFLEN, f);
            kw(q.to);
            kw(q.message);
            strcpy(q.qid, d->d_name);

            fclose(f);

	    getqueueinfo(&q);

            if(gq_checkit(q, number))
	    {
		if(index == size-1)
		{
		    size<<=1;
		    printf("Reallocating, now need %d bytes for %d\n",
			size*sizeof(struct queuent *), size);
		    list=realloc(list, size*sizeof(struct queuent));
		}
		list[index++]=q;
	    }
            /* unlink(q.qid); */
        }
    }
    list[index].to[0]==0x00;
    closedir(dir);
    return(list);
}
