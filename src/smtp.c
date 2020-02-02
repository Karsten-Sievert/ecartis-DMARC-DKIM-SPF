#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <unistd.h>
#else
#include <winsock2.h>
#endif

#include "config.h"
#include "sockio.h"
#include "variables.h"
#include "core.h"
#include "hooks.h"
#include "compat.h"
#include "mystring.h"

#define CAPS_MIME	0x01
#define CAPS_DSN	0x02



const char *cfg_user_notify = "DELAY,FAILURE";

LSOCKET my_socket = -1;
int servercaps;
int receipients;

int notify_type;

int try_connect_EHLO(const char *hostname)
{
    char dbg_hostname[BIG_BUF];
    char dbg_version[BIG_BUF];
    char buf[BIG_BUF];
    char testme[BIG_BUF];
    int read_data = 0;

    if(my_socket != -1)
        sock_close(my_socket);

    log_printf(9, "Attempting to connect to %s\n", get_string("mailserver"));

    if (sock_open(get_string("mailserver"), get_number("smtp-socket"), &my_socket))
        return 0;

    while(sock_readline(my_socket, buf, sizeof(buf)) != 0) {
        int val;
        char ch;

        read_data = 1;
        log_printf(9,"Server sent: %s\n", buf);

        sscanf(buf, "%d%c%s %s", &val, &ch, &dbg_hostname[0], &dbg_version[0]);
        if(val != 220) {
             return 0;
        }
        if(ch != '-') {
            log_printf(9, "Connected: %s (%s)\n", dbg_hostname, dbg_version); 
            break;
        }
    }
    if(!read_data)
        return 0;

    sock_printf(my_socket, "EHLO %s\r\n", hostname);
    if(sock_readline(my_socket, buf, sizeof(buf)) == 0)
        return 0;

    /* Check for valid response */
    if(!sscanf(buf, "250-%s", testme))
        return 0;

    /* Okay, we have a valid ESMTP server.  Read the server caps */
    while(sock_readline(my_socket, buf, sizeof(buf)) != 0) {
        int val;
        char ch;

        log_printf(9,"Server sent: %s\n", buf);

        sscanf(buf, "%d%c%s", &val, &ch, testme);
        if(val != 250) {
             return 0;
        }
        if(ch != '-')
            break;
        else {
            if (!strcmp(testme,"DSN")) {
                servercaps &= CAPS_DSN;
                log_printf(9, "Server caps: server supports DSN\n");
            } else if (!strcmp(testme,"8BITMIME")) { 
                servercaps &= CAPS_MIME;
                log_printf(9, "Server caps: server supports MIME\n");
            }
        }
    }
    return 1;
}

int try_connect_HELO(const char *hostname)
{
    char dbg_hostname[BIG_BUF];
    char dbg_version[BIG_BUF];
    char buf[BIG_BUF];
    char testme[BIG_BUF];

    /* Try the HELO protocal */
    if(my_socket != -1)
        sock_close(my_socket);

    if (sock_open(get_string("mailserver"), get_number("smtp-socket"), &my_socket))
        return 0;

    if (sock_readline(my_socket, buf, sizeof(buf)) == 0)
        return 0;

    if(!sscanf(buf,"220 %s %s", &dbg_hostname[0], &dbg_version[0]))
        return 0;
    log_printf(9, "Connected: %s (%s)\n", dbg_hostname, dbg_version); 

    sock_printf(my_socket, "HELO %s\r\n", hostname);
    if(sock_readline(my_socket, buf, sizeof(buf)) == 0)
        return 0;
    if(!sscanf(buf, "250 %s", testme))
        return 0;
    return 1;
}

int smtp_start(int notify) 
{
    char hostname[BIG_BUF];

    clean_var("smtp-last-error", VAR_TEMP);
    notify_type = notify;

    my_socket = -1;
    servercaps = 0;
    receipients = 0;

    build_hostname(hostname,sizeof(hostname));

    if(!try_connect_EHLO(hostname)) {
        if(!try_connect_HELO(hostname)) {
            if(my_socket != -1)
                sock_close(my_socket);
            my_socket = -1;
            set_var("smtp-last-error", "Error connecting to SMTP server.", VAR_TEMP);
            return 0;
        }
    }
    return 1;
}

