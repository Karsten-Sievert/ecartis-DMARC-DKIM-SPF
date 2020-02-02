/*
 * Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)
 * 
 * Permission to use, copy, modify, and distribute this material 
 * for any purpose and without fee is hereby granted, provided 
 * that the above copyright notice and this permission notice 
 * appear in all copies, and that the name of Bellcore not be 
 * used in advertising or publicity pertaining to this 
 * material without the specific, prior written permission 
 * of an authorized representative of Bellcore.  BELLCORE 
 * MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY 
 * OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", 
 * WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "codes.h"
#include "core.h"
#include "fileapi.h"
#include "mystring.h"
#include "variables.h"

static char basis_64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char index_64[128] = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
    52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
    -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
    -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
};

#define char64(c)  (((c) < 0 || (c) > 127) ? -1 : index_64[(c)])

void output64chunk(int c1, int c2, int c3, int pads, FILE *outfile)
{
    putc(basis_64[c1>>2], outfile);
    putc(basis_64[((c1 & 0x3)<< 4) | ((c2 & 0xF0) >> 4)], outfile);
    switch(pads) {
        case 2:
            putc('=', outfile);
            putc('=', outfile);
            break;
        case 1:
            putc(basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)], outfile);
            putc('=', outfile);
            break;
        default:
            putc(basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)], outfile);
            putc(basis_64[c3 & 0x3F], outfile);
            break;
    }
}

void to64(FILE *infile, FILE *outfile) 
{
    int c1, c2, c3, ct=0;
    while ((c1 = getc(infile)) != EOF) {
        c2 = getc(infile);
        if (c2 == EOF) {
            output64chunk(c1, 0, 0, 2, outfile);
        } else {
            c3 = getc(infile);
            if (c3 == EOF) {
                output64chunk(c1, c2, 0, 1, outfile);
            } else {
                output64chunk(c1, c2, c3, 0, outfile);
            }
        }
        ct += 4;
        if (ct > 71) {
            putc('\n', outfile);
            ct = 0;
        }
    }
    if (ct)
        putc('\n', outfile);
    fflush(outfile);
}

int pendingBoundary(char *s, char **boundaries, int *boundaryCt)
{
    int i, len;

    if (s[0] != '-' || s[1] != '-')
        return(0);

    for (i=0; i < *boundaryCt; ++i) {
	len = strlen(boundaries[i]);
        if (!strncmp(s, boundaries[i], len)) {
            if (s[len] == '-' && s[len+1] == '-')
                *boundaryCt = i;
            return(1);
        }
    }
    return(0);
}

void from64(FILE *infile, FILE *outfile, char **boundaries, int *boundaryct) 
{
    int c1, c2, c3, c4;
    int newline = 1, DataDone = 0;

    /* always reinitialize */
    while ((c1 = getc(infile)) != EOF) {
        if (isspace(c1)) {
            newline = ((c1 == '\n') ? 1 : 0);
            continue;
        }
        if (newline && boundaries && c1 == '-') {
            char buf[200];
            /* a dash is NOT base 64, so all bets are off if NOT a boundary */
            ungetc(c1, infile);
            read_file(buf, sizeof(buf), infile);
            if (boundaries && (buf[0] == '-') && (buf[1] == '-') &&
                pendingBoundary(buf, boundaries, boundaryct)) {
                return;
            }
            log_printf(1, "Ignoring unrecognized boundary line: %s\n", buf);
            continue;
        }
        if (DataDone)
            continue;
        newline = 0;
        do {
            c2 = getc(infile);
        } while (c2 != EOF && isspace(c2));
        do {
            c3 = getc(infile);
        } while (c3 != EOF && isspace(c3));
        do {
            c4 = getc(infile);
        } while (c4 != EOF && isspace(c4));
        if (c2 == EOF || c3 == EOF || c4 == EOF) {
            log_printf(9, "Warning: base64 decoder saw premature EOF\n");
            return;
        }
        if (c1 == '=' || c2 == '=') {
            DataDone=1;
            continue;
        }
        c1 = char64(c1);
        c2 = char64(c2);
        putc(((c1<<2) | ((c2&0x30)>>4)), outfile);
        if (c3 == '=') {
            DataDone = 1;
        } else {
            c3 = char64(c3);
            putc((((c2&0XF) << 4) | ((c3&0x3C) >> 2)), outfile);
            if (c4 == '=') {
                DataDone = 1;
            } else {
                c4 = char64(c4);
                putc((((c3&0x03) <<6) | c4), outfile);
            }
        }
    }
}

static char basis_hex[] = "0123456789ABCDEF";
static char index_hex[128] = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
    -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

#define hexchar(c)  (((c) > 127) ? -1 : index_hex[(c)])

