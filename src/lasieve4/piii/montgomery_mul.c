/* montgomery_mul.c
  Written by T. Kleinjung. 
  6/13/04: Hacked up for inclusion in GGNFS by Chris Monico.

  Copyright (C) 2001 Jens Franke, T. Kleinjung.
  This file is part of gnfs4linux, distributed under the terms of the 
  GNU General Public Licence and WITHOUT ANY WARRANTY.

  You should have received a copy of the GNU General Public License along
  with this program; see the file COPYING.  If not, write to the Free
  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <gmp.h>
#include "lasieve-asm.h"
#include "../lasieve.h"


ulong montgomery_inv_n;
ulong *montgomery_modulo_n;
ulong montgomery_modulo_R2[NMAX_ULONGS], montgomery_modulo_R4[NMAX_ULONGS];
ulong montgomery_ulongs;
mpz_t montgomery_gmp_help;

/* CJM: The best thing would be to declare a macro, ASM(x) which
   evaluates to asm(x) on gcc, empty on others. This way we would
   have just the one collection of prototypes here. I'll get to it.
*/
#ifndef _MSC_VER
/* function pointers */
void (*asm_mulmod) (ulong *, ulong *, ulong *) = NULL;
extern void asm_mulm64(ulong *, ulong *, ulong *) asm("asm_mulm64");
extern void asm_mulm96(ulong *, ulong *, ulong *) asm("asm_mulm96");
extern void asm_mulm128(ulong *, ulong *, ulong *) asm("asm_mulm128");

void (*asm_squmod) (ulong *, ulong *) = NULL;
extern void asm_sqm64(ulong *, ulong *) asm("asm_sqm64");
extern void asm_sqm96(ulong *, ulong *) asm("asm_sqm96");
extern void asm_sqm128(ulong *, ulong *) asm("asm_sqm128");

void (*asm_add2) (ulong *, ulong *) = NULL;
extern void asm_add64(ulong *, ulong *) asm("asm_add64");
extern void asm_add96(ulong *, ulong *) asm("asm_add96");
extern void asm_add128(ulong *, ulong *) asm("asm_add128");

void (*asm_diff) (ulong *, ulong *, ulong *) = NULL;
extern void asm_diff64(ulong *, ulong *, ulong *) asm("asm_diff64");
extern void asm_diff96(ulong *, ulong *, ulong *) asm("asm_diff96");
extern void asm_diff128(ulong *, ulong *, ulong *) asm("asm_diff128");

void (*asm_sub) (ulong *, ulong *, ulong *) = NULL;
extern void asm_sub64(ulong *, ulong *, ulong *) asm("asm_sub64");
extern void asm_sub96(ulong *, ulong *, ulong *) asm("asm_sub96");
extern void asm_sub128(ulong *, ulong *, ulong *) asm("asm_sub128");

void (*asm_add2_ui) (ulong *, ulong) = NULL;
extern void asm_add64_ui(ulong *, ulong) asm("asm_add64_ui");
extern void asm_add96_ui(ulong *, ulong) asm("asm_add96_ui");
extern void asm_add128_ui(ulong *, ulong) asm("asm_add128_ui");

void (*asm_zero) (ulong *, ulong *) = NULL;
extern void asm_zero64(ulong *, ulong *) asm("asm_zero64");
extern void asm_zero96(ulong *, ulong *) asm("asm_zero96");
extern void asm_zero128(ulong *, ulong *) asm("asm_zero128");

void (*asm_copy) (ulong *, ulong *) = NULL;
extern void asm_copy64(ulong *, ulong *) asm("asm_copy64");
extern void asm_copy96(ulong *, ulong *) asm("asm_copy96");
extern void asm_copy128(ulong *, ulong *) asm("asm_copy128");

void (*asm_sub_n) (ulong *, ulong *) = NULL;
extern void asm_sub_n64(ulong *, ulong *) asm("asm_sub_n64");
extern void asm_sub_n96(ulong *, ulong *) asm("asm_sub_n96");
extern void asm_sub_n128(ulong *, ulong *) asm("asm_sub_n128");

