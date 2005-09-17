/* siever-config.h
  Hacked up for inclusion in GGNFS by Chris Monico.

  Copyright (C) 2000 Jens Franke
  This file is part of mpqs4linux, distributed under the terms of the 
  GNU General Public Licence and WITHOUT ANY WARRANTY.

  You should have received a copy of the GNU General Public License along
  with this program; see the file COPYING.  If not, write to the Free
  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.
*/
#ifndef _LASIEVE_ASM_H
#define _LASIEVE_ASM_H

#include <limits.h>
#include <gmp.h>
#include <sys/types.h>

#include "if.h"

#define L1_BITS 14
#define ULONG_RI
#define HAVE_CMOV
#define HAVE_SSIMD

#define PREINVERT
#define HAVE_ASM_GETBC
#define ASM_SCHEDSIEVE
#define modinv32 asm_modinv32 

#undef GGNFS_BIGENDIAN

#if 1
#define N_PRIMEBOUNDS 7
#else
#define N_PRIMEBOUNDS 1
#endif
/*
extern const uint32_t *schedule_primebounds;
extern const uint32_t *schedule_sizebits;
*/

extern const uint32_t schedule_primebounds[N_PRIMEBOUNDS];
extern const uint32_t schedule_sizebits[N_PRIMEBOUNDS];
void siever_init(void);

#if defined (_MSC_VER)
    #define GGNFS_x86_32_MSCASM_MMX
#elif defined(__GNUC__) && defined(__x86_64__)
    #define GGNFS_x86_64_ATTASM_MMX
#elif defined(__GNUC__) && defined(__i386__)
    #define GGNFS_x86_32_ATTASM_MMX
#endif

#if !defined(_MSC_VER)
#define NAME(_a) asm(_a)
#else
#define NAME(_a) 
#endif

#if defined(GGNFS_x86_32_MSCASM_MMX)

#define inline __inline

volatile extern u32_t modulo32;
u32_t asm_modinv32(u32_t x);

static inline u32_t modsq32(u32_t x)
{	u32_t res;

	__asm
	{	mov		eax,[x]
		mul		[x]
		div		[modulo32]
		mov		res,edx
	}
	return res;
}

static inline u32_t modmul32(u32_t x, u32_t y)
{	u32_t res;
	
	__asm
	{	mov		eax,[x]
		mul		[y]
		div		[modulo32]
		mov		res,edx
	}
	return res;
}

#ifdef HAVE_CMOV

static inline u32_t modadd32(u32_t x, u32_t y)
{	u32_t res;
	
	__asm
	{
		xor		edx,edx
		mov		ecx,[y]
		add		ecx,[x]
		cmovc	edx,[modulo32]
	    cmp		ecx,[modulo32]
		cmovae	edx,[modulo32]
		sub		ecx,edx
		mov		[res],ecx
	}
	return res;
}

static inline u32_t modsub32(u32_t subtrahend, u32_t minuend)
{	u32_t res;
  
	__asm
	{	xor		edx,edx
		mov		ecx,[subtrahend]
		sub		ecx,[minuend]
		cmovb	edx,[modulo32]
		add		ecx,edx
		mov		[res],ecx
	}
	return res;
}

#else

static inline u32_t modadd32(u32_t x,u32_t y)
{	u32_t res;
	
	__asm
	{	
		mov		ecx,[x]
		add		ecx,[y]
		jc		l1f
		cmp		ecx,[modulo32]
		jb		l2f
l1f:	sub		ecx,[modulo32]
l2f:	mov		[res],ecx
	}
	return res;
}

static inline u32_t modsub32(u32_t subtrahend,u32_t minuend)
{	u32_t res;

	__asm
	{
		mov		ecx,[subtrahend]
		sub		ecx,[minuend]
		jae		l1f
		addl	ecx,[modulo32]
l1f:	mov		[res],ecx
	}
	return res;
}

#endif

#elif defined(GGNFS_x86_32_ATTASM_MMX)

/* 32bit.h */
extern volatile u32_t modulo32 NAME("modulo32");
u32_t asm_modinv32(u32_t x) NAME("asm_modinv32");

static inline u32_t modsq32(u32_t x)
{ u32_t res,clobber;
  __asm__ volatile ("mull %%eax\n"
	   "divl modulo32" : "=d" (res), "=a" (clobber) : "a" (x) : "cc" );
  return res;
}