int smtp_from(const char *fromaddy)
{
    int result, final;
    char buf[BIG_BUF];
    int maxretries;

    maxretries = get_number("max-rcpt-tries");
    if (!maxretries) maxretries = 5;

    clean_var("smtp-last-error", VAR_TEMP);
    final = sock_printf(my_socket,"MAIL FROM:<%s>", fromaddy);

    log_printf(9, "Mail from set to %s\n", fromaddy);

    if (servercaps & CAPS_DSN) {
        result = sock_printf(my_socket," RET=%s",
                             (notify_type ? "FULL" : "HDRS"));
        final += result;
    }

    final += sock_printf(my_socket, "\r\n");
    if (!final) {
        set_var("smtp-last-error", "Error setting SMTP from address.", VAR_TEMP);
        return 0;
    }

    if (!get_bool("smtp-blind-blast")) {
       memset(buf, 0, sizeof(buf));
          result = 0;
       if (get_bool("smtp-retry-forever")) {

          while (!result) {
             result = sock_readline(my_socket, buf, sizeof(buf));
          }
       } else {
          int tries = 0;

          log_printf(9, "sock_readline maxretries=%d\n", maxretries);
 
          while(!result && (tries < maxretries)) {
             result = sock_readline(my_socket, buf, sizeof(buf));
             tries++;
          }
       }

       if (result) {
           log_printf(9, "Response: %s\n", buf); 

           if (strncmp(buf,"250",3)) {
               result = 0;
               log_printf(9, "Sender '%s' rejected!\n", fromaddy);
               set_var("smtp-last-error", buf, VAR_TEMP);
           } else {
                log_printf(9, "Sender '%s' accepted.\n", fromaddy);
           }
        } else {
            log_printf(9,"IO Err: Read timeout on MAIL FROM.");
            set_var("smtp-last-error", "Error setting SMTP from address.",
                  VAR_TEMP);
        }
        return (result != 0);
    }
    else return 1;
}

int smtp_to(const char *toaddy)
{
    char buf[BIG_BUF];
    int result, final;
    int maxretries;

    result = 0;
    maxretries = get_number("max-rcpt-tries");
    if (!maxretries) maxretries = 5;

    clean_var("smtp-last-error", VAR_TEMP);
    final = sock_printf(my_socket, "RCPT TO:<%s>", toaddy);
   
    log_printf(9, "Mail sent to %s\n", toaddy);

    if (servercaps & CAPS_DSN) {
        result = sock_printf(my_socket, " NOTIFY=%s", cfg_user_notify);
        final += result;

        result = sock_printf(my_socket, " ORCPT=%s", toaddy);
        final += result;
    }

    final += sock_printf(my_socket, "\r\n");
  
    if (!final) {
        set_var("smtp-last-error","Can't send to SMTP server (RCPT TO)", VAR_TEMP);
        return 0;
    }

    if (!get_bool("smtp-blind-blast")) {
       int tries;
       int tryforever = 0;

       tries = 0; result = 0;

       while (!result && (tryforever ? 1 : tries < maxretries)) {
          result = sock_readline(my_socket, buf, sizeof(buf));
       }

       if (result > 0) {
           log_printf(9, "Response: %s\n", buf);

           if (strncmp(buf,"25",2)) {
               result = 0;
               log_printf(9, "Receipient '%s' rejected!\n", toaddy);
               set_var("smtp-last-error", buf, VAR_TEMP);
               set_var("bounce-error", buf, VAR_TEMP);
               set_var("bounce-address", toaddy, VAR_TEMP);
               (void)do_hooks("LOCAL-BOUNCE");
           } else {
               receipients++;
               log_printf(9, "Receipient '%s' accepted. (%d total)\n",
                 toaddy, receipients);
           }
        }
    }

    if (get_bool("sendmail-sleep")) {
        int sleeplen;

        sleeplen = get_seconds("sendmail-sleep-length");
        do_sleep(sleeplen);
    }
   
    return (result != 0);
}

