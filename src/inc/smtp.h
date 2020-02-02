#ifndef _SMTP_H
#define _SMTP_H

extern int smtp_start(int notify);
extern int smtp_from(const char *fromaddy);
extern int smtp_to(const char *toaddy);
extern int smtp_body_start(void);
extern int smtp_body_text(const char *line);
extern int smtp_body_line(const char *line);
extern int smtp_body_end(void);
extern int smtp_end(void);

#endif /* _SMTP_H */
