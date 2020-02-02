#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif /* WIN32 */

#include "core.h"
#include "config.h"
#include "smtp.h"
#include "list.h"
#include "variables.h"
#include "compat.h"
#include "mystring.h"

extern int messagecnt;

/* general code for sending a task report to an address */
int task_heading(const char *toaddy)
{
    const char *conf, *extraheader;
    char datebuffer[80];
    char datestr[80];
    char buffer[BIG_BUF];
    char hostname[BIG_BUF];
    time_t now;
    struct tm *tm_now;
    const char *tptr;
    int expires, seconds = 0;
    const char *fromname;

    time(&now);

    expires = get_bool("task-expires");
    clean_var("task-expires", VAR_TEMP);

    if (expires) {
       seconds = get_seconds("reply-expires-time");
    }

    get_date(datestr, sizeof(datestr), now);
    buffer_printf(datebuffer, sizeof(datebuffer) - 1, "Date: %s", datestr);

	if(get_var("hostname")) {
		buffer_printf(hostname, sizeof(hostname) - 1, "%s", get_string("hostname"));
	} else {
		build_hostname(hostname, sizeof(hostname) - 1);
	}

    if(!smtp_start(0))
        return 0;

    tptr = get_var("form-send-as");
    if (!tptr) {
        tptr = get_var("list-owner");
    }
    if (!tptr) {
        tptr = get_var("listserver-admin");
    }
    if(!smtp_from(tptr))
        return 0;
    if(!smtp_to(toaddy))
        return 0;

    conf = get_var("form-cc-address");
    if (conf) {
        if(!smtp_to(conf))
            return 0;
    }

    extraheader = get_var("stocksend-extra-headers");
  
    if(!smtp_body_start())
        return 0;
    buffer_printf(buffer, sizeof(buffer) - 1, "Received: from %s by %s (%s/%s);\n\t%s", hostname,
            hostname, SERVICE_NAME_UC, VER_PRODUCTVERSION_STR, datestr);
    smtp_body_line(buffer);
    smtp_body_line(datebuffer);
    if (get_bool("form-show-listname") && get_var("list")) {
        fromname = get_var("list");
    } else
        fromname = get_var("listserver-full-name");
    buffer_printf(buffer, sizeof(buffer) - 1, "From: %s <%s>", fromname, get_var("listserver-address"));
    smtp_body_line(buffer);
    tptr = get_var("form-reply-to");
    if (tptr) {
        buffer_printf(buffer, sizeof(buffer) - 1, "Reply-To: %s", tptr);
        smtp_body_line(buffer);
    }
    buffer_printf(buffer, sizeof(buffer) - 1, "To: %s", toaddy);
    smtp_body_line(buffer);
    if (conf) {
        buffer_printf(buffer, sizeof(buffer) - 1, "Cc: %s", conf);
        smtp_body_line(buffer);
    }
    tm_now = localtime(&now);
    buffer_printf(buffer, sizeof(buffer) - 1, "%s-%s", SERVICE_NAME_LC, "%m%d%Y%H%M%S");
    strftime(datebuffer, sizeof(datebuffer) - 1, buffer, tm_now);
    buffer_printf(buffer, sizeof(buffer) - 1, "Message-ID: <%s.%d.%d@%s>", datebuffer, (int)getpid(),
			messagecnt++, hostname);
    smtp_body_line(buffer);
    buffer_printf(buffer, sizeof(buffer) - 1, "X-%s-antiloop: %s", SERVICE_NAME_LC, hostname);
    smtp_body_line(buffer);
    smtp_body_line("Precedence: list");

    if (expires) {
       now = now + seconds;

       get_date(datestr, sizeof(datestr), now);
       buffer_printf(datebuffer, sizeof(datebuffer) - 1, "Expiry-Date: %s", datestr);
       smtp_body_line(datebuffer);
    }

    if (get_var("task-form-subject")) {
        buffer_printf(buffer, sizeof(buffer) - 1, "Subject: %s", get_string("task-form-subject"));
        clean_var("task-form-subject", VAR_TEMP);
    } else {
        buffer_printf(buffer, sizeof(buffer) - 1, "Subject: %s request results", SERVICE_NAME_MC);
    }

    if (extraheader) 
        smtp_body_line(extraheader);

    smtp_body_line(buffer);
    smtp_body_line("");
    return 1;
}