void (*asm_half) (ulong *) = NULL;
extern void asm_half64(ulong *) asm("asm_half64");
extern void asm_half96(ulong *) asm("asm_half96");
extern void asm_half128(ulong *) asm("asm_half128");

#else

/* function pointers */
void (*asm_mulmod) (ulong *, ulong *, ulong *) = NULL;
#if !defined( ASM_SUBS_DECLARED )
extern void asm_mulm64(ulong *, ulong *, ulong *)
extern void asm_mulm96(ulong *, ulong *, ulong *);
extern void asm_mulm128(ulong *, ulong *, ulong *);
#endif

void (*asm_squmod) (ulong *, ulong *) = NULL;
#if !defined( ASM_SUBS_DECLARED )
extern void asm_sqm64(ulong *, ulong *);
extern void asm_sqm96(ulong *, ulong *);
extern void asm_sqm128(ulong *, ulong *);
#endif

void (*asm_add2) (ulong *, ulong *) = NULL;
#if !defined( ASM_SUBS_DECLARED )
extern void asm_add64(ulong *, ulong *);
extern void asm_add96(ulong *, ulong *);
extern void asm_add128(ulong *, ulong *);
#endif

void (*asm_diff) (ulong *, ulong *, ulong *) = NULL;
#if !defined( ASM_SUBS_DECLARED )
extern void asm_diff64(ulong *, ulong *, ulong *);
extern void asm_diff96(ulong *, ulong *, ulong *);
extern void asm_diff128(ulong *, ulong *, ulong *);
#endif

void (*asm_sub) (ulong *, ulong *, ulong *) = NULL;
#if !defined( ASM_SUBS_DECLARED )
extern void asm_sub64(ulong *, ulong *, ulong *);
extern void asm_sub96(ulong *, ulong *, ulong *);
extern void asm_sub128(ulong *, ulong *, ulong *);
#endif

void (*asm_add2_ui) (ulong *, ulong) = NULL;
#if !defined( ASM_SUBS_DECLARED )
extern void asm_add64_ui(ulong *, ulong);
extern void asm_add96_ui(ulong *, ulong);
extern void asm_add128_ui(ulong *, ulong);
#endif

void (*asm_zero) (ulong *, ulong *) = NULL;
#if !defined( ASM_SUBS_DECLARED )
extern void asm_zero64(ulong *, ulong *);
extern void asm_zero96(ulong *, ulong *);
extern void asm_zero128(ulong *, ulong *);
#endif

void (*asm_copy) (ulong *, ulong *) = NULL;
#if !defined( ASM_SUBS_DECLARED )
extern void asm_copy64(ulong *, ulong *);
extern void asm_copy96(ulong *, ulong *);
extern void asm_copy128(ulong *, ulong *);
#endif

void (*asm_sub_n) (ulong *, ulong *) = NULL;
#if !defined( ASM_SUBS_DECLARED )
extern void asm_sub_n64(ulong *, ulong *);
extern void asm_sub_n96(ulong *, ulong *);
extern void asm_sub_n128(ulong *, ulong *);
#endif

void (*asm_half) (ulong *) = NULL;
#if !defined( ASM_SUBS_DECLARED )
extern void asm_half64(ulong *);
extern void asm_half96(ulong *);
extern void asm_half128(ulong *);
#endif

#endif



/***************************************************/
void init_montgomery_R2_2()
/***************************************************/
{ long i;
  ulong h[2], c;

  h[0] = 1;
  h[1] = 0;
  for (i = 0; i < 128; i++) {
    if (h[1] & 0x80000000)
      c = 1;
    else
      c = 0;
    h[1] = (h[1] << 1) | (h[0] >> 31);
    h[0] <<= 1;
    if (!c) {
      if (h[1] < montgomery_modulo_n[1])
        continue;
      if (h[1] == montgomery_modulo_n[1]) {
        if (h[0] < montgomery_modulo_n[0])
          continue;
      }
    }
    if (h[0] < montgomery_modulo_n[0])
      c = 1;
    else
      c = 0;
    h[0] -= montgomery_modulo_n[0];
    h[1] -= montgomery_modulo_n[1];
    h[1] -= c;
  }
  montgomery_modulo_R2[0] = h[0];
  montgomery_modulo_R2[1] = h[1];
}

