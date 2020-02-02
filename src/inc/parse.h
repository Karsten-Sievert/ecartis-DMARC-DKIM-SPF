#ifndef _PARSE_H
#define _PARSE_H

#define PARSE_ERR 0
#define PARSE_OK  1
#define PARSE_END 2

extern int parse_message(void);
extern int parse_line(const char *input_line_base, int type, FILE
            *queuefile, int *counter);

extern int open_adminspit(const char *filename);
extern FILE *get_adminspit(void);
extern int handle_spit_admin(const char *line);
extern int handle_spit_admin2(const char *line);

extern void strip_queue(void);

#endif /* _PARSE_H */