static inline u32_t modmul32(u32_t x,u32_t y)
{ u32_t res,clobber;
  __asm__ volatile ("mull %%ecx\n"
	   "divl modulo32" : "=d" (res), "=a" (clobber) : "a" (x), "c" (y) :
	   "cc");
  return res;
}

static inline u32_t modadd32(u32_t x,u32_t y)
{ u32_t res;
#ifdef HAVE_CMOV
  __asm__ volatile ("xorl %%edx,%%edx\n"
	   "addl %%eax,%%ecx\n"
	   "cmovc modulo32,%%edx\n"
	   "cmpl modulo32,%%ecx\n"
	   "cmovae modulo32,%%edx\n"
	   "subl %%edx,%%ecx\n"
	   "2:\n" : "=c" (res) : "a" (x), "c" (y) : "%edx", "cc");
#else
  __asm__ volatile ("addl %%eax,%%ecx\n"
	   "jc 1f\n"
	   "cmpl modulo32,%%ecx\n"
	   "jb 2f\n"
	   "1:\n"
	   "subl modulo32,%%ecx\n"
	   "2:\n" : "=c" (res) : "a" (x), "c" (y) : "cc");
#endif
  return res;
}

static inline u32_t modsub32(u32_t subtrahend,u32_t minuend)
{ u32_t res;
#ifdef HAVE_CMOV
  __asm__ volatile ("xorl %%edx,%%edx\n"
	   "subl %%eax,%%ecx\n"
	   "cmovbl modulo32,%%edx\n"
	   "addl %%edx,%%ecx\n"
	   "1:" : "=c" (res) : "a" (minuend), "c" (subtrahend) : "%edx", "cc");
#else
  __asm__ volatile ("subl %%eax,%%ecx\n"
	   "jae 1f\n"
	   "addl modulo32,%%ecx\n"
	   "1:" : "=c" (res) : "a" (minuend), "c" (subtrahend) : "cc" );
#endif
  return res;
}

#else
    #error Unsupported assembler model!
#endif

/* lasched.h */
u32_t*lasched(u32_t*,u32_t*,u32_t*,u32_t,u32_t**,u32_t,u32_t);

/* medsched.h */
u32_t*medsched(u32_t*,u32_t*,u32_t*,u32_t**,u32_t,u32_t);

/* montgomery_mul.h */
#define NMAX_ULONGS   4

/*  montgomery_mul.c */
void init_montgomery_multiplication();
int set_montgomery_multiplication(mpz_t n);

/* sieve-from-sched.S */
int schedsieve(unsigned char x, unsigned char *sieve_interval, 
               u16_t *schedule_ptr, u16_t *sptr_ub) NAME("schedsieve");

/* basemath.c */
int asm_cmp(uint32_t * a, uint32_t * b);
int asm_cmp64(uint32_t * a, uint32_t * b);

/* gcd32.c */
u32_t gcd32(u32_t x, u32_t y);

/* ri-aux.s */
uint32_t asm_getbc(u32_t r, u32_t p, u32_t A, u32_t *b, u32_t *s, u32_t *c, u32_t *t) NAME("asm_getbc");

/* mpqs_sieve.s */
int asm_sieve() NAME("asm_sieve");

/* mpqs_eval.s */
uint16_t asm_evaluate(uint32_t *, uint32_t *, uint16_t *, int32_t) NAME("asm_evaluate");

/* mpqs_td.s */
int asm_td(uint16_t *, uint16_t, uint64_t*, uint32_t*) NAME("asm_td");

/* misc arithmetic files: */

#if !defined(_MSC_VER)

extern void asm_mulm64(uint32_t *, uint32_t *, uint32_t *) NAME("asm_mulm64");
extern void asm_mulm96(uint32_t *, uint32_t *, uint32_t *) NAME("asm_mulm96");
extern void asm_mulm128(uint32_t *, uint32_t *, uint32_t *) NAME("asm_mulm128");

extern void asm_sqm64(uint32_t *, uint32_t *) NAME("asm_sqm64");
extern void asm_sqm96(uint32_t *, uint32_t *) NAME("asm_sqm96");
extern void asm_sqm128(uint32_t *, uint32_t *) NAME("asm_sqm128");

