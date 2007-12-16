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

$Id: relation.c,v 1.1 2007-12-16 03:54:46 jasonp_sf Exp $
----------------------------------------------------------------------*/

#include <common.h>
#include "gnfs.h"

/*--------------------------------------------------------------------*/
static uint32 divide_factor_out(mp_t *polyval, uint32 p, 
				uint32 *factors, uint32 *num_factors,
				uint32 compress) {

	/* read the rational factors. Note that the following
	   will work whether a given factor appears only once
	   or whether its full multiplicity is in the relation */

	uint32 i = *num_factors;
	uint32 multiplicity = 0;

	while (mp_mod_1(polyval, p) == 0) {
		mp_divrem_1(polyval, p, polyval);
		multiplicity++;
	}
	if (i + multiplicity >= TEMP_FACTOR_LIST_SIZE)
		return 1;

	if (compress) {
		if (multiplicity & 1)
			factors[i++] = p;
	}
	else if (multiplicity) {
		while (multiplicity--)
			factors[i++] = p;
	}
	*num_factors = i;
	return 0;
}

/*--------------------------------------------------------------------*/
int32 nfs_read_relation(char *buf, factor_base_t *fb, 
			relation_t *r, uint32 compress) {

	/* note that only the polynomials within the factor
	   base need to be initialized */

	uint32 i, p;
	int64 a, atmp;
	uint32 b;
	char *tmp, *next_field;
	signed_mp_t polyval;
	uint32 num_factors_r;
	uint32 num_factors_a;
	uint32 *factors = r->factors;

	/* read the relation coordinates */

	a = (int64)strtod(buf, &next_field);
	tmp = next_field;
	if (tmp[0] != ',' || !isdigit(tmp[1]))
		return -1;

	b = strtoul(tmp+1, &next_field, 10);
	tmp = next_field;

	num_factors_r = 0;
	num_factors_a = 0;
	r->a = a;
	r->b = b;

	/* for free relations, store the roots and not
	   the prime factors */

	if (b == 0) {
		uint32 i;
		uint32 roots[MAX_POLY_DEGREE];
		uint32 high_coeff, num_roots;

		if (a == 0)
			return -2;

		num_roots = poly_get_zeros(roots, &fb->rfb.poly,
					(uint32)a, &high_coeff, 0);
		if (num_roots != fb->rfb.poly.degree || high_coeff == 0)
			return -3;
		factors[num_factors_r++] = a;

		num_roots = poly_get_zeros(roots, &fb->afb.poly,
					(uint32)a, &high_coeff, 0);
		if (num_roots != fb->afb.poly.degree || high_coeff == 0)
			return -4;
		for (i = 0; i < num_roots; i++)
			factors[num_factors_r + i] = roots[i];

		r->num_factors_r = num_factors_r;
		r->num_factors_a = num_roots;
		return 0;
	}

	if (tmp[0] != ':' || !isxdigit(tmp[1]))
		return -5;
	
	atmp = a % (int64)b;
	if (atmp < 0)
		atmp += b;

	if (mp_gcd_1((uint32)atmp, b) != 1)
		return -6;

	/* handle a rational factor of -1 */

	eval_poly(&polyval, a, b, &fb->rfb.poly);
	if (mp_is_zero(&polyval.num))
		return -6;
	if (polyval.sign == NEGATIVE)
		factors[num_factors_r++] = 0;

	/* read the rational factors */

	do {
		uint32 p = strtoul(tmp + 1, &next_field, 16);
		if (p > 1 && divide_factor_out(&polyval.num, p, factors,
					&num_factors_r, compress)) {
			return -8;
		}
		tmp = next_field;
	} while(tmp[0] == ',' && isxdigit(tmp[1]));

	if (tmp[0] != ':')
		return -9;

	/* if there are rational factors still to be accounted
	   for, assume they are small and find them by brute force */

	for (i = p = 0; !mp_is_one(&polyval.num) &&
				p < 1000; i++) {

		p += prime_delta[i];
		if (divide_factor_out(&polyval.num, p, factors,
					&num_factors_r, compress)) {
			return -10;
		}
	}

	if (!mp_is_one(&polyval.num))
		return -11;

	/* read the algebraic factors */

	eval_poly(&polyval, a, b, &fb->afb.poly);
	if (mp_is_zero(&polyval.num))
		return -12;

	do {
		uint32 p = strtoul(tmp + 1, &next_field, 16);
		if (p > 1 && divide_factor_out(&polyval.num, p, 
					factors + num_factors_r, 
					&num_factors_a, compress)) {
			return -13;
		}
		tmp = next_field;
	} while(tmp[0] == ',' && isxdigit(tmp[1]));

	/* if there are algebraic factors still to be accounted
	   for, assume they are small and find them by brute force */

	for (i = p = 0; !mp_is_one(&polyval.num) &&
				p < 1000; i++) {

		p += prime_delta[i];
		if (divide_factor_out(&polyval.num, p,
					factors + num_factors_r, 
					&num_factors_a, compress)) {
			return -14;
		}
	}

	if (!mp_is_one(&polyval.num))
		return -15;
	
	r->num_factors_r = num_factors_r;
	r->num_factors_a = num_factors_a;
	return 0;
}

