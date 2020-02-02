#ifndef _UNMIME_H
#define _UNMIME_H

struct mime_field {
   char *name;
   char *value;
   char *params;
};

struct mime_header {
   int numfields;
   struct mime_field **fields;
};

#define MIME_PARAMS struct mime_header *header, const char *mimefile
typedef int (*MimeFn)(MIME_PARAMS);

#define MIME_HANDLE_OK		0
#define MIME_HANDLE_FAIL	1

#define MIME_HANDLER(x)  int x(MIME_PARAMS)

struct mime_handler {
   char *mimetype;
   MimeFn handler;
   int priority;
   struct mime_handler *next;
};

/* The Guts Of The Program */
extern void unmime_file(const char *file1, const char *file2);
extern void unquote_file(const char *file1, const char *file2);
extern void unquote_string(const char *orig, char *dest, int len);
extern void requote_string(const char *orig, char *dest, int len);

/* Handler management */
extern void new_mime_handlers();
extern void nuke_mime_handlers();
extern void add_mime_handler(const char *mimetype, int priority, MimeFn handler);

/* Mime variable management */
extern void mime_eat_header(struct mime_header *);
extern struct mime_field *mime_getfield(struct mime_header *, const char *);
extern const char *mime_fieldval(struct mime_header *, const char *);
extern const char *mime_parameter(struct mime_header *, const char *, const char *);

/* Default handlers */
extern MIME_HANDLER(mimehandle_multipart_default);
extern MIME_HANDLER(mimehandle_multipart_alternative);
extern MIME_HANDLER(mimehandle_text);
extern MIME_HANDLER(mimehandle_unknown);
extern MIME_HANDLER(mimehandle_rabid);
extern MIME_HANDLER(mimehandle_moderate);
#endif /* _UNMIME_H */