/* Finish sending a general task message */
void task_ending()
{
    char buf[BIG_BUF];

    if (!get_bool("task-no-footer")) {
       smtp_body_line("");
       smtp_body_line("---");
       buffer_printf(buf, sizeof(buf) - 1, "%s v%s - job execution complete.", SERVICE_NAME_MC,
               VER_PRODUCTVERSION_STR);
       smtp_body_line(buf);
    }
    smtp_body_end();
    smtp_end();
}

/* Send an error message to the list owner and the server admin */
int error_heading()
{
    char datebuffer[80];
    char datestr[80];
    char hostname[BIG_BUF];
    char buf[BIG_BUF];
    const char *sendas;
    const char *sendto = NULL;
    int valid_send = 0;
    time_t now;
    struct tm *tm_now;
    const char *fromname;

    time(&now);

    get_date(datestr, sizeof(datestr), now);
    buffer_printf(datebuffer, sizeof(datebuffer) - 1, "Date: %s", datestr);
	if(get_var("hostname")) {
		buffer_printf(hostname, sizeof(hostname) - 1, "%s", get_string("hostname"));
	} else {
		build_hostname(hostname, sizeof(hostname) - 1);
	}

    sendas = get_var("listserver-admin");
    if(!sendas) {
        log_printf(0, "Unable to send error report! no listserver-admin set.");
        return 0;
    }

    if(!smtp_start(0)) {
        log_printf(0, "Unable to send error report! %s.", get_string("smtp-last-error"));
        return 0;
    }

    if(!smtp_from(sendas)) {
        log_printf(0, "Unable to send error report! %s had error '%s'.", sendas,
                   get_string("smtp-last-error"));
        return 0;
    }
    if(get_var("listserver-admin")) {
        if(!smtp_to(get_string("listserver-admin"))) {
            log_printf(0, "Unable send error report to listserver-admin! %s had error '%s'.", get_string("listserver-admin"), get_string("smtp-last-error"));
        } else {
            sendto = get_string("listserver-admin");
            valid_send = 1;
        }
    } 

    if(!valid_send)
        return 0;

    if(!smtp_body_start())
        return 0;
    buffer_printf(buf, sizeof(buf) - 1, "Received: from %s by %s (%s/%s);\n\t%s", hostname,
            hostname, SERVICE_NAME_UC, VER_PRODUCTVERSION_STR, datestr);
    smtp_body_line(buf);
    smtp_body_line(datebuffer);
    fromname = get_var("listserver-full-name");
    buffer_printf(buf, sizeof(buf) - 1, "From: %s <%s>", fromname, sendas);
    smtp_body_line(buf);
    buffer_printf(buf, sizeof(buf) - 1, "To: %s", sendto);
    smtp_body_line(buf);
    buffer_printf(buf, sizeof(buf) - 1, "%s-%s", SERVICE_NAME_LC, "%m%d%Y%H%M%S");
    tm_now = localtime(&now);
    strftime(datebuffer, sizeof(datebuffer) - 1, buf, tm_now);
    buffer_printf(buf, sizeof(buf) - 1, "Message-ID: <%s.%d.%d@%s>", datebuffer, (int)getpid(),
			messagecnt++, hostname); 
    smtp_body_line(buf);
    buffer_printf(buf, sizeof(buf) - 1, "X-%s-antiloop: %s", SERVICE_NAME_LC, hostname);
    smtp_body_line(buf);
    buffer_printf(buf, sizeof(buf) - 1, "Subject: %s Error Report", SERVICE_NAME_MC);
    smtp_body_line(buf);
    smtp_body_line("");
    smtp_body_line("Error report:");
    smtp_body_line("");
    return 1;
}

/* General error handler ending */
void error_ending()
{
    smtp_body_line("");
    smtp_body_line("---");
    smtp_body_line("End of error report.");
    smtp_body_end();
    smtp_end();
}