/***************************************************/
void init_montgomery_R2_3()
/***************************************************/
{ long i;
  ulong h[3], c;

  h[0] = 1;
  h[1] = 0;
  h[2] = 0;
  for (i = 0; i < 192; i++) {
    if (h[2] & 0x80000000)
      c = 1;
    else
      c = 0;
    h[2] = (h[2] << 1) | (h[1] >> 31);
    h[1] = (h[1] << 1) | (h[0] >> 31);
    h[0] <<= 1;
    if (!c) {
      if (h[2] < montgomery_modulo_n[2])
        continue;
      if (h[2] == montgomery_modulo_n[2]) {
        if (h[1] < montgomery_modulo_n[1])
          continue;
        if (h[1] == montgomery_modulo_n[1]) {
          if (h[0] < montgomery_modulo_n[0])
            continue;
        }
      }
    }
    if (h[0] < montgomery_modulo_n[0])
      c = 1;
    else
      c = 0;
    h[0] -= montgomery_modulo_n[0];
    if (c) {
      if (!h[1])
        h[2]--;
      h[1]--;
    }
    if (h[1] < montgomery_modulo_n[1])
      c = 1;
    else
      c = 0;
    h[1] -= montgomery_modulo_n[1];
    h[2] -= montgomery_modulo_n[2];
    h[2] -= c;
  }
  montgomery_modulo_R2[0] = h[0];
  montgomery_modulo_R2[1] = h[1];
  montgomery_modulo_R2[2] = h[2];
}

/***************************************************/
void init_montgomery_R2()
/***************************************************/
{ long i, j;
  ulong h[NMAX_ULONGS], c;

  h[0] = 1;
  for (i = 1; i < montgomery_ulongs; i++)
    h[i] = 0;
  for (i = 0; i < 64 * montgomery_ulongs; i++) {
    if (h[montgomery_ulongs - 1] & 0x80000000)
      c = 1;
    else
      c = 0;
    for (j = montgomery_ulongs - 1; j > 0; j--)
      h[j] = (h[j] << 1) | (h[j - 1] >> 31);
    h[0] <<= 1;
    if (!c) {
      for (j = montgomery_ulongs - 1; j >= 0; j--)
        if (h[j] != montgomery_modulo_n[j])
          break;
      if ((j >= 0) && (h[j] < montgomery_modulo_n[j]))
        continue;
      /* h[] is smaller than montgomery_modulo_n[] */
    }
    /* h[]-=montgomery_modulo_n[]: */
#if 1
    asm_sub_n(h, montgomery_modulo_n);
#else
    c = 0;
    for (j = 0; j < montgomery_ulongs; j++) {
      if (c) {
        if (montgomery_modulo_n[j] + 1 == 0)
          c = 1;
        else {
          if (h[j] < (montgomery_modulo_n[j] + 1))
            c = 1;
          else
            c = 0;
          h[j] -= (montgomery_modulo_n[j] + 1);
        }
      } else {
        if (h[j] < montgomery_modulo_n[j])
          c = 1;
        else
          c = 0;
        h[j] -= montgomery_modulo_n[j];
      }
    }
    if (c)
      complain("init_montgomery_R2\n");
#endif
  }
  for (j = 0; j < montgomery_ulongs; j++)
    montgomery_modulo_R2[j] = h[j];
}

/***************************************************/
ulong montgomery_inverse()
/***************************************************/
{ ulong v1, v2, q, b, p;

  if (montgomery_modulo_n[0] == 1)
    return -1;
  v1 = 0;
  v2 = 1;
  b = -montgomery_modulo_n[0];
  p = montgomery_modulo_n[0];   /* 2^32-b */
  v1 += v2;
  if (b >= p) {
    q = b / p;
    v2 += q * v1;
    q *= p;
    b -= q;
  }
  while (b > 1) {
    p -= b;
    v1 += v2;
    if (p >= b) {
      p -= b;
      v1 += v2;
      if (p >= b) {
        q = p / b;
        v1 += q * v2;
        q *= b;
        p -= q;
      }
    }
    if (p <= 1) {
      v2 = -v1;                 /* 2^32-v1 */
      break;
    }
    b -= p;
    v2 += v1;
    if (b >= p) {
      b -= p;
      v2 += v1;
      if (b >= p) {
        q = b / p;
        v2 += q * v1;
        q *= p;
        b -= q;
      }
    }
  }
  return v2;
}

