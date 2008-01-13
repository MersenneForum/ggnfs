/*----------------------------------------------------------------------
Copyright 2007, Jason Papadopoulos

This file is part of GGNFS.

GGNFS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GGNFS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GGNFS; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
----------------------------------------------------------------------*/

#ifndef _MP_H_
#define _MP_H_

#include <util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Basic multiple-precision arithmetic implementation. Precision
   is hardwired not to exceed ~255 digits. Numbers are stored in 
   two's-complement binary form, in little-endian word order.
   All inputs and results are assumed positive, and the high-order 
   words that are not in use must be zero for all input operands.

   The array of bits for the number is always composed of 32-bit
   words. This is because I want things to be portable and there's 
   no support in C for 128-bit data types, so that 64x64 multiplies
   and 128/64 divides would need assembly language support */

#define MAX_MP_WORDS 27

#define MP_RADIX 4294967296.0

typedef struct {
	uint32 nwords;		/* number of nonzero words in val[] */
	uint32 val[MAX_MP_WORDS];
} mp_t;


/* signed multiple-precision integers */

#ifndef POSITIVE
#define POSITIVE 0
#endif

#ifndef NEGATIVE
#define NEGATIVE 1
#endif

typedef struct {
	uint32 sign;	/* POSITIVE or NEGATIVE */
	mp_t num;
} signed_mp_t;


	/* initialize an mp_t / signed_mp_t */

static INLINE void mp_clear(mp_t *a) {
	memset(a, 0, sizeof(mp_t));
}

static INLINE void signed_mp_clear(signed_mp_t *a) {
	mp_clear(&a->num);
	a->sign = POSITIVE;
}

static INLINE void mp_copy(mp_t *a, mp_t *b) {
	*b = *a;
}

static INLINE void signed_mp_copy(signed_mp_t *a, signed_mp_t *b) {
	*b = *a;
}

	/* return the number of bits needed to hold an mp_t.
   	   This is equivalent to floor(log2(a)) + 1. */

uint32 mp_bits(mp_t *a);

	/* approximate the logarithm of an mp_t */

double mp_log(mp_t *x);

	/* Addition and subtraction; a + b = sum
	   or a - b = diff. 'b' may be an integer or 
	   another mp_t. sum or diff may overwrite 
	   the input operands */

void mp_add(mp_t *a, mp_t *b, mp_t *sum);
void mp_add_1(mp_t *a, uint32 b, mp_t *sum);
void mp_sub(mp_t *a, mp_t *b, mp_t *diff);
void mp_sub_1(mp_t *a, uint32 b, mp_t *diff);

void signed_mp_add(signed_mp_t *a, signed_mp_t *b, signed_mp_t *sum);
void signed_mp_sub(signed_mp_t *a, signed_mp_t *b, signed_mp_t *diff);

	/* return -1, 0, or 1 if a is less than, equal to,
	   or greater than b, respectively */

static INLINE int32 mp_cmp(const mp_t *a, const mp_t *b) {

	int32 i;

	if (a->nwords > b->nwords)
		return 1;
	if (a->nwords < b->nwords)
		return -1;

	for (i = a->nwords - 1; i >= 0; i--) {
		if (a->val[i] > b->val[i])
			return 1;
		if (a->val[i] < b->val[i])
			return -1;
	}

	return 0;
}


	/* quick test for zero or one mp_t */

#define mp_is_zero(a) ((a)->nwords == 0)
#define mp_is_one(a) ((a)->nwords == 1 && (a)->val[0] == 1)

	/* Shift 'a' right by 'shift' bit positions.
	   The result may overwrite 'a'. shift amount
	   must not exceed 32*MAX_MP_WORDS */

void mp_rshift(mp_t *a, uint32 shift, mp_t *res);

	/* Right-shift 'a' by an amount equal to the
	   number of trailing zeroes. Return the shift count */

uint32 mp_rjustify(mp_t *a, mp_t *res);

	/* multiply a by b. 'b' is either a 1-word
	   operand or an mp_t. In the latter case, 
	   the product must fit in MAX_MP_WORDS words
	   and may not overwrite the input operands. */