/*--------------------------------------------------------------------*/
uint32 find_large_ideals(relation_t *rel, 
			relation_lp_t *out, uint32 filtmin) {
	uint32 i;
	uint32 num_ideals = 0;
	uint32 num_factors_r;
	int64 a = rel->a;
	uint32 b = rel->b;

	out->gf2_factors = 0;

	/* handle free relations */

	if (b == 0) {
		uint32 p = rel->factors[0];

		if (p > filtmin) {
			ideal_t *ideal = out->ideal_list + num_ideals;
			ideal->i.r = p;
			ideal->i.compressed_p = (p - 1) / 2;
			ideal->i.rat_or_alg = RATIONAL_IDEAL;
			num_ideals++;
		}
		else if (p > MAX_PACKED_PRIME) {
			out->gf2_factors++;
		}

		if (p > filtmin) {
			for (i = 0; i < rel->num_factors_a; i++) {
				ideal_t *ideal = out->ideal_list + 
							num_ideals + i;
				ideal->i.r = rel->factors[i + 1];
				ideal->i.compressed_p = (p - 1) / 2;
				ideal->i.rat_or_alg = ALGEBRAIC_IDEAL;
			}
			num_ideals += rel->num_factors_a;
		}
		else if (p > MAX_PACKED_PRIME) {
			out->gf2_factors += rel->num_factors_a;
		}

		out->ideal_count = num_ideals;
		return num_ideals;
	}

	/* find the large rational ideals */

	num_factors_r = rel->num_factors_r;

	for (i = 0; i < num_factors_r; i++) {
		uint32 p = rel->factors[i];

		/* if processing all the ideals, make up a
		   separate unique entry for rational factors of -1 */

		if (p == 0 && filtmin == 0) {
			ideal_t *ideal = out->ideal_list + num_ideals;
			ideal->i.compressed_p = 0x7fffffff;
			ideal->i.rat_or_alg = RATIONAL_IDEAL;
			ideal->i.r = (uint32)(-1);
			num_ideals++;
			continue;
		}

		if (p > filtmin) {

			/* make a single unique entry for p, instead
			   of finding the exact number r for which
			   rational_poly(r) mod p is zero */

			ideal_t *ideal = out->ideal_list + num_ideals;

			if (num_ideals >= TEMP_FACTOR_LIST_SIZE)
				return TEMP_FACTOR_LIST_SIZE + 1;

			ideal->i.compressed_p = (p - 1) / 2;
			ideal->i.rat_or_alg = RATIONAL_IDEAL;
			ideal->i.r = p;
			num_ideals++;
		}
		else if (p > MAX_PACKED_PRIME) {

			/* we only keep a count of the ideals that are
			   too small to list explicitly. NFS filtering
			   will work a little better if we completely
			   ignore the smallest ideals */

			out->gf2_factors++;
		}
	}

	/* repeat for the large algebraic ideals */

	for (i = 0; i < (uint32)rel->num_factors_a; i++) {
		uint32 p = rel->factors[num_factors_r + i];
		if (p > filtmin) {
			ideal_t *ideal = out->ideal_list + num_ideals;
			uint32 bmodp;

			if (num_ideals >= TEMP_FACTOR_LIST_SIZE)
				return TEMP_FACTOR_LIST_SIZE + 1;

			/* this time we have to find the exact r */

			bmodp = b % p;
			if (bmodp == 0) {
				ideal->i.r = p;
			}
			else {
				uint32 root;
				int64 mapped_a = a % (int64)p;
				if (mapped_a < 0)
					mapped_a += p;
				root = (uint32)mapped_a;
				root = mp_modmul_1(root, 
						mp_modinv_1(bmodp, p), p);
				ideal->i.r = root;
			}
			ideal->i.compressed_p = (p - 1) / 2;
			ideal->i.rat_or_alg = ALGEBRAIC_IDEAL;
			num_ideals++;
		}
		else if (p > MAX_PACKED_PRIME) {
			out->gf2_factors++;
		}
	}

	out->ideal_count = num_ideals;
	return num_ideals;
}

/*--------------------------------------------------------------------*/
#define LOG2_RELIDX_HASHTABLE_SIZE 20