void toqp(FILE *infile, FILE *outfile) 
{
    int c, ct=0, prevc=255;
    while ((c = getc(infile)) != EOF) {
        if ((c < 32 && (c != '\n' && c != '\t')) || (c == '=') ||
            (c >= 127) || (ct == 0 && c == '.')) {
            putc('=', outfile);
            putc(basis_hex[c>>4], outfile);
            putc(basis_hex[c&0xF], outfile);
            ct += 3;
            prevc = 'A'; /* close enough */
        } else if (c == '\n') {
            if (prevc == ' ' || prevc == '\t') {
                putc('=', outfile); /* soft & hard lines */
                putc(c, outfile);
            }
            putc(c, outfile);
            ct = 0;
            prevc = c;
        } else {
            if (c == 'F' && prevc == '\n') {
                /* HORRIBLE but clever hack suggested by MTR for sendmail-avoidance */
                c = getc(infile);
                if (c == 'r') {
                    c = getc(infile);
                    if (c == 'o') {
                        c = getc(infile);
                        if (c == 'm') {
                            c = getc(infile);
                            if (c == ' ') {
                                /* This is the case we are looking for */
                                fputs("=46rom", outfile);
                                ct += 6;
                            } else {
                                fputs("From", outfile);
                                ct += 4;
                            }
                        } else {
                            fputs("Fro", outfile);
                            ct += 3;
                        }
                    } else {
                        fputs("Fr", outfile);
                        ct += 2;
                    }
                } else {
                    putc('F', outfile);
                    ++ct;
                }
                ungetc(c, infile);
                prevc = 'x'; /* close enough -- printable */
            } else { /* END horrible hack */
                putc(c, outfile);
                ++ct;
                prevc = c;
            }
        }
        if (ct > 72) {
            putc('=', outfile);
            putc('\n', outfile);
            ct = 0;
            prevc = '\n';
        }
    }
    if (ct) {
        putc('=', outfile);
        putc('\n', outfile);
    }
}

/* frombase64 decode a base64 string (b64) into dest */
int frombase64(unsigned const char *b64, unsigned char *dest, int maxlen)
{
	int i, bit, pos;
	bit = 0;
	pos = 0;
	for ( i=0 ; (b64[i]) && (pos<(maxlen-1)) ; i++ ) {
		int left, right;

		/* End of encoded string */
		if ((b64[i] == '?') && (b64[i+1] == '=')) break;

		/* All characters with -1 value are ignored */
		if (index_64[b64[i]] != -1) {
			/* just 6 more bits as we are working up to 2 chars, I'm using pos
			 * which is the position in dest string, while left is the number
			 * of bits modified in the end of the first char and left the
			 * number of bits modified at the beginnning of the second char
			 * the first 6 bits are added to the beginning of the second char,
			 * so we should start with pos = -1
			 */
			pos = (bit+6) / 8 - 1;
			left = (pos+1)*8 - bit;
			right = 6 - left;

			if (left) { /* no need for masking */
				dest[pos] = dest[pos] | (index_64[b64[i]] >> right);
				/* a \0 is meaning end of string */
				if(!dest[pos]) return (pos);
			}
			pos++;
			if (right) {
				dest[pos] = (index_64[b64[i]] << (8-right));
			}
			bit += 6;
		}
	}
	/* OK, if we are here, it means there was no \0 by the end of the
	 * string, so we just add it :-)
	 */
	dest[pos] = 0;
	return pos;
}