void mp_mul(mp_t *a, mp_t *b, mp_t *prod);
void mp_mul_1(mp_t *a, uint32 b, mp_t *x);
void signed_mp_mul(signed_mp_t *a, signed_mp_t *b, signed_mp_t *prod);

	/* divide a 64-bit input by a 32-bit input,
	   and return the remainder. The quotient must
	   not exceed 2^32 */

static INLINE uint32 mp_mod64(uint64 a, uint32 n) {

	uint32 ans;

#if (defined(__GNUC__) || defined(__ICL)) && \
	(defined(__i386__) || defined(__x86_64__))
	asm("divl %3  \n\t"
	     : "=d"(ans)
	     : "a"((uint32)(a)), "0"((uint32)(a >> 32)), "g"(n) : "cc");

#elif defined(_MSC_VER) && !defined(_WIN64)
	__asm
	{
		lea	ecx,a
		mov	eax,[ecx]
		mov	edx,[ecx+4]
		div	n
		mov	ans,edx
	}

#else
	ans = (uint32)(a % n);
#endif
	return ans;
}

	/* modular multiplication: compute 'a' * 'b' mod 'n'.
	   Multiple precision operands can have up to 
	   MAX_MP_WORDS words each; the out 'res' can alias
	   a or b but not n */

void mp_modmul(mp_t *a, mp_t *b, mp_t *n, mp_t *res);

static INLINE uint32 mp_modmul_1(uint32 a, uint32 b, uint32 n) {
	uint64 acc = (uint64)a * (uint64)b;
	return mp_mod64(acc, n);
}

	/* General-purpose division routines. mp_divrem
	   divides num by denom, putting the quotient in
	   quot (if not NULL) and the remainder in rem
	   (if not NULL). No aliasing is allowed */

void mp_divrem(mp_t *num, mp_t *denom, mp_t *quot, mp_t *rem);
#define mp_div(n, d, q) mp_divrem(n, d, q, NULL)
#define mp_mod(n, d, rem) mp_divrem(n, d, NULL, rem)

	/* Division routine where the denominator is a
	   single word. The quotient is written to quot
	   (if not NULL) and the remainder is returned.
	   quot may overwrite the input */

uint32 mp_divrem_1(mp_t *num, uint32 denom, mp_t *quot);


	/* Divide an mp_t by a single word and return the
	   remainder */

static INLINE uint32 mp_mod_1(mp_t *num, uint32 denom) {
	int32 i = num->nwords - 1;
	uint32 rem = 0;

	if (num->val[i] < denom) {
		rem = num->val[i--];
	}

#if (defined(__GNUC__) || defined(__ICL)) && \
	(defined(__i386__) || defined(__x86_64__))
	while (i >= 0) {
		asm("divl %3"
			: "=d"(rem)
			: "0"(rem), "a"(num->val[i]), "r"(denom) : "cc" );
		i--;
	}

#elif defined(_MSC_VER) && !defined(_WIN64)
	__asm
	{
		push	ebx
		push	esi
		mov	ecx,i
		or	ecx,ecx
		jl	L1
		mov	ebx,denom
		mov	esi,num
		lea	esi,[esi]num.val
		mov	edx,rem
	L0:	mov	eax,[esi+4*ecx]
		div	ebx
		dec	ecx
		jge	L0
		mov	rem,edx
	L1:	pop	esi
		pop	ebx
	}
#else

	while (i >= 0) {
		uint64 acc = (uint64)rem << 32 | (uint64)num->val[i];
		rem = (uint32)(acc % denom);
		i--;
	}
#endif
	return rem;
}

	/* Calculate floor(i_th root of 'a'). The return value 
	   is zero if res is an exact i_th root of 'a' */

uint32 mp_iroot(mp_t *a, uint32 i, mp_t *res);
#define mp_isqrt(a, res) mp_iroot(a, 2, res)

	/* Calculate greatest common divisor of x and y.
	   Any quantities may alias */

void mp_gcd(mp_t *x, mp_t *y, mp_t *out);

static INLINE uint32 mp_gcd_1(uint32 x, uint32 y) {

	uint32 tmp;

	if (y < x) {
		tmp = x; x = y; y = tmp;
	}

	while (y > 0) {
		x = x % y;
		tmp = x; x = y; y = tmp;
	}
	return x;
}

	/* Print routines: print the input mp_t to a file
	   (if f is not NULL) and also return a pointer to
	   a string representation of the input (requires
	   sufficient scratch space to be passed in). The 
	   input is printed in radix 'base' (2 to 36). The
	   maximum required size for scratch space is
	   32*MAX_MP_WORDS+1 bytes (i.e. enough to print
	   out 'a' in base 2) */

