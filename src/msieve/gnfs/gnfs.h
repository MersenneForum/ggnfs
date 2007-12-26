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

#ifndef _GNFS_H_
#define _GNFS_H_

/* An implementation of the General Number Field
   Sieve algorithm for integer factorization. */

/* include basic stuff */

#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------- general stuff ---------------------------*/

/* the crossover point from degree 5 polynomials to
   degree 6 is so huge (theoretically somewhere over
   200 digits) that there's no point in allowing for
   the degree to exceed 5. However, we allow degree 6
   for anyone who wants to undertake SNFS */

#define MAX_POLY_DEGREE 6

/* representation of polynomials with multiple-
   precision coefficients. For polynomial p(x),
   element i of coeff[] gives the coefficient
   of x^i */

typedef struct {
	uint32 degree;
	signed_mp_t coeff[MAX_POLY_DEGREE + 1];
} mp_poly_t;

/* buffered output to the NFS savefile */

void nfs_print_to_savefile(msieve_obj *obj, char *buf);
void nfs_flush_savefile(msieve_obj *obj);

/* evaluate the homogeneous form of poly(x). If poly has
   degree d, then res = (b ^ d) * poly(a / b) */

void eval_poly(signed_mp_t *res, int64 a, uint32 b, mp_poly_t *poly);


typedef struct {
	int64 a;
	uint32 b;
} abpair_t;

/* Configuration for NFS parameters */

typedef struct {
	uint32 bits;       /* size of integer this config info applies to */
	uint32 rfb_limit;   /* largest rational factor base prime */
	uint32 afb_limit;   /* largest algebraic factor base prime */
	uint32 rfb_lp_size;   /* size of rational large primes */
	uint32 afb_lp_size;   /* size of algebraic large primes */
	uint64 sieve_size; /* the size of the sieve (actual sieve is 2x this) */
} sieve_param_t;

/*------------------------ hashtable stuff ---------------------------*/

/* we use two separate hash functions for 2-word structures */

#define HASH1(word) ((uint32)(word) * (uint32)(2654435761UL))
#define HASH2(word) ((uint32)(word) * ((uint32)40499 * 65543))

/* hashtables are used in several different ways, but
   internally the hashtable indexes hash_t structures */

typedef struct {
	uint32 payload[2];
	uint32 next;
} hash_t;

/* structure used by hashtables */

typedef struct {
	uint32 *hashtable;
	hash_t *match_array;
	uint32 log2_hashtable_size;
	uint32 match_array_size;
	uint32 match_array_alloc;
	uint32 blob_words;
} hashtable_t;

void hashtable_init(hashtable_t *h, 
		    uint32 log2_hashtable_size,
		    uint32 init_match_size,
		    uint32 blob_words);

void hashtable_close(hashtable_t *h);
void hashtable_free(hashtable_t *h);

/* return a pointer to the hashtable entry that matches
   blob[]. If there is no such entry, add blob to 
   the hashtable, make *present zero (if the pointer
   if non-NULL) and return a pointer to the result */

hash_t *hashtable_find(hashtable_t *h, void *blob, uint32 *present);

uint32 hashtable_get_offset(hashtable_t *h, hash_t *entry);

uint32 hashtable_getall(hashtable_t *h, hash_t **all_entries);

/*---------------------- finite field poly stuff ---------------------*/

/* reduce the coefficients of _f modulo p, then compute
   all the x for which _f(x) = 0 mod p. The number of
   roots found, and the leading coefficient of _f mod p,
   is returned. If count_only is zero, the roots are 
   also returned in zeros[] */

uint32 poly_get_zeros(uint32 *zeros, 
			mp_poly_t *_f, 
			uint32 p,
			uint32 *high_coeff,
			uint32 count_only);

/* return 1 if poly cannot be expressed as the product 
   of some other polynomials with coefficients modulo p,
   zero otherwise */

uint32 is_irreducible(mp_poly_t *poly, uint32 p);

/* compute the inverse square root of the polynomial s_in,
   modulo the monic polynomial f_in, with all coefficients
   reduced modulo q. Returns 1 if the root is found and 
   zero otherwise */

uint32 inv_sqrt_mod_q(mp_poly_t *res, mp_poly_t *s_in, 
			mp_poly_t *f_in, uint32 q, 
			uint32 *rand_seed1, uint32 *rand_seed2);

/*---------------------- factor base stuff ---------------------------*/

/* general entry in the factor base */

typedef struct {
	uint32 p;   /* prime for the entry */
	uint32 r;   /* the root of polynomial mod p for this
			entry, or p for projective roots */
} fb_entry_t;

/* rational and algebraic factor bases are treated
   the same as often as possible */

typedef struct {
	mp_poly_t poly;         /* rational or algebraic polynomial */
	uint32 max_prime;       /* largest prime in the factor base */
	uint32 num_entries;     /* number of factor base entries */
	uint32 num_alloc;       /* amount allocated for FB entries */
	fb_entry_t *entries;    /* list of factor base entries */
} fb_side_t;

/* the NFS factor base */

typedef struct {
	fb_side_t rfb;    /* rational factor base */
	fb_side_t afb;    /* algebraic factor base */
} factor_base_t;

/* Given a factor base fb with the polynomials and the 
   maximum size primes filled in, fill in the rest of
   the entries in fb */

void create_factor_base(msieve_obj *obj, 
			factor_base_t *fb, 
			uint32 report_progress);

/* read / write / free a factor base */

int32 read_factor_base(msieve_obj *obj, mp_t *n,
		     sieve_param_t *params, factor_base_t *fb);

void write_factor_base(msieve_obj *obj, mp_t *n,
			sieve_param_t *params, factor_base_t *fb);

