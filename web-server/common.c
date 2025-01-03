#include "common.h"

void unix_error(char* msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

void app_error(char* msg)
{
    fprintf(stderr, "%s\n", msg);
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

void Getnameinfo(const struct sockaddr* sa, socklen_t salen, char* host,
                    size_t hostlen, char* serv, size_t servlen, int flags)
{
    int rc;

    if ((rc = getnameinfo(sa, salen, host, hostlen, serv, servlen, flags)))
    {
        gai_error(rc, "Getnameinfo error");
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

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen) 
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
	unix_error("Accept error");
    return rc;
}

char* Fgets(char* ptr, int n, FILE* stream)
{
    char* rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
    {
        app_error("Fgets error");
    }
    return rptr;
}

void Fputs(const char* ptr, FILE* stream)
{
    if (fputs(ptr, stream) == EOF)
    {
        unix_error("Fputs error");
    }
}

/* Robustly read n bytes (unbuffered) */
ssize_t rio_readn(int fd, void* usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char* bufp = usrbuf;

    while (nleft > 0)
    {
        if ((nread = read(fd, bufp, nleft)) < 0)
        {
            if (errno == EINTR)
            {
                nread = 0;
            } else {
                return -1;
            }
        } else if (nread == 0) {
            break;
        }
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);
}

/* Robustly write n bytes (unbuffered) */
ssize_t rio_writen(int fd, void* userbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char* bufp = userbuf;

    while (nleft > 0)
    {
        if (nwritten = write(fd, bufp, nleft) <= 0)
        {
            if (errno == EINTR)     /* Interrupted by sig handler*/
            {
                nwritten = 0;
            } else {
                return -1;          /* errno set by write() */
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}

/* 
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* Refill if buf is empty */
	rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, 
			   sizeof(rp->rio_buf));
	if (rp->rio_cnt < 0) {
	    if (errno != EINTR) /* Interrupted by sig handler return */
		return -1;
	}
	else if (rp->rio_cnt == 0)  /* EOF */
	    return 0;
	else 
	    rp->rio_bufptr = rp->rio_buf; /* Reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;          
    if (rp->rio_cnt < n)   
	cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

void rio_readinitb(rio_t* rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;
    
    while (nleft > 0) {
        if ((nread = rio_read(rp, bufp, nleft)) < 0) 
                return -1;                              /* errno set by read() */ 
        else if (nread == 0)
            break;                                      /* EOF */
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);                                 /* return >= 0 */
}

/* Robustly read a text line (buffered)*/
ssize_t rio_readlineb(rio_t* rp, void* userbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = userbuf;

    for (n = 1; n < maxlen; n++)
    {
        /* TODO: write rio_read */
        if ((rc = rio_read(rp, &c, 1)) == 1)
        {
            *bufp++ = c;
            if (c == '\n')
            {
                n++;
                break;
            }
        } else if (rc == 0) {
            if (n == 1)
            {
                return 0; /* EOF, no data read*/
            } else {
                break;    /* EOF, some data was read */
            }
        } else {
            return -1;    /* Error */
        }
    }
    *bufp = 0;
    return n-1;
}

ssize_t Rio_readn(int fd, void *ptr, size_t nbytes) 
{
    ssize_t n;
  
    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
    {
        unix_error("Rio_readn error");
    }
    return n;
}

void Rio_writen(int fd, void* userbuf, size_t n)
{
    if (rio_writen(fd, userbuf, n) != n)
    {
        unix_error("Rio_writen error");
    }
}

void Rio_readinitb(rio_t* rp, int fd)
{
    rio_readinitb(rp, fd);
}

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
    {
        unix_error("Rio_readnb error");
    }
    return rc;
}

ssize_t Rio_readlineb(rio_t* rp, void* userbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, userbuf, maxlen)) < 0)
    {
        unix_error("Rio_readlineb error");
    }
    return rc;
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
    printf("port: %s\n", port);
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

int Open_clientfd(char* hostname, char* port)
{
    int rc;

    if ((rc = open_cliendfd(hostname, port)) < 0)
    {
        unix_error("Open_clientfd error");
    }
    return rc;  
}

int Open_listenfd(char* port)
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
    {
        unix_error("Open_listenfd error");
    }
    return rc;
}