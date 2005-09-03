/* input-poly.c
  By Jens Franke.
  6/13/04: Hacked up for use in GGNFS by Chris Monico.
                                                                                                                                                                                                             
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
                                                                                                                                                                                                             
  You should have received a copy of the GNU General Public License along
  with this program; see the file COPYING.  If not, write to the Free
  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <gmp.h>

#include "lasieve.h"

#ifdef _OLD_FORMAT
/*******************************************************/
void input_poly(mpz_t N, mpz_t ** A, i32_t * adeg_ptr, mpz_t ** B,
                i32_t * bdeg_ptr, mpz_t m, FILE * input_file)
/*******************************************************/
{
  char *input_line = NULL;
  size_t input_line_alloc = 0;
  i32_t have_m = 0;

  if (mpz_inp_str(N, input_file, 10) == 0)
    complain("Cannot read number which is to be factored: %m\n");

  *adeg_ptr = -1;
  *bdeg_ptr = -1;
  while (have_m == 0) {
    i32_t grad;
    char *field;

    if (skip_blanks_comments(&input_line, &input_line_alloc, input_file) <= 0)
      complain
        ("Cannot read common root of NFS polynomials from input file\n");
    switch (*input_line) {
      case 'X':
        if (sscanf(input_line + 1, "%d", &grad) == 0)
          complain("Cannot understand input line %s\n", input_line);
        if (grad > *adeg_ptr) {
          i32_t i;

          if (*adeg_ptr >= 0)
            *A = xrealloc(*A, (grad + 1) * sizeof(**A));
          else
            *A = xmalloc((grad + 1) * sizeof(**A));
          for (i = *adeg_ptr + 1; i <= grad; i++)
            mpz_init_set_ui((*A)[i], 0);
          *adeg_ptr = grad;
        }
        strtok(input_line, " \t");
        field = strtok(NULL, " \t");
        if (string2mpz((*A)[grad], field, 10) != 0)
          complain("Cannot understand number %s\n", field);
        break;
      case 'Y':
        if (sscanf(input_line + 1, "%d", &grad) == 0)
          complain("Cannot understand input line %s\n", input_line);
        if (grad > *bdeg_ptr) {
          i32_t i;

          if (*bdeg_ptr >= 0)
            *B = xrealloc(*B, (grad + 1) * sizeof(**B));
          else
            *B = xmalloc((grad + 1) * sizeof(**B));
          for (i = *bdeg_ptr + 1; i <= grad; i++)
            mpz_init_set_ui((*B)[i], 0);
          *bdeg_ptr = grad;
        }
        strtok(input_line, " \t");
        field = strtok(NULL, " \t");
        if (string2mpz((*B)[grad], field, 10) != 0)
          complain("Cannot understand number %s\n", field);
        break;
      case 'M':
        strtok(input_line, " \t");
        field = strtok(NULL, " \t");
        if (string2mpz(m, field, 10) != 0)
          complain("Cannot understand number %s\n", field);
        have_m = 1;
        break;
    }
  }
  if (*adeg_ptr == -1) {
    *adeg_ptr = 1;
    *A = xmalloc(2 * sizeof(**A));
    mpz_init_set_ui((*A)[1], 1);
    mpz_init((*A)[0]);
    mpz_neg((*A)[0], m);
  }
  if (*bdeg_ptr == -1) {
    *bdeg_ptr = 1;
    *B = xmalloc(2 * sizeof(**B));
    mpz_init_set_ui((*B)[1], 1);
    mpz_init((*B)[0]);
    mpz_neg((*B)[0], m);
  }
  if (*adeg_ptr == 0 || *bdeg_ptr == 0)
    complain("Polynomials of degree zero are not allowed\n");

  {
    mpz_t x;
    i32_t i;

    if (mpz_sgn(*(*A + *adeg_ptr)) == 0) {
      complain("Leading coefficient (degree %u) vanishes\n", *adeg_ptr);
    }
    if (mpz_sgn(*(*B + *bdeg_ptr)) == 0) {
      complain("Leading coefficient (degree %u) vanishes\n", *bdeg_ptr);
    }
    for (i = 1, mpz_init_set(x, (*A)[*adeg_ptr]); i <= *adeg_ptr; i++) {
      mpz_mul(x, x, m);
      mpz_add(x, x, (*A)[*adeg_ptr - i]);
    }
    mpz_fdiv_r(x, x, N);
    if (mpz_sgn(x) != 0) {
      mpz_out_str(stderr, 10, m);
      complain(" not a root of first poly\n");
    }
    for (i = 1, mpz_set(x, (*B)[*bdeg_ptr]); i <= *bdeg_ptr; i++) {
      mpz_mul(x, x, m);
      mpz_add(x, x, (*B)[*bdeg_ptr - i]);
    }
    mpz_fdiv_r(x, x, N);
    if (mpz_sgn(x) != 0) {
      mpz_out_str(stderr, 10, m);
      complain(" not a root of second poly\n");
    }
    mpz_clear(x);
  }

  free(input_line);
}

#else
/*******************************************************/
void input_poly(mpz_t N, mpz_t ** A, i32_t *adeg, mpz_t ** B,
                i32_t *bdeg, mpz_t m, FILE *fp)
/*******************************************************/
{ char  token[256], value[256], thisLine[1024];
  int   i, j, cont=1;
  mpz_t tmp, tmp2, mpow;

  *adeg = *bdeg = 0;                                                    
  *A = xmalloc(8*sizeof(**A)); /* plenty o' room. */
  *B = xmalloc(8*sizeof(**B));
  for (i=0; i<8; i++) {
    mpz_init_set_ui((*A)[i], 0);
    mpz_init_set_ui((*B)[i], 0);
  }
  while (cont) {
    thisLine[0] = 0;
    fgets(thisLine, 1023, fp);
    if ((sscanf(thisLine, "%255s %255s", token, value)==2) &&
                (thisLine[0] != '#')) {
	  token[sizeof(token)-1] = 0;
      if (strncmp(token, "n:", 2)==0) {
        mpz_set_str(N, value, 10);
      } else if (strncmp(token, "m:", 2)==0) {
        mpz_set_str(m, value, 10);
      } else if ((token[0]=='c') && (token[1] >= '0') && (token[1] <= '6')) {
        mpz_set_str((*A)[token[1]-'0'], value, 10);
        *adeg = MAX(*adeg, token[1]-'0');
      } else if ((token[0]=='Y') && (token[1] >= '0') && (token[1] <= '6')) {
        mpz_set_str((*B)[token[1]-'0'], value, 10);
        *bdeg = MAX(*bdeg, token[1]-'0');
      } else if (strncmp(token, "END_POLY", 8)==0) {
        cont=0;
      }
    }
    if (feof(fp)) cont=0;
  }
  if (*bdeg == 0) {
    mpz_set_ui((*B)[1], 1);
    mpz_neg((*B)[0], m);
    *bdeg=1;
  }

  /* Verify the polynomials: */
  mpz_init(tmp); mpz_init(tmp2); mpz_init(mpow);
  mpz_set_ui(tmp, 0);
  mpz_set_ui(mpow, 1);
  
  if (*bdeg) {
    mpz_neg(m, (*B)[0]);  /* m for temporary use */
    for(i=0; i<=*adeg; i++) {
      mpz_mul(tmp2, mpow, (*A)[i]);
        for(j=i; j <= *adeg; j++)
          mpz_mul(tmp2, tmp2, (*B)[1]);
      mpz_add(tmp, tmp, tmp2);
      mpz_mul(mpow, mpow, m);
    }
  } else {
    for (i=*adeg; i>=0; i--) {
      mpz_mul(tmp, tmp, m);
      mpz_add(tmp, tmp, (*A)[i]);
    }
  }
  mpz_mod(tmp, tmp, N);
  if (mpz_sgn(tmp)) {
    printf("Error: the polynomials don't have a common root:\n");
    for (i=0; i<=*adeg; i++) 
      printf("c%d: %s\n", i, mpz_get_str(token, 10, (*A)[i]));
    for (i=0; i<=*bdeg; i++) 
      printf("Y%d: %s\n", i, mpz_get_str(token, 10, (*B)[i]));
    exit(-1);
  }
  mpz_clear(tmp2); mpz_clear(mpow); mpz_clear(tmp);
}
#endif
