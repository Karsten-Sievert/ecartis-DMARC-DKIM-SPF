#ifndef _CODES_H
#define _CODES_H

extern void to64(FILE *infile, FILE *outfile);
extern void from64(FILE *infile, FILE *outfile, char **boundaries, int *boundaryct);
extern void toqp(FILE *infile, FILE *outfile);
extern void fromqp(FILE *infile, FILE *outfile, char **boundaries, int *boundaryct);

extern void fromrfc2047(const char *orig, char *dest, int maxlen);
extern void toqps(unsigned const char *orig, char *dest, int maxlen);
#endif /* _CODES_H */
