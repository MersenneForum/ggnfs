/* primgen32.c
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

#include <math.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>

#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__)
#include <malloc.h>
#endif

#include "lasieve.h"

#ifdef DEBUG
#define NPrimes16Bit 6542
#endif

#define P32_SIEVESIZE 0x200000
#define PRIMEDIFFS_ALLOCSIZE P32_SIEVESIZE/4
#define PD_COM_ALLOC PRIMEDIFFS_ALLOCSIZE

static unsigned char *primediffs = NULL;
static u32_t NCommonPrimes;

/*********************************************************/
void initprime32(pr32_struct * ps)
/*********************************************************/
{
  if (primediffs == NULL) {
    u32_t i, j, p;

    primediffs = xmalloc(PRIMEDIFFS_ALLOCSIZE);
    memset(primediffs, 1, 1 + USHRT_MAX);
    for (i = 3; i < 0x100; i += 2) {
      if (primediffs[i])
        for (j = i * i; j <= USHRT_MAX; j += i * 2)
          primediffs[j] = 0;
    }
    p = 3;
    for (i = 2, j = 5; j <= USHRT_MAX; j += 2)
      if (primediffs[j]) {
        primediffs[i++] = (j - p) / 2;
        p = j;
      }
    NCommonPrimes = i;
#if 0
    if (NCommonPrimes != NPrimes16Bit)
      Schlendrian("%u!!!\n", NPrimes16Bit);
#endif
  }

  ps->Pind = 0;
  ps->PDiffs = NULL;
  ps->nPrim = 0;
  ps->Prime = 0;
  ps->PDiffs_allocated = 0;
  ps->first_in_sieve = 0;
  ps->use_private = 1;
}

/*********************************************************/
void clearprime32(pr32_struct * ps)
/*********************************************************/
{
  if (ps->PDiffs)
    free(ps->PDiffs);
  ps->PDiffs_allocated = 0;
}

/*********************************************************/
u32_t firstprime32(pr32_struct * ps)
/*********************************************************/
{
  ps->first_in_sieve = 0;
  ps->Prime = 2;
  ps->Pind = 0;
  return 2;
}

/*********************************************************/
u32_t nextprime32(pr32_struct * ps)
/*********************************************************/
{
  if (ps->first_in_sieve) {
    if (ps->Pind < ps->nPrim) {
      ps->Prime += 2 * ps->PDiffs[ps->Pind++];
      return ps->Prime;
    } else {
      if (ps->first_in_sieve < UINT_MAX - 2 * P32_SIEVESIZE) {
        ps->first_in_sieve += 2 * P32_SIEVESIZE;

        {
          unsigned char *sieve;
          u32_t i, M, j, diff, q, dmax = 0, lasti=0, nprim, ssz;

          sieve = xmalloc(P32_SIEVESIZE);
          if (ps->PDiffs_allocated == 0) {
            ps->PDiffs = xmalloc(PD_COM_ALLOC);
            ps->PDiffs_allocated = PD_COM_ALLOC;
          }
          memset(sieve, 1, P32_SIEVESIZE);

          if (ps->first_in_sieve < UINT_MAX - 2 * P32_SIEVESIZE)
            ssz = P32_SIEVESIZE;
          else
            ssz = (UINT_MAX - ps->first_in_sieve) / 2;
          M = 1 + floor(sqrt(ps->first_in_sieve + 2 * ssz));
          for (i = 2, q = 3; q <= M; q += 2 * primediffs[i++]) {
            j = ps->first_in_sieve % q;
            if (j) {
              if (j & 1)
                j = (q - j) / 2;
              else
                j = q - j / 2;
            }
            for (; j < ssz; j += q)
              sieve[j] = 0;
          }
          for (i = 0, nprim = 0; i < ssz; i++)
            if (sieve[i]) {
#if 0
              if (!j) {
                logbook(4, "%u in sieve No\n", q);
                j = 1;
              }
#endif
              if (!nprim) {
                nprim = 1;
                ps->Prime = ps->first_in_sieve + 2 * i;
                lasti = i;
              } else {
                if (nprim > ps->PDiffs_allocated)
                  complain("Should never find that many primes!\n");
                diff = i - lasti;
                lasti = i;
                if (diff > dmax && (dmax = diff) > UCHAR_MAX)
                  complain("Difference %u between consecutive primes!\n",
                           dmax);
                ps->PDiffs[nprim - 1] = diff;
                nprim++;
              }
            }
          free(sieve);
          if (nprim == 0)
            complain("Found no prime\n");
          ps->nPrim = nprim - 1;
          ps->Pind = 0;
          return ps->Prime;

#if 0
          logbook(4, "Largest diff in Sieve was %u\n", dmax);
#endif
        }

      } else
        return 0;
    }
  }
  if (++(ps->Pind) == 1) {
    ps->Prime = 3;
    return 3;
  }
  if (ps->Pind < PD_COM_ALLOC) {
    if (ps->Pind >= NCommonPrimes) {
      unsigned char *sieve;
      u32_t i, M, q, j, diff, dmax = 0, oldprime, start;

      sieve = xmalloc(P32_SIEVESIZE);
      memset(sieve, 1, P32_SIEVESIZE);

      oldprime = ps->Prime;
      start = oldprime + 2;
      M = 1 + floor(sqrt(start + 2 * P32_SIEVESIZE));
      for (i = 2, q = 3; q <= M; q += 2 * primediffs[i++]) {
        j = start % q;
        if (j) {
          if (j & 1)
            j = (q - j) / 2;
          else
            j = q - j / 2;
        }
        for (; j < P32_SIEVESIZE; j += q)
          sieve[j] = 0;
      }
      for (i = 0, j = 0, q = start; i < P32_SIEVESIZE; i++, q += 2)
        if (sieve[i]) {
          if (!j) {
#if 0
            logbook(4, "%u in sieve No\n", q);
#endif
            j = 1;
          }
          diff = (q - oldprime) / 2;
          if (diff > dmax && (dmax = diff) > UCHAR_MAX)
            complain("Difference %u between consecutive primes!\n", dmax);
          if (NCommonPrimes < PD_COM_ALLOC) {
            primediffs[NCommonPrimes++] = diff;
            oldprime = q;
          } else
            break;
        }
      free(sieve);
#if 0
      logbook(4, "Largest diff in Sieve was %u\n", dmax);
#endif
    }

    ps->Prime += 2 * primediffs[ps->Pind];
    return ps->Prime;
  } else {
    if (ps->use_private == 0)
      return 0;
    ps->first_in_sieve = ps->Prime + 2;
    ps->Pind = 0;
    ps->nPrim = 0;

    {
      unsigned char *sieve;
      u32_t i, M, j, diff, q, dmax = 0, lasti=0, nprim, ssz;

      sieve = xmalloc(P32_SIEVESIZE);
      if (ps->PDiffs_allocated == 0) {
        ps->PDiffs = xmalloc(PD_COM_ALLOC);
        ps->PDiffs_allocated = PD_COM_ALLOC;
      }
      memset(sieve, 1, P32_SIEVESIZE);

      if (ps->first_in_sieve < UINT_MAX - 2 * P32_SIEVESIZE)
        ssz = P32_SIEVESIZE;
      else
        ssz = (UINT_MAX - ps->first_in_sieve) / 2;
      M = 1 + floor(sqrt(ps->first_in_sieve + 2 * ssz));
      for (i = 2, q = 3; q <= M; q += 2 * primediffs[i++]) {
        j = ps->first_in_sieve % q;
        if (j) {
          if (j & 1)
            j = (q - j) / 2;
          else
            j = q - j / 2;
        }
        for (; j < ssz; j += q)
          sieve[j] = 0;
      }
      for (i = 0, nprim = 0; i < ssz; i++)
        if (sieve[i]) {
#if 0
          if (!j) {
            logbook(4, "%u in sieve No\n", q);
            j = 1;
          }
#endif
          if (!nprim) {
            nprim = 1;
            ps->Prime = ps->first_in_sieve + 2 * i;
            lasti = i;
          } else {
            if (nprim > ps->PDiffs_allocated)
              complain("Should never find that many primes!\n");
            diff = i - lasti;
            lasti = i;
            if (diff > dmax && (dmax = diff) > UCHAR_MAX)
              complain("Difference %u between consecutive primes!\n", dmax);
            ps->PDiffs[nprim - 1] = diff;
            nprim++;
          }
        }
      free(sieve);
      if (nprim == 0)
        complain("Found no prime\n");
      ps->nPrim = nprim - 1;
      ps->Pind = 0;
      return ps->Prime;

#if 0
      logbook(4, "Largest diff in Sieve was %u\n", dmax);
#endif
    }

  }
}

