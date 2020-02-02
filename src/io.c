#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#ifdef IRIX_IS_CRAP
#include <bstring.h>
#endif /* IRIX_IS_CRAP */

#include "compat.h"
#include "core.h"
#include "sockio.h"
#include "variables.h"

/* Write a formatted string to a socket */
int sock_printf(LSOCKET sock, char *format, ...)
{
    va_list vargs;
    int result;
    char mybuf[HUGE_BUF];

    if (sock == -1) return 0;

    va_start(vargs, format);
    vsprintf(mybuf, format, vargs);
    va_end(vargs);

    result = write(sock, &mybuf[0], strlen(mybuf));

    /* Ricardo-debug */
    log_printf(10, "IO OUT: %s\n", mybuf);

    return(result);
}

/* Read a line of input from the socket */
int sock_readline(LSOCKET sock, char *buffer, int maxlen)
{
    fd_set read_fds;
    int done, pos;
    int duration;
    struct timeval tv;
    int possible_close = 0;

    if (sock == -1) return 0;

    duration = get_seconds("socket-timeout");

    memset(buffer, 0, maxlen);
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);
    /* wait up to 30 seconds */
    tv.tv_sec = duration;
    tv.tv_usec = 0;
    done = 0;
    pos = 0;

    /* Check for data before giving up. */
    if(select(sock+1, &read_fds, NULL, NULL, &tv) <= 0) {
        return 0;
    }

    done = 0;
    if(!FD_ISSET(sock, &read_fds)) {
        return 0;
    }

    while(!done && (pos < maxlen)) {
        char getme[2];
        int numread;

        numread = read(sock, &getme[0], 1);
        if (numread != 0) {
            possible_close = 0;
            if (getme[0] != '\n') {
                *(buffer + pos) = getme[0];
                pos++;
            } else {
                done = 1;
            }
        } else {
            if(possible_close) {
                done = 1;
            } else {
                FD_ZERO(&read_fds);
                FD_SET(sock, &read_fds);
                /* wait up to 30 seconds */
                tv.tv_sec = duration;
                tv.tv_usec = 0;
                /* Check for data before giving up. */
                if(select(sock+1, &read_fds, NULL, NULL, &tv) <= 0) {
                    done = 1;
                }

                if(FD_ISSET(sock, &read_fds)) {
                    possible_close = 1;
                }
            }
        }
    }
    *(buffer + pos + 1) = 0;

    /* Ricardo-debug */
    if (strlen(buffer) > 0) {
       log_printf(10,"IO IN : %s\n", buffer);
    }

    return strlen(buffer);
}

/* Open a socket to a specific host/port */
int sock_open(const char *conhostname, int port, LSOCKET *sock)
{
    struct hostent *conhost;
    struct sockaddr_in name;
    int addr_len;
    int mysock;

    conhost = gethostbyname(conhostname);
    if (conhost == 0)
        return -1;

    name.sin_port = htons(port);
    name.sin_family = AF_INET;
    bcopy((char *)conhost->h_addr, (char *)&name.sin_addr, conhost->h_length);
    mysock = socket(AF_INET, SOCK_STREAM, 0);
    addr_len = sizeof(name);
   
    if (connect(mysock, (struct sockaddr *)&name, addr_len) == -1)
        return -1;

    *sock = mysock;
 
    return 0;
}

int sock_close(LSOCKET sock)
{
    return close(sock);
}