/***************************************************/
int set_montgomery_multiplication(mpz_t n)
/***************************************************/
{ ulong bl, old;
  long j;

  old = montgomery_ulongs;
  bl = mpz_sizeinbase(n, 2);
  if (bl > 32 * NMAX_ULONGS)
    return 0;
  for (j = 1; j < NMAX_ULONGS; j++)
    montgomery_modulo_n[j] = 0;
  montgomery_modulo_n[0] = mpz_get_ui(n);
  montgomery_ulongs = 1;
  if (bl > 32)
    mpz_fdiv_q_2exp(montgomery_gmp_help, n, 32);
  while (bl > 32 * montgomery_ulongs) {
    montgomery_modulo_n[montgomery_ulongs] = mpz_get_ui(montgomery_gmp_help);
    mpz_fdiv_q_2exp(montgomery_gmp_help, montgomery_gmp_help, 32);
    montgomery_ulongs++;
  }
  if (montgomery_ulongs < 2)
    montgomery_ulongs = 2;      /* have no 32 bit functions */
  if (montgomery_ulongs > NMAX_ULONGS)
    complain("set_montgomery_multiplication\n");
#ifdef MPQS_ZEIT
  zeitA(2);
#endif
  montgomery_inv_n = montgomery_inverse();
#ifdef MPQS_ZEIT
  zeitB(2);
#endif
  if (montgomery_inv_n * montgomery_modulo_n[0] + 1) {
    fprintf(stderr, "init_montgomery_multiplication failed %lu %lu\n",
            montgomery_inv_n, montgomery_modulo_n[0]);
    return 0;
  }
  if (old != montgomery_ulongs) {
    if (montgomery_ulongs == 2) {
      asm_mulmod = asm_mulm64;
      asm_squmod = asm_sqm64;
      asm_add2 = asm_add64;
      asm_diff = asm_diff64;
      asm_sub = asm_sub64;
      asm_add2_ui = asm_add64_ui;
      asm_zero = asm_zero64;
      asm_copy = asm_copy64;
      asm_sub_n = asm_sub_n64;
      asm_half = asm_half64;
    } else if (montgomery_ulongs == 3) {
      asm_mulmod = asm_mulm96;
      asm_squmod = asm_sqm96;
      asm_add2 = asm_add96;
      asm_diff = asm_diff96;
      asm_sub = asm_sub96;
      asm_add2_ui = asm_add96_ui;
      asm_zero = asm_zero96;
      asm_copy = asm_copy96;
      asm_sub_n = asm_sub_n96;
      asm_half = asm_half96;
    } else if (montgomery_ulongs == 4) {
      asm_mulmod = asm_mulm128;
      asm_squmod = asm_sqm128;
      asm_add2 = asm_add128;
      asm_diff = asm_diff128;
      asm_sub = asm_sub128;
      asm_add2_ui = asm_add128_ui;
      asm_zero = asm_zero128;
      asm_copy = asm_copy128;
      asm_sub_n = asm_sub_n128;
      asm_half = asm_half128;
    } else
      Schlendrian("set_montgomery_multiplication\n");
  }
#ifdef MPQS_ZEIT
  zeitA(9);
#endif
  if (montgomery_ulongs == 2)
    init_montgomery_R2_2();
  else if (montgomery_ulongs == 3)
    init_montgomery_R2_3();
  else
    init_montgomery_R2();
  asm_mulmod(montgomery_modulo_R4, montgomery_modulo_R2,
             montgomery_modulo_R2);
#ifdef MPQS_ZEIT
  zeitB(9);
#endif
  return 1;
}

/***************************************************/
void init_montgomery_multiplication()
/***************************************************/
{
  mpz_init(montgomery_gmp_help);
  montgomery_modulo_n = (ulong *) xmalloc(NMAX_ULONGS * sizeof(ulong));
  montgomery_ulongs = 0;
}
