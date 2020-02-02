#ifndef _IO_H
#define _IO_H

#ifndef WIN32
# define LSOCKET int
#else
# define LSOCKET SOCKET
#endif

extern int sock_printf(LSOCKET sock, char *format, ...);
extern int sock_readline(LSOCKET sock, char *buffer, int maxlen);
extern int sock_open(const char *hostname, int port, LSOCKET *sock);
extern int sock_close(LSOCKET sock);

#endif /* _IO_H */