/*********************************************************/
u32_t pr32_seek(pr32_struct * ps, u32_t lb)
/*********************************************************/
{
  if (lb < 3)
    return firstprime32(ps);
  if (lb % 2 == 0)
    lb++;
  ps->first_in_sieve = lb;

  {
    unsigned char *sieve;
    u32_t i, M, j, diff, q, dmax = 0, lasti=0, nprim, ssz;

    sieve = xmalloc(P32_SIEVESIZE);
    if (ps->PDiffs_allocated == 0) {
      ps->PDiffs = xmalloc(PD_COM_ALLOC);
      ps->PDiffs_allocated = PD_COM_ALLOC;
    }
    memset(sieve, 1, P32_SIEVESIZE);

    if (ps->first_in_sieve < UINT_MAX - 2 * P32_SIEVESIZE)
      ssz = P32_SIEVESIZE;
    else
      ssz = (UINT_MAX - ps->first_in_sieve) / 2;
    M = 1 + floor(sqrt(ps->first_in_sieve + 2 * ssz));
    for (i = 2, q = 3; q <= M; q += 2 * primediffs[i++]) {
      j = ps->first_in_sieve % q;
      if (j) {
        if (j & 1)
          j = (q - j) / 2;
        else
          j = q - j / 2;
      }
      for (; j < ssz; j += q)
        sieve[j] = 0;
    }
    for (i = 0, nprim = 0; i < ssz; i++)
      if (sieve[i]) {
#if 0
        if (!j) {
          logbook(4, "%u in sieve No\n", q);
          j = 1;
        }
#endif
        if (!nprim) {
          nprim = 1;
          ps->Prime = ps->first_in_sieve + 2 * i;
          lasti = i;
        } else {
          if (nprim > ps->PDiffs_allocated)
            complain("Should never find that many primes!\n");
          diff = i - lasti;
          lasti = i;
          if (diff > dmax && (dmax = diff) > UCHAR_MAX)
            complain("Difference %u between consecutive primes!\n", dmax);
          ps->PDiffs[nprim - 1] = diff;
          nprim++;
        }
      }
    free(sieve);
    if (nprim == 0)
      complain("Found no prime\n");
    ps->nPrim = nprim - 1;
    ps->Pind = 0;
    return ps->Prime;

#if 0
    logbook(4, "Largest diff in Sieve was %u\n", dmax);
#endif
  }
}
