#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <winsock2.h>

#include "compat.h"
#include "core.h"
#include "variables.h"

#include "sockio.h"

void sock_init()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
		log_printf(0,"WSAStartup failed: unable to initialize Windows socket service.\n");
		internal_error("Unable to initialize Window Socket service");
	} else {
		log_printf(3, "Winsock enabled...\n");
	}
}


/* Write a formatted string to a socket */
int sock_printf(LSOCKET sock, char *format, ...)
{
    va_list vargs;
    int result;
    char mybuf[BIG_BUF];

    if (sock == -1) return 0;

    va_start(vargs, format);
    vsprintf(mybuf, format, vargs);
    va_end(vargs);

    result = send(sock, &mybuf[0], strlen(mybuf), 0);

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

        numread = recv(sock, &getme[0], 1, 0);
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
                tv.tv_sec = duration;
                tv.tv_usec = 0;
                FD_ZERO(&read_fds);
                FD_SET(sock, &read_fds);
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
    LSOCKET mysock;
	unsigned char *pAddrList;
	unsigned long longAddr, curPart;
	int i;

    conhost = gethostbyname(conhostname);
    if (conhost == 0) {
//		log_printf(10,"IO: Unable to resolve '%s'...\n", conhostname);
        return -1;
	}

	pAddrList = (unsigned char*)conhost->h_addr_list[0];
	longAddr = 0;
	for(i=0; i < 4; i++)
	{
		curPart = pAddrList[i];
		curPart <<= (3-i) * 8;
		longAddr |= curPart;
	}

    name.sin_port = htons((unsigned short)port);
    name.sin_family = AF_INET;
	name.sin_addr.s_addr = htonl(longAddr);    
    mysock = socket(AF_INET, SOCK_STREAM, 0);
    addr_len = sizeof(name);
   
    if (connect(mysock, (struct sockaddr *)&name, addr_len) == SOCKET_ERROR) {
//		log_printf(10,"IO: Unable to open socket...\n");
        return -1;
	}

    *sock = mysock;
 
    return 0;
}

int sock_close(LSOCKET sock)
{
	if (sock == 0) return 0;

	return (closesocket(sock));
}