static int compare_relidx(const void *x, const void *y) {
	hash_t *xx = (hash_t *)x;
	hash_t *yy = (hash_t *)y;
	if (xx->payload[0] > yy->payload[0])
		return 1;
	if (xx->payload[0] < yy->payload[0])
		return -1;
	return 0;
}

static int bsearch_relation(const void *key, const void *rel) {
	relation_t *r = (relation_t *)rel;
	uint32 *k = (uint32 *)key;

	if ((*k) < r->rel_index)
		return -1;
	if ((*k) > r->rel_index)
		return 1;
	return 0;
}

static void nfs_get_cycle_relations(msieve_obj *obj, 
				factor_base_t *fb, uint32 num_cycles, 
				la_col_t *cycle_list, 
				uint32 *num_relations_out,
				relation_t **rlist_out,
				uint32 compress) {
	uint32 i, j;
	char buf[256];
	FILE *relation_fp;
	relation_t *rlist;

	hashtable_t unique_relidx;
	uint32 num_unique_relidx;
	hash_t *relidx_list;

	uint32 tmp_factors[TEMP_FACTOR_LIST_SIZE];
	relation_t tmp_relation;

	tmp_relation.factors = tmp_factors;

	hashtable_init(&unique_relidx, LOG2_RELIDX_HASHTABLE_SIZE,
			10000, (uint32)1);

	/* find the list of unique relations that
	   cycle_list will need */

	for (i = 0; i < num_cycles; i++) {
		la_col_t *c = cycle_list + i;

		for (j = 0; j < c->cycle.num_relations; j++) {
			hashtable_find(&unique_relidx, 
					c->cycle.list + j, NULL);
		}
	}

	/* sort the list in order of increasing relation number */

	hashtable_close(&unique_relidx);
	num_unique_relidx = hashtable_getall(&unique_relidx, &relidx_list);
	qsort(relidx_list, (size_t)num_unique_relidx, 
		sizeof(hash_t), compare_relidx);

	logprintf(obj, "cycles contain %u unique relations\n", 
				num_unique_relidx);

	relation_fp = fopen(obj->savefile_name, "r");
	if (relation_fp == NULL) {
		logprintf(obj, "error: read_cycles can't open rel. file\n");
		exit(-1);
	}

	/* read the list of relations */

	rlist = (relation_t *)xmalloc(num_unique_relidx * sizeof(relation_t));

	i = (uint32)(-1);
	j = 0;
	fgets(buf, (int)sizeof(buf), relation_fp);
	while (!feof(relation_fp) && j < num_unique_relidx) {
		
		int32 status;

		if (buf[0] != '-' && !isdigit(buf[0])) {

			/* no relation at this line */

			fgets(buf, (int)sizeof(buf), relation_fp);
			continue;
		}
		if (++i < relidx_list[j].payload[0]) {

			/* relation not needed */

			fgets(buf, (int)sizeof(buf), relation_fp);
			continue;
		}

		status = nfs_read_relation(buf, fb, &tmp_relation, compress);
		if (status) {
			/* at this point, if the relation couldn't be
			   read then the filtering stage should have
			   found that out and skipped it */

			logprintf(obj, "error: relation %u corrupt\n", i);
			exit(-1);
		}
		else {
			/* save the relation */

			uint32 num_r = tmp_relation.num_factors_r;
			uint32 num_a = tmp_relation.num_factors_a;
			relation_t *r = rlist + j++;

			*r = tmp_relation;
			r->rel_index = i;
			r->refcnt = 0;
			r->factors = (uint32 *)xmalloc((num_r + num_a) * 
							sizeof(uint32));
			memcpy(r->factors, tmp_relation.factors,
					(num_r + num_a) * sizeof(uint32));
		}

		fgets(buf, (int)sizeof(buf), relation_fp);
	}

	num_unique_relidx = *num_relations_out = j;
	logprintf(obj, "read %u relations\n", j);
	fclose(relation_fp);
	hashtable_free(&unique_relidx);

	/* walk through the list of cycles and convert
	   relation indices to relation pointers */

	for (i = 0; i < num_cycles; i++) {
		la_col_t *c = cycle_list + i;

		for (j = 0; j < c->cycle.num_relations; j++) {

			/* since relations were read in order of increasing
			   relation index (= savefile line number), use 
			   binary search to locate relation j for this
			   cycle, then save a pointer to it */

			relation_t *rptr = (relation_t *)bsearch(
						c->cycle.list + j,
						rlist,
						(size_t)num_unique_relidx,
						sizeof(relation_t),
						bsearch_relation);
			if (rptr == NULL) {
				/* this cycle is corrupt somehow */
				logprintf(obj, "error: cannot locate "
						"relation %u\n", 
						c->cycle.list[j]);
				exit(-1);
			}
			else {
				c->cycle.list[j] = rptr - rlist;
			}
		}
	}
	*rlist_out = rlist;
}

/*--------------------------------------------------------------------*/
#define MAX_CYCLE_SIZE 500

void nfs_read_cycles(msieve_obj *obj, 
			factor_base_t *fb,
			uint32 *num_cycles_out, 
			la_col_t **cycle_list_out, 
			uint32 *num_relations_out,
			relation_t **rlist_out,
			uint32 compress,
			uint32 dependency) {

	uint32 i, j;
	uint32 num_cycles;
	uint32 curr_cycle;
	uint32 num_relations;
	uint32 rel_index[MAX_CYCLE_SIZE];
	char buf[256];
	FILE *cycle_fp;
	FILE *dep_fp = NULL;
	la_col_t *cycle_list;
	relation_t *rlist;
	uint64 mask = 0;

	sprintf(buf, "%s.cyc", obj->savefile_name);
	cycle_fp = fopen(buf, "rb");
	if (cycle_fp == NULL) {
		logprintf(obj, "error: read_cycles can't open cycle file\n");
		exit(-1);
	}

	if (dependency) {
		sprintf(buf, "%s.dep", obj->savefile_name);
		dep_fp = fopen(buf, "rb");
		if (dep_fp == NULL) {
			logprintf(obj, "error: read_cycles can't "
					"open dependency file\n");
			exit(-1);
		}
		mask = (uint64)1 << (dependency - 1);
	}

	/* read the number of cycles to expect */

	fread(&num_cycles, sizeof(uint32), (size_t)1, cycle_fp);
	cycle_list = (la_col_t *)xcalloc((size_t)num_cycles, sizeof(la_col_t));

	/* read the relation numbers for each cycle */

	for (i = curr_cycle = 0; i < num_cycles; i++) {

		la_col_t *c;

		if (fread(&num_relations, sizeof(uint32), (size_t)1, cycle_fp) != 1)
			break;

		if (num_relations > MAX_CYCLE_SIZE) {
			printf("error: cycle too large; corrupt file?\n");
			exit(-1);
		}

		if (fread(rel_index, sizeof(uint32), (size_t)num_relations, 
					cycle_fp) != num_relations)
			break;

		/* all the relation numbers for this cycle
		   have been read; save them and start the
		   count for the next cycle. If reading in 
		   relations to produce a particular dependency
		   from the linear algebra phase, skip any
		   cycles that will not appear in the dependency */

		if (dependency) {
			uint64 curr_dep;

			if (fread(&curr_dep, sizeof(uint64), 
						(size_t)1, dep_fp) == 0) {
				printf("dependency file corrupt\n");
				exit(-1);
			}
			if (!(curr_dep & mask))
				continue;
		}

		c = cycle_list + curr_cycle++;
		c->cycle.num_relations = num_relations;
		c->cycle.list = (uint32 *)xmalloc(num_relations * 
						sizeof(uint32));
		memcpy(c->cycle.list, rel_index, 
				num_relations * sizeof(uint32));
	}
	logprintf(obj, "read %u cycles\n", curr_cycle);
	num_cycles = curr_cycle;
	fclose(cycle_fp);
	if (dep_fp) {
		fclose(dep_fp);
	}
	if (num_cycles == 0) {
		free(cycle_list);
		*num_cycles_out = 0;
		*cycle_list_out = NULL;
		*num_relations_out = 0;
		*rlist_out = NULL;
		return;
	}

	cycle_list = (la_col_t *)xrealloc(cycle_list, 
				num_cycles * sizeof(la_col_t));
	if (fb == NULL || num_relations_out == NULL || rlist_out == NULL) {
		*num_cycles_out = num_cycles;
		*cycle_list_out = cycle_list;
		return;
	}

	/* now read the list of relations needed by the
	   list of cycles, and convert relation numbers
	   to relation pointers */

	nfs_get_cycle_relations(obj, fb, num_cycles, cycle_list, 
				&num_relations, &rlist, compress);

	/* count the number of times each relation is referenced */

	for (i = 0; i < num_cycles; i++) {
		la_col_t *c = cycle_list + i;
		for (j = 0; j < c->cycle.num_relations; j++) {
			relation_t *r = rlist + c->cycle.list[j];
			r->refcnt++;
		}
	}

	*num_cycles_out = num_cycles;
	*cycle_list_out = cycle_list;
	*num_relations_out = num_relations;
	*rlist_out = rlist;
}

/*--------------------------------------------------------------------*/
void nfs_free_relation_list(relation_t *rlist, uint32 num_relations) {

	uint32 i;

	for (i = 0; i < num_relations; i++)
		free(rlist[i].factors);
	free(rlist);
}