char * mp_print(mp_t *a, uint32 base, FILE *f, char *scratch);
#define mp_printf(a, base, scratch) mp_print(a, base, stdout, scratch)
#define mp_fprintf(a, base, f, scratch) mp_print(a, base, f, scratch)
#define mp_sprintf(a, base, scratch) mp_print(a, base, NULL, scratch)

	/* A multiple-precision version of strtoul(). The
	   string 'str' is converted from an ascii representation
	   of radix 'base' (2 to 36) into an mp_t. If base is 0,
	   the base is assumed to be 16 if the number is preceded
	   by "0x", 8 if preceded by "0", and 10 otherwise. Con-
	   version stops at the first character that cannot belong
	   to radix 'base', or otherwise at the terminating NULL.
	   The input is case insensitive. */

void mp_str2mp(char *str, mp_t *a, uint32 base);

	/* modular exponentiation: raise 'a' to the power 'b' 
	   mod 'n' and return the result. a and b may exceed n.
	   In the multiple precision case, the result may not
	   alias any of the inputs */

void mp_expo(mp_t *a, mp_t *b, mp_t *n, mp_t *res);

	/* ordinary exponentiation: raise 'a' to the power 'b' 
	   and return the result. The result may not alias 
	   any of the inputs, and must fit in an mp_t */

void mp_pow(mp_t *a, mp_t *b, mp_t *res);

static INLINE uint32 mp_expo_1(uint32 a, uint32 b, uint32 n) {

	uint32 res = 1;
	while (b) {
		if (b & 1)
			res = mp_modmul_1(res, a, n);
		a = mp_modmul_1(a, a, n);
		b = b >> 1;
	}
	return res;
}

	/* Return the Legendre symbol for 'a'. This is 1 if
	   x * x = a (mod p) is solvable for some x, -1 if not, 
	   and 0 if a and p have factors in common. p must be 
	   an odd prime, and a may exceed p */

int32 mp_legendre_1(uint32 a, uint32 p);
int32 mp_legendre(mp_t *a, mp_t *p);

	/* Find an inverse of 'a' modulo prime 'p', i.e. the number
	   x such that a * x mod p is 1. These routines assume that 
	   a will never exceed p. The multiple-precision version returns
	   zero if the inverse exists, and uses the extended Euclidean
	   algorithm so that p may be composite */

uint32 mp_modinv_1(uint32 a, uint32 p);
uint32 mp_modinv(mp_t *a, mp_t *p, mp_t *res);

	/* For odd prime p, solve 'x * x = a (mod p)' for x and
	   return the result. Assumes legendre(a,p) = 1 (this is
	   not verified).
	   
	   This and the next few routines use random numbers, but
	   since they are intended to be 'stateless' the state of the
	   random number generator is passed in as 'seed1' and 'seed2'.
	   This state is updated as random numbers are produced */

uint32 mp_modsqrt_1(uint32 a, uint32 p);
void mp_modsqrt(mp_t *a, mp_t *p, mp_t *res, uint32 *seed1, uint32 *seed2);
void mp_modsqrt2(mp_t *a, mp_t *p, mp_t *res, uint32 *seed1, uint32 *seed2);

	/* Generate a random number between 0 and 2^bits - 1 */

void mp_rand(uint32 bits, mp_t *res, uint32 *seed1, uint32 *seed2);

	/* mp_is_prime returns 1 if the input is prime and 0 
	   otherwise. mp_random_prime generates a random number 
	   between 0 and 2^bits - 1 which is probably prime. 
	   mp_next_prime computes the next number greater than p
	   which is prime, and returns (res - p). The probability 
	   of these routines accidentally declaring a composite 
	   to be prime is < 4 ^ -NUM_WITNESSES, and probably is
	   drastically smaller than that */

#define NUM_WITNESSES 20
int32 mp_is_prime(mp_t *p, uint32 *seed1, uint32 *seed2);
void mp_random_prime(uint32 bits, mp_t *res, uint32 *seed1, uint32 *seed2);
uint32 mp_next_prime(mp_t *p, mp_t *res, uint32 *seed1, uint32 *seed2);


	/* Modular addition/subtraction: compute a +- b mod p */

