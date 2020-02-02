#include "lpm.h"

extern struct LPMAPI *LMAPI;

extern void handle_error(int, const char *, const char *, const char *);

extern void parse_postfix_bounce(FILE *, FILE *, const char *, int *);
extern void parse_qmail_bounce(FILE *, FILE *, const char *, int *);
extern void parse_exim_bounce(FILE *, FILE *, const char *, int *);
extern void parse_msn_bounce(FILE *, FILE *, const char *, int *);
extern void parse_sendmail_bounce(FILE *, FILE *, const char *, int *);
extern void parse_lotus_bounce(FILE *, FILE *, const char *, int *);
extern void parse_yahoo_bounce(FILE *, FILE *, const char *, int *);
extern void parse_mms_relay_bounce(FILE *, FILE *, const char *, int *);
extern int parse_xmail_bounce(FILE *, FILE *, const char *, char *, int *);

