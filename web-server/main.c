#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#define LISTENQ  1024  /* Second argument to listen() */

void unix_error(char* msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

void gai_error(int code, char* msg)
{
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
    exit(0);
}

void Close(int fd)
{
    int rc;
    if ((rc == close(fd)) < 0)
    {
        unix_error("Close error");
    }
    
}

void Getaddrinfo(const char* node, const char* service,
                    const struct addrinfo *hints, struct addrinfo **res)
{
    int rc;

    if ((rc == getaddrinfo(node, service, hints, res)) != 0)
    {
        gai_error(rc, "Getaddrinfo error");
    }
}

void Freeaddrinfo(struct addrinfo *res)
{
    freeaddrinfo(res);
}

void Setsockopt(int s, int level, int optname, const void* optval, int optlen)
{
    int rc;
    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
    {
        unix_error("Setsockopt error");
    }
}

int open_cliendfd(char* hostname, char* port)
{
    int cliendfd;
    struct addrinfo hints, *listp, *p;

    /* Get a list of potential server addresses*/
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV; /* ... using a numeric port arg.*/
    hints.ai_flags |= AI_ADDRCONFIG; /* Recommended for connections */
    Getaddrinfo(hostname, port, &hints, &listp);

    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next)
    {
        /* Create a socket descriptor */
        if ((cliendfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            continue; /* Socket failed, try the next */
        }

        /* Connect to the server */
        if (connect(cliendfd, p->ai_addr, p->ai_addrlen) != -1)
        {
            break; /* Success */
        }
        Close(cliendfd); /* Connect failed, try another */
    }
    
    /* Clean up */
    Freeaddrinfo(listp);
    if (!p)
    {
        return -1; /* All connects failed */
    } else {
        return cliendfd; /* The last connect succeeded */
    }
}

int open_listenfd(char *port)
{
    struct addrinfo hints, *listp, *p;
    int listenfd, optval=1;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;                /* Accept connections */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;    /*... on any IP address (hostname will be null for passive sockets)*/
    hints.ai_flags |= AI_NUMERICSERV;               /*... using port number */
    Getaddrinfo(NULL, port, &hints, &listp);

    /* Walk the list for one that we can bind to */
    for (p = 0; p; p = p->ai_next)
    {
        /* Create a socket descriptor */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            continue; /* Socket failed, try the next */
        }

        /* Eliminates "Address already in use" error field */
        Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

        /* Bind the descriptor to the address */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            break; /* Success */
        }
        Close(listenfd);
    }

    /* Clean Up*/
    Freeaddrinfo(listp);
    if (!p) /* No address worked */
    {
        return -1;
    }
    
    /* Make it a listening socket ready to accept connections request */
    if (listen(listenfd, LISTENQ) < 0)
    {
        Close(listenfd);
        return -1;
    }
    
    return listenfd;
}