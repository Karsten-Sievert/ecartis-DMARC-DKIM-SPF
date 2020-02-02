/*
 * Copyright (C) 1996-2000 Michael R. Elkins <me@cs.hmc.edu>
 * Copyright (C) 2000-2001 Edmund Grimley Evans <edmundo@rano.org>
 * 
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software
 *     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 */ 

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "core.h"
#include "fileapi.h"
#include "variables.h"


#define strfcpy(A,B,C) strncpy(A,B,C), *(A+(C)-1)=0

void *safe_malloc (size_t siz);
void safe_free (void **p);

int Index_hex[128] = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
    -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

int Index_64[128] = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
    52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
    -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
    -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
};

#define hexval(c) Index_hex[(unsigned int)(c)]
#define base64val(c) Index_64[(unsigned int)(c)]

/* Content-Transfer-Encoding */
enum
{
  ENCOTHER,
  ENC7BIT,
  ENC8BIT,
  ENCQUOTEDPRINTABLE,
  ENCBASE64,
  ENCBINARY,
  ENCUUENCODED
};

#define MAXCHARSETLEN 64



static int rfc2047_decode_word (char *d, const char *s, size_t len)
{
  const char *pp = s, *pp1;
  char *pd, *d0;
  const char *t, *t1;
  int enc = 0, count = 0, c1, c2, c3, c4;
  char *charset = NULL;

  pd = d0 = safe_malloc (strlen (s));

  for (pp = s; (pp1 = strchr (pp, '?')); pp = pp1 + 1)
  {
    count++;
    switch (count)
    {
      case 2:
	/* ignore language specification a la RFC 2231 */        
	t = pp1;
        if ((t1 = memchr (pp, '*', t - pp)))
	  t = t1;
	charset = safe_malloc (t - pp + 1);
	memcpy (charset, pp, t - pp);
	charset[t-pp] = '\0';
	break;
      case 3:
	if (toupper (*pp) == 'Q')
	  enc = ENCQUOTEDPRINTABLE;
	else if (toupper (*pp) == 'B')
	  enc = ENCBASE64;
	else
	{
	  safe_free ((void **) &charset);
	  safe_free ((void **) &d0);
	  return (-1);
	}
	break;
      case 4:
	if (enc == ENCQUOTEDPRINTABLE)
	{
	  while (pp < pp1 && len > 0)
	  {
	    if (*pp == '_')
	    {
	      *pd++ = ' ';
	      len--;
	    }
	    else if (*pp == '=')
	    {
	      if (pp[1] == 0 || pp[2] == 0)
		break;	/* something wrong */
	      *pd++ = (hexval(pp[1]) << 4) | hexval(pp[2]);
	      len--;
	      pp += 2;
	    }
	    else
	    {
	      *pd++ = *pp;
	      len--;
	    }
	    pp++;
	  }
	  *pd = 0;
	}
	else if (enc == ENCBASE64)
	{
	  while (pp < pp1 && len > 0)
	  {
	    if (pp[0] == '=' || pp[1] == 0 || pp[1] == '=')
	      break;  /* something wrong */
	    c1 = base64val(pp[0]);
	    c2 = base64val(pp[1]);
	    *pd++ = (c1 << 2) | ((c2 >> 4) & 0x3);
	    if (--len == 0) break;
	    
	    if (pp[2] == 0 || pp[2] == '=') break;

	    c3 = base64val(pp[2]);
	    *pd++ = ((c2 & 0xf) << 4) | ((c3 >> 2) & 0xf);
	    if (--len == 0)
	      break;

	    if (pp[3] == 0 || pp[3] == '=')
	      break;

	    c4 = base64val(pp[3]);
	    *pd++ = ((c3 & 0x3) << 6) | c4;
	    if (--len == 0)
	      break;

	    pp += 4;
	  }
	  *pd = 0;
	}
	break;
    }
  }
  
  if (charset && strlen(charset) < MAXCHARSETLEN) {
    if (!get_var("headers-charset")) {
      log_printf(5, "Setting headers-charset : %s\n", charset);
      set_var("headers-charset", charset, VAR_GLOBAL);
  }
  } else {
    const char *s;
    s = get_var("headers-charset-frombody");
    if (s) {
      log_printf(5, "Using charset frombody in header: %s\n", s);
      set_var("headers-charset", s, VAR_GLOBAL);
    }
  }
  strfcpy (d, d0, len);
  safe_free ((void **) &charset);
  safe_free ((void **) &d0);
  return (0);
}

/*
 * Find the start and end of the first encoded word in the string.
 * We use the grammar in section 2 of RFC 2047, but the "encoding"
 * must be B or Q. Also, we don't require the encoded word to be
 * separated by linear-white-space (section 5(1)).
 */
static const char *find_encoded_word (const char *s, const char **x)
{
  const char *p, *q;

  q = s;
  while ((p = strstr (q, "=?")))
  {
    for (q = p + 2;
	 0x20 < *q && *q < 0x7f && !strchr ("()<>@,;:\"/[]?.=", *q);
	 q++)
      ;
    if (q[0] != '?' || !strchr ("BbQq", q[1]) || q[2] != '?')
      continue;
    for (q = q + 3; 0x20 < *q && *q < 0x7f && *q != '?'; q++)
      ;
    if (q[0] != '?' || q[1] != '=')
    {
      --q;
      continue;
    }

    *x = q + 2;
    return p;
  }

  return 0;
}

/* try to decode anything that looks like a valid RFC2047 encoded
 * header field, ignoring RFC822 parsing rules
 */
void rfc2047_decode (const char *orig, char *dest, int maxlen)
{
  const char *p, *q;
  size_t n;
  int found_encoded = 0;
  char *d0, *d;
  const char *s = orig;
  size_t dlen;

  if (!s || !*s)
    return;

  dlen = 4 * strlen (s); /* should be enough */
  d = d0 = safe_malloc (dlen + 1);

  while (*s && dlen > 0)
  {
    if (!(p = find_encoded_word (s, &q)))
    {
      /* no encoded words */
      strncpy (d, s, dlen);
      d += dlen;
      break;
    }

    if (p != s)
    {
      n = (size_t) (p - s);
      /* ignore spaces between encoded words */
      if (!found_encoded || strspn (s, " \t\r\n") != n)
      {
	if (n > dlen)
	  n = dlen;
	memcpy (d, s, n);
	d += n;
	dlen -= n;
      }
    }

    rfc2047_decode_word (d, p, dlen);
    found_encoded = 1;
    s = q;
    n = strlen (d);
    dlen -= n;
    d += n;
  }
  *d = 0;

  if (strlen(d0) > (unsigned int)maxlen) {
      d0[maxlen-1] = '\0';
  }
  strcpy(dest, d0);
  log_printf(5, "Subject: %s\n", dest);
  safe_free((void **)&d0);
}

void *safe_malloc (size_t siz)
{   
    void *p;

    if (siz == 0)
	return 0; 
    if ((p = (void *) malloc (siz)) == 0)
    {
	exit(1); /* ??? */
    }
    return (p);
} 

void safe_free (void **p) 
{
    if (*p)
    {
	free (*p);
	*p = 0;
    }
} 