extern void asm_add64(uint32_t *, uint32_t *) NAME("asm_add64");
extern void asm_add96(uint32_t *, uint32_t *) NAME("asm_add96");
extern void asm_add128(uint32_t *, uint32_t *) NAME("asm_add128");

extern void asm_diff64(uint32_t *, uint32_t *, uint32_t *) NAME("asm_diff64");
extern void asm_diff96(uint32_t *, uint32_t *, uint32_t *) NAME("asm_diff96");
extern void asm_diff128(uint32_t *, uint32_t *, uint32_t *) NAME("asm_diff128");

extern void asm_sub64(uint32_t *, uint32_t *, uint32_t *) NAME("asm_sub64");
extern void asm_sub96(uint32_t *, uint32_t *, uint32_t *) NAME("asm_sub96");
extern void asm_sub128(uint32_t *, uint32_t *, uint32_t *) NAME("asm_sub128");

extern void asm_add64_ui(uint32_t *, uint32_t) NAME("asm_add64_ui");
extern void asm_add96_ui(uint32_t *, uint32_t) NAME("asm_add96_ui");
extern void asm_add128_ui(uint32_t *, uint32_t) NAME("asm_add128_ui");

extern void asm_zero64(uint32_t *, uint32_t *) NAME("asm_zero64");
extern void asm_zero96(uint32_t *, uint32_t *) NAME("asm_zero96");
extern void asm_zero128(uint32_t *, uint32_t *) NAME("asm_zero128");

extern void asm_copy64(uint32_t *, uint32_t *) NAME("asm_copy64");
extern void asm_copy96(uint32_t *, uint32_t *) NAME("asm_copy96");
extern void asm_copy128(uint32_t *, uint32_t *) NAME("asm_copy128");

extern void asm_sub_n64(uint32_t *, uint32_t *) NAME("asm_sub_n64");
extern void asm_sub_n96(uint32_t *, uint32_t *) NAME("asm_sub_n96");
extern void asm_sub_n128(uint32_t *, uint32_t *) NAME("asm_sub_n128");

extern void asm_half64(uint32_t *) NAME("asm_half64");
extern void asm_half96(uint32_t *) NAME("asm_half96");
extern void asm_half128(uint32_t *) NAME("asm_half128");

#else

#define ASM_SUBS_DECLARED

extern void asm_mulm64(uint32_t *, uint32_t *, uint32_t *);
extern void asm_mulm96(uint32_t *, uint32_t *, uint32_t *);
extern void asm_mulm128(uint32_t *, uint32_t *, uint32_t *);

extern void asm_sqm64(uint32_t *, uint32_t *);
extern void asm_sqm96(uint32_t *, uint32_t *);
extern void asm_sqm128(uint32_t *, uint32_t *);

extern void asm_add64(uint32_t *, uint32_t *);
extern void asm_add96(uint32_t *, uint32_t *);
extern void asm_add128(uint32_t *, uint32_t *);

extern void asm_diff64(uint32_t *, uint32_t *, uint32_t *);
extern void asm_diff96(uint32_t *, uint32_t *, uint32_t *);
extern void asm_diff128(uint32_t *, uint32_t *, uint32_t *);

extern void asm_sub64(uint32_t *, uint32_t *, uint32_t *);
extern void asm_sub96(uint32_t *, uint32_t *, uint32_t *);
extern void asm_sub128(uint32_t *, uint32_t *, uint32_t *);

extern void asm_add64_ui(uint32_t *, uint32_t);
extern void asm_add96_ui(uint32_t *, uint32_t);
extern void asm_add128_ui(uint32_t *, uint32_t);

extern void asm_zero64(uint32_t *, uint32_t *);
extern void asm_zero96(uint32_t *, uint32_t *);
extern void asm_zero128(uint32_t *, uint32_t *);

extern void asm_copy64(uint32_t *, uint32_t *);
extern void asm_copy96(uint32_t *, uint32_t *);
extern void asm_copy128(uint32_t *, uint32_t *);

extern void asm_sub_n64(uint32_t *, uint32_t *);
extern void asm_sub_n96(uint32_t *, uint32_t *);
extern void asm_sub_n128(uint32_t *, uint32_t *);

extern void asm_half64(uint32_t *);
extern void asm_half96(uint32_t *);
extern void asm_half128(uint32_t *);

#endif

#endif