static INLINE uint32 mp_modsub_1(uint32 a, uint32 b, uint32 p) {

#if (defined(__GNUC__) || defined(__ICL)) && \
		defined(__i386__) && defined(HAVE_CMOV)
	uint32 ans;
	asm("xorl %%edx, %%edx	\n\t"
	    "subl %2, %0	\n\t"
	    "cmovbl %3, %%edx	\n\t"
	    "addl %%edx, %0	\n\t"
	 : "=r"(ans)
	 : "0"(a), "g"(b), "g"(p) : "%edx", "cc");

	return ans;

#elif defined(_MSC_VER) && !defined(_WIN64) && defined(HAVE_CMOV)
	uint32 ans;
	__asm
	{
		mov	eax,a
		mov	ecx,b
		xor	edx,edx
		sub	eax,ecx
		cmovb	edx,p
		add	eax,edx
		mov	ans,eax
	}
	return ans;

#else
	if (a >= b)
		return a - b;
	else
		return a - b + p;
#endif
}

static INLINE uint32 mp_modadd_1(uint32 a, uint32 b, uint32 p) {

	return mp_modsub_1(a, p - b, p);
}

	/* conversion to/from doubles. Note that the maximum
	   exponent in a double cannot accurately represent
	   an mp_t that is sufficiently large. In order to get
	   any precision in the mantissa at all, the input
	   should have under 100 digits */

static INLINE double mp_mp2d(mp_t *x) {

	/* convert a multiple-precision number to a double */

	uint32 i = x->nwords;

	switch(i) {
	case 0:
		return 0;
	case 1:
		return (double)(x->val[0]);
	case 2:
		return (double)(x->val[0]) + 
			MP_RADIX * x->val[1];
	case 3:
		return (double)(x->val[0]) + MP_RADIX * 
		       ((double)x->val[1] + MP_RADIX * x->val[2]);
	default:
		return ((double)(x->val[i-3]) + MP_RADIX * 
		       ((double)x->val[i-2] + MP_RADIX * x->val[i-1])) *
		       pow(2.0, 32.0 * (i - 3));
	}
}

static INLINE double mp_signed_mp2d(signed_mp_t *x) {

	if (x->sign == POSITIVE)
		return mp_mp2d(&x->num);
	else
		return -mp_mp2d(&x->num);
}

static INLINE void mp_d2mp(double *d, mp_t *x) {

	int32 i;
	int32 exponent;
	uint64 int_mant;

	/* convert a double to a multiple-precision integer.
	   It's assumed the double represents an integer
	   
	   Note that a pointer to d, and not d itself, is
	   *required* for PowerPC builds (at least) to work */

	mp_clear(x);

	/* cut up the double into mantissa and exponent.
	   Reading it in as a uint64 makes this process
	   endian-independent */

	int_mant = *(uint64 *)(d);
	exponent = ((int32)(int_mant >> 52) & 0x7ff) - 1023;
	int_mant &= ~((uint64)(0xfff) << 52);
	int_mant |= (uint64)(1) << 52;

	/* insert the bits of the mantissa into the multi-precision
	   array. First shift away any fractional bits, then place
	   in the array of zero bits */

	if (exponent < 0) {
		return;
	}
	else if (exponent <= 52) {
		int_mant = int_mant >> (52 - exponent);
		exponent = 0;
	}
	else {
		exponent -= 52;
	}
	if (int_mant == 0)
		return;

	i = exponent / 32;
	if (exponent % 32 == 0) {
		x->val[i] = (uint32)int_mant;
		x->val[i+1] = (uint32)(int_mant >> 32);
	}
	else {
		x->val[i] = (uint32)(int_mant << (exponent % 32));
		x->val[i+1] = (uint32)(int_mant >> (32 - (exponent % 32)));
		x->val[i+2] = (uint32)(int_mant >> (64 - (exponent % 32)));
	}

	if (x->val[i+2] != 0)
		x->nwords = i+3;
	else if (x->val[i+1] != 0)
		x->nwords = i+2;
	else
		x->nwords = i+1;
}

#ifdef __cplusplus
}
#endif

#endif /* !_MP_H_ */