void free_factor_base(factor_base_t *fb);

/*---------------------- poly selection stuff ---------------------------*/

/* attempt to read NFS polynomials from the factor 
   base file, save them and return 0 if successful */

int32 read_poly(msieve_obj *obj, mp_t *n,
	       mp_poly_t *rat_poly,
	       mp_poly_t *alg_poly);

/* unconditionally write the input NFS polynomials
   to a new factor base file */

void write_poly(msieve_obj *obj, mp_t *n,
	       mp_poly_t *rat_poly,
	       mp_poly_t *alg_poly);

/*---------------------- sieving stuff ----------------------------------*/

/* external interface to perform sieving. The number of
   relations in the savefile at the time sieving completed
   is returned */

uint32 do_line_sieving(msieve_obj *obj, 
			sieve_param_t *params,
			mp_t *n, uint32 start_relations,
			uint32 max_relations);

/* add free relations to the savefile for this factorization;
   returns the number of relations added */

uint32 add_free_relations(msieve_obj *obj, sieve_param_t *params, mp_t *n);

/*---------------------- filtering stuff --------------------------------*/

/* external interface to filter relations. The return value is zero
   if filtering succeeded and the linear algebra can run, otherwise
   the estimated number of relations still needed before filtering
   could succeed is returned */

uint32 nfs_filter_relations(msieve_obj *obj, mp_t *n);

/*---------------------- linear algebra stuff ----------------------------*/

/* the minimum number of excess columns in the final
   matrix generated from relations. Note that the value 
   chosen contains a healthy fudge factor */

#define NUM_EXTRA_RELATIONS 200

/* external interface for NFS linear algebra */

void nfs_solve_linear_system(msieve_obj *obj, mp_t *n);

/* The largest prime ideal that is stored in compressed format
   when the matrix is built. Setting this to zero will cause
   all matrix rows to be stored in uncompressed format */

#define MAX_PACKED_PRIME 97

/*------------------------ square root stuff --------------------------*/

uint32 nfs_find_factors(msieve_obj *obj, mp_t *n, 
			factor_list_t *factor_list);

/*------------------- relation processing stuff --------------------------*/

#define RATIONAL_IDEAL 0
#define ALGEBRAIC_IDEAL 1

/* canonical representation of an ideal. NFS filtering
   and linear algebra will only work if the different
   ideals that occur in a factorization map to unique
   values of this structure. 
   
   Every ideal has a prime p and root r. To save space
   but still allow 32-bit p we store (p-1)/2 (thanks to
   Alex Kruppa for this trick) */

typedef union {
	struct {
		uint32 compressed_p : 31;  /* (p - 1) / 2 */
		uint32 rat_or_alg : 1;     /* RATIONAL_IDEAL, ALGEBRAIC_IDEAL */
		uint32 r;                  /* root for ideal */
	} i;
	uint32 blob[2];
} ideal_t;

/* canonical representation of a relation, used in
   the NFS postprocessing phase */

typedef struct relation_t {
	int64 a;               /* coordinates of relation; free relations */
	uint32 b;              /*   have b = 0 */
	uint32 rel_index;      /* line of savefile where relation occurs */
	uint8 num_factors_r;   /* number of rational factors */
	uint8 num_factors_a;   /* number of algebraic factors */
	uint16 refcnt;         /* scratch value used in postprocessing */
	uint32 *factors;       /* list of rational+algebraic factors */
} relation_t;

/* used whenever temporary arrays are needed to store
   lists of factors of relations */

#define TEMP_FACTOR_LIST_SIZE 100

/* structure used to conveniently represent all of
   the large ideals that occur in a relation. The
   structure is far too large to be useful when 
   representing large groups of relations, so in
   these applications the data should be transferred
   to other containers once it is filled in here.
   Note that all of the rational ideals are listed
   first, then all of the algebraic ideals */

typedef struct {
	uint32 rel_index;         /* line of savefile where relation occurs */
	uint8 ideal_count;        /* count of large ideals */
	uint8 gf2_factors;        /* count of ideals not listed in ideal_list */
	ideal_t ideal_list[TEMP_FACTOR_LIST_SIZE];
} relation_lp_t;

/* convert a line of text into a relation_t, return 0
   if conversion succeeds. The arrays pointed to within
   'r' should have at least TEMP_FACTOR_LIST_SIZE entries.
   If 'compress' is nonzero then store only one instance 
   of any factors, and only if the factor occurs in r 
   an odd number of times */

int32 nfs_read_relation(char *buf, factor_base_t *fb, 
			relation_t *r, uint32 compress);

/* given a relation, find and list all of the rational
   and algebraic ideals whose prime exceeds filtmin. If
   this bound is zero then all ideals are listed */

uint32 find_large_ideals(relation_t *rel, relation_lp_t *out, uint32 filtmin);

/* Assuming a group of relations has been grouped together
   into a collection of cycles, read the collection of cycles
   from disk, read the relations they need, and link the two
   collections together. If 'compress' is nonzero then relations
   get only one instance of any factors, and only if the factor 
   occurs an odd number of times in the relation. If dependency
   is nonzero, only the cycles and relations required by that
   one dependency are read in. If fb is NULL, only the cycles
   (and not the relations they need) are read in */
   
void nfs_read_cycles(msieve_obj *obj, factor_base_t *fb, uint32 *ncols, 
			la_col_t **cols, uint32 *num_relations,
			relation_t **relation_list, uint32 compress,
			uint32 dependency);

void nfs_free_relation_list(relation_t *rlist, uint32 num_relations);

#ifdef __cplusplus
}
#endif

#endif /* _GNFS_H_ */