void toqps( unsigned const char *orig, char *dest, int maxlen )
{
	int mylen=0, wasqp=0, inqp=0, space=0, qpcr=0, qpstart=0, len, i; 

	if (!orig) {
		dest[0] = '\0';
		return;
	}
	log_printf(5, "Encoding : %s", orig);

	for ( i=0 ;  orig[i] && ( mylen < (maxlen-1) ) ; i++) {
		/* Reading a word at a time */
		if ((orig[i]==' ') || (orig[i]=='\t') ||
			(orig[i]=='\n') || (orig[i]=='\r')) {
			if (inqp) { /* 8 bits chars in last word */
				int j;
				inqp += qpcr ; /* addind special chars */
				len = (i - space + 0) + 2 * inqp;
				/* We have to limit the length of "word" under 75 chars */
				if ((wasqp) && ((mylen+len-qpstart)>70)) {
					if (mylen+4 > maxlen-1) break;
					dest[mylen++] = '?';
					dest[mylen++] = '=';
					while ((orig[space]==' ') || (orig[space]=='\t')) {
						dest[mylen++] = orig[space++];
						len -= 3;
					}
					dest[mylen++] = '\n';
					dest[mylen++] = '\t';
					wasqp = 0;
				}
				if (!wasqp) { /* I have to add QP keyword */
					char buf[100];
					int l;
					const char *s;
					s = get_var("headers-charset");
					if (!s) get_var("headers-charset-frombody");
					if (s)
						l = buffer_printf(buf, sizeof(buf) - 1, "=?%s?Q?", s);
					else
						l = buffer_printf(buf, sizeof(buf) - 1, "=?%s?Q?", "ISO-8859-1");
					if (mylen+l+len+2 > maxlen-1) break;
					memcpy(&(dest[mylen]), buf, l);
					qpstart = mylen;
					mylen += l;
				}

				/* Let's encode the chars (without the last one) */
				for (j=space ; j<i ; j++) {
					if ((orig[j]<=32) || (orig[j]>127) ||
						(orig[j]=='_') || (orig[j]=='?')) {
						dest[mylen++] = '=';
						dest[mylen++] = basis_hex[orig[j]>>4];
						dest[mylen++] = basis_hex[orig[j]&0xF];
					} else {
						dest[mylen++] = orig[j];
					}
				}

				if ((orig[i]=='\n') || (orig[i]=='\r')) {
					/* We end the QP string */
					if (mylen+2 > maxlen-1) break ;
					dest[mylen++] = '?';
					dest[mylen++] = '=';
					wasqp = 0;
					qpcr = 0;
				} else {
					wasqp = 1;
					qpcr = 1;
				}
				inqp = 0;
				space = i;
			} else { /* 7 bits clean word */
				if (wasqp) {
					if (mylen+2 > maxlen-1) break;
					dest[mylen++] = '?';
					dest[mylen++] = '=';
				}
				len = (i - space + 1);
				if (mylen+len > maxlen-1) break;
				memcpy(&(dest[mylen]), &(orig[space]), len);
				mylen += len;
				space = i+1;
				wasqp = 0;
			}

			if ((orig[i]=='\n') || (orig[i]=='\r')) {
				while (space > i) i++;
				while ((orig[i]=='\n') || (orig[i]==' ') || (orig[i]=='\t'))
					dest[mylen++] = orig[i++];
				space = i;
				if (!orig[i]) break;
			}
		} else {
			if ((orig[i]<32) || (orig[i]>127)) inqp++;
			if ((orig[i]=='_') || (orig[i]=='?') || (orig[i]==' ')) qpcr++;
		}
	}

	dest[mylen] = '\0';
	log_printf(5, "toqps : %s", dest);
	return;
}

void fromqp(FILE *infile, FILE *outfile, char **boundaries, int *boundaryct) 
{
    unsigned int c1, c2;
    int sawnewline = 1, neednewline = 0;
    /* The neednewline hack is necessary because the newline leading into 
      a multipart boundary is part of the boundary, not the data */

    while ((c1 = getc(infile)) != EOF) {
        if (sawnewline && boundaries && (c1 == '-')) {
            char buf[200];
            unsigned char *s;

            ungetc(c1, infile);
            read_file(buf, sizeof(buf), infile);
            if (boundaries && (buf[0] == '-') && (buf[1] == '-') &&
                pendingBoundary(buf, boundaries, boundaryct)) {
                return;
            }
            /* Not a boundary, now we must treat THIS line as q-p, sigh */
            if (neednewline) {
                putc('\n', outfile);
                neednewline = 0;
            }
            for (s=(unsigned char *) buf; *s; ++s) {
                if (*s == '=') {
                    if (!*++s) break;
                    if (*s == '\n') {
                        /* ignore it */
                        sawnewline = 1;
                    } else {
                        int i,j;
                        i = *s;
                        if (hexchar(i) == -1) {
                            putc(*s, outfile);
                            continue;
                        }
                        if (!*++s) break;
                        j = *s;
                        if (hexchar(j) == -1) {
                            putc('=', outfile);
                            putc((unsigned char)i, outfile);
                            putc((unsigned char)j, outfile);
                            continue;
                        }
                        c1 = hexchar(i);
                        c2 = hexchar(j);
                        putc(c1<<4 | c2, outfile);
                    }
                } else {
                    putc(*s, outfile);
                }
            }
        } else {
            if (neednewline) {
                putc('\n', outfile);
                neednewline = 0;
            }
            if (c1 == '=') {
                sawnewline = 0;
                c1 = getc(infile);
                if (c1 == '\n') {
                    /* ignore it */
                    sawnewline = 1;
                } else {
                    c2 = getc(infile);
                    if (hexchar(c1) != -1 && hexchar(c2) != -1) {
                        c1 = hexchar(c1);
                        c2 = hexchar(c2);
                        putc(c1<<4 | c2, outfile);
                    } else {
                        putc('=', outfile);
                        putc(c1, outfile);
                        putc(c2, outfile);
                    }
                    if (c2 == '\n') sawnewline = 1;
                }
            } else {
                if (c1 == '\n') {
                    sawnewline = 1;
                    neednewline = 1;
                } else {
                    sawnewline = 0;
                    putc(c1, outfile);
                }
            }
        }
    }
    if (neednewline) {
        putc('\n', outfile);
        neednewline = 0;
    }    
}