int smtp_body_start()
{
    char buf[BIG_BUF];
    int tryforever;
    int maxretries;

    maxretries = get_number("max-rcpt-tries");
    if (!maxretries) maxretries = 5;

    tryforever = get_bool("smtp-retry-forever");

    clean_var("smtp-last-error", VAR_TEMP);
    log_printf(9, "Data transmit starting.\n");
    if (!sock_printf(my_socket,"DATA\r\n")) {
        set_var("smtp-last-error", "Unable to write to SMTP server (DATA).", VAR_TEMP);
        return 0;
    }

    if (!get_bool("smtp-blind-blast")) {
       int result, tries;

       tries = 0; result = 0;

       while(!result && (tryforever ? 1 : tries < maxretries)) {
           result = sock_readline(my_socket, buf, sizeof(buf));
           tries++;
       }

       if (!result) {
           set_var("smtp-last-error", "No response from SMTP server (DATA).", VAR_TEMP);
           return 0;
       }

       log_printf(9, "Response: %s\n", buf);

       if (strncmp(buf,"354",3)) {
           log_printf(9, "Unable to start message body!\n");
           set_var("smtp-last-error", buf, VAR_TEMP);
           return 0;
       }
   }

   return 1;
}

void smtp_body_822bis(const char *src, char *dest, size_t size)
{
    const char *ptr1;
    char *ptr2, *end;
    int lastcr;

    lastcr = 0;

    ptr1 = src;
    ptr2 = dest;
    end = dest + size - 2;

    while(*ptr1 && ptr2 < end) {
       if ((*ptr1 == '\n') && (!lastcr)) {
          *ptr2++ = '\r';
       } else if (*ptr1 == '\r') {
          lastcr = 1;
       } else if (*ptr1 == '.') {
          /* Handle escaping of single periods on a line */
          if((ptr2 == dest) || (*(ptr2-1) == '\n')) {
             *ptr2++ = '.';
          }
       }

       if (lastcr && (*ptr1 != '\r')) {
          lastcr = 0;
       }

       *ptr2++ = *ptr1++;
    }

    *ptr2 = 0;
}

int smtp_body_text(const char *line)
{
    char buffer[HUGE_BUF];

    smtp_body_822bis(line,&buffer[0], sizeof(buffer));

    clean_var("smtp-last-error", VAR_TEMP);
    if (!sock_printf(my_socket,"%s",buffer)) {
        set_var("smtp-last-error", "Unable to write to SMTP server (message body).", VAR_TEMP);
        return 0;
    }

    return 1;
}

int smtp_body_line(const char *line)
{
    char buffer[HUGE_BUF];
    char buffer2[HUGE_BUF];

    buffer_printf(buffer2, sizeof(buffer2) - 1, "%s\r\n", line);

    smtp_body_822bis(buffer2,&buffer[0], sizeof(buffer));

    clean_var("smtp-last-error", VAR_TEMP);
    if (!sock_printf(my_socket,"%s",buffer)) {
        set_var("smtp-last-error", "Unable to write to SMTP server (message body).", VAR_TEMP);
        return 0;
    }

    return 1;
}

int smtp_body_end()
{
    char buf[BIG_BUF], buf2[BIG_BUF];
    int tryforever;
    int maxretries;

    maxretries = get_number("max-rcpt-tries");
    if (!maxretries) maxretries = 5;

    tryforever = get_bool("smtp-retry-forever");

    clean_var("smtp-last-error", VAR_TEMP);
    log_printf(9, "Data transmit ending.\n");
    if (!sock_printf(my_socket,"\r\n.\r\n")) {
        set_var("smtp-last-error", "Unable to write to SMTP server (message end).", VAR_TEMP);
        return 0;
    }

    if (!get_bool("smtp-blind-blast")) {
       int tries, result;

       tries = 0; result = 0;

       while(!result && (tryforever ? 1 : tries < maxretries)) {
           result = sock_readline(my_socket, buf, sizeof(buf));
           tries++;
       }

       if (!result) {
           set_var("smtp-last-error", "No response from SMTP server. (message end)", VAR_TEMP);
           return 0;
       }

       log_printf(9, "Response: %s\n", buf);

       if (!strncmp(&buf[0],"250",3)) {
           if (sscanf(buf, "250 %s Message accepted for delivery.", &buf2[0]) == 1)
              log_printf(9, "Message accepted and queued as %s.\n", buf2);
       } else {
           log_printf(9, "Message rejected.\n");
           set_var("smtp-last-error", buf, VAR_TEMP);
           return 0;
       }
    }

    return 1;
}

int smtp_end()
{
    clean_var("smtp-last-error", VAR_TEMP);
    log_printf(9, "Disconnecting.\n");
    if (!sock_printf(my_socket, "QUIT\r\n")) {
        set_var("smtp-last-error", "Unable to write to SMTP server (QUIT).", VAR_TEMP);
        return 0;
    }

    sock_close(my_socket);
    my_socket = -1;
    log_printf(9, "Disconnected.\n");

    return 1;
}
