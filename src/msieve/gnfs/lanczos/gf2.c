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

#include <common.h>
#include "gnfs.h"

/* the number of quadratic characters for each
   matrix column. A practical upper limit given
   in Buhler et. al. is 3 * log2(n), while 
   Bernstein writes that a QCB size of 50 'should 
   be enough for any possible NFS factorization'. 
   If each quadratic character reduces by half the
   odds that the linear algebra will not produce
   an algebraic square, then a very few characters
   (say 16) will be enough. The matrix-building code 
   is flexible enough so that this number may take 
   any positive value and everything else will 
   just work 
   
   The size of the QCB is limited to allow caching
   the quadratic characters */

#define QCB_SIZE 32

#if QCB_SIZE > 32
#error "QCB size must be 32 or less"
#endif

/*------------------------------------------------------------------*/
static int compare_ideals(const void *x, const void *y) {
	
	/* used to determine the ordering of two ideals.
	   Ordering is by prime, then by root of prime,
	   then by rational or algebraic type. The ordering 
	   by prime is tricky, because -1 has a special value 
	   that must be explicitly accounted for. This ordering
	   is designed to put the most dense matrix rows first */

	ideal_t *k = (ideal_t *)x;
	ideal_t *t = (ideal_t *)y;

	if (k->i.compressed_p == 0x7fffffff ||
	    t->i.compressed_p == 0x7fffffff) {
		if (k->i.compressed_p == 0x7fffffff &&
		    t->i.compressed_p != 0x7fffffff)
			return -1;
		else if (k->i.compressed_p != 0x7fffffff &&
		         t->i.compressed_p == 0x7fffffff)
			return 1;
		return 0;
	}

	if (k->i.compressed_p < t->i.compressed_p)
		return -1;
	if (k->i.compressed_p > t->i.compressed_p)
		return 1;
		
	if (k->i.r < t->i.r)
		return -1;
	if (k->i.r > t->i.r)
		return 1;

	if (k->i.rat_or_alg < t->i.rat_or_alg)
		return -1;
	if (k->i.rat_or_alg > t->i.rat_or_alg)
		return 1;
		
	return 0;
}

/*------------------------------------------------------------------*/
#define MAX_SMALL_IDEALS 200

static ideal_t *fill_small_ideals(factor_base_t *fb,
				uint32 *num_ideals_out,
				uint32 *max_small_ideal) {
	uint32 i, j;
	uint32 num_ideals;
	ideal_t *small_ideals;
	uint32 p;

	if (MAX_PACKED_PRIME == 0) {
		*num_ideals_out = 0;
		*max_small_ideal = 0;
		return NULL;
	}

	small_ideals = (ideal_t *)xmalloc(MAX_SMALL_IDEALS * sizeof(ideal_t));

	/* fill in the rational ideal of -1 */

	small_ideals[0].i.rat_or_alg = RATIONAL_IDEAL;
	small_ideals[0].i.compressed_p = 0x7fffffff;
	small_ideals[0].i.r = (uint32)(-1);
	num_ideals = 1;

	/* for each small prime */

	for (i = p = 0; i < PRECOMPUTED_NUM_PRIMES; i++) {

		uint32 num_roots_r;
		uint32 num_roots_a;
		uint32 roots_r[MAX_POLY_DEGREE + 1];
		uint32 roots_a[MAX_POLY_DEGREE + 1];
		uint32 high_coeff;

		if (p + prime_delta[i] >= MAX_PACKED_PRIME)
			break;

		/* count the number of ideals; all rational ideals
		   are only counted once */

		p += prime_delta[i];
		num_roots_r = poly_get_zeros(roots_r, &fb->rfb.poly, p, 
					&high_coeff, 0);
		if (high_coeff == 0 || num_roots_r > 0)
			num_roots_r = 1;

		num_roots_a = poly_get_zeros(roots_a, &fb->afb.poly, p, 
					&high_coeff, 0);
		if (high_coeff == 0)
			roots_a[num_roots_a++] = p;

		/* if there's room in the array, save the ideals */

		if (num_ideals + num_roots_r + 
				num_roots_a >= MAX_SMALL_IDEALS)
			break;

		if (num_roots_r > 0) {
			small_ideals[num_ideals].i.rat_or_alg = RATIONAL_IDEAL;
			small_ideals[num_ideals].i.compressed_p = (p - 1) / 2;
			small_ideals[num_ideals].i.r = p;
			num_ideals++;
		}
		for (j = 0; j < num_roots_a; j++) {
			small_ideals[num_ideals].i.rat_or_alg = ALGEBRAIC_IDEAL;
			small_ideals[num_ideals].i.compressed_p = (p - 1) / 2;
			small_ideals[num_ideals].i.r = roots_a[j];
			num_ideals++;
		}
	}

	/* put the ideals in order of increasing size */

	qsort(small_ideals, (size_t)num_ideals, 
			sizeof(ideal_t), compare_ideals);

	*num_ideals_out = num_ideals;
	*max_small_ideal = p;
	return small_ideals;
}

/*------------------------------------------------------------------*/
#define QCB_VALS(r) ((r)->rel_index)

static void fill_qcb(msieve_obj *obj, mp_poly_t *apoly, 
			relation_t *rlist, uint32 num_relations) {
	uint32 i, j;
	prime_sieve_t sieve;
	fb_entry_t qcb[QCB_SIZE];
	uint32 min_qcb_ideal;

	/* find the largest algebraic factor across the whole set */

	for (i = min_qcb_ideal = 0; i < num_relations; i++) {
		relation_t *r = rlist + i;
		uint32 *factors_a = r->factors + r->num_factors_r;
		for (j = 0; j < r->num_factors_a; j++)
			min_qcb_ideal = MAX(min_qcb_ideal, factors_a[j]);
	}

	/* choose the quadratic character base from the primes
	   larger than any that appear in the list of relations */

	logprintf(obj, "using %u quadratic characters above %u\n",
				QCB_SIZE, min_qcb_ideal + 1);

	/* construct the quadratic character base, starting
	   with the primes above min_qcb_ideal */

	init_prime_sieve(&sieve, min_qcb_ideal + 1, 0xffffffff);

	i = 0;
	while (i < QCB_SIZE) {
		uint32 roots[MAX_POLY_DEGREE];
		uint32 num_roots, high_coeff;
		uint32 p = get_next_prime(&sieve);

		num_roots = poly_get_zeros(roots, apoly, p, 
					&high_coeff, 0);

		/* p cannot be a projective root of the algebraic poly */

		if (high_coeff == 0)
			continue;

		/* save the next batch of roots */

		for (j = 0; i < QCB_SIZE && j < num_roots; i++, j++) {
			qcb[i].p = p;
			qcb[i].r = roots[j];
		}
	}

	free_prime_sieve(&sieve);

	/* cache each relation's quadratic characters for later use */

	for (i = 0; i < num_relations; i++) {
		relation_t *rel = rlist + i;
		int64 a = rel->a;
		uint32 b = rel->b;

		QCB_VALS(rel) = 0;
		for (j = 0; j < QCB_SIZE; j++) {
			uint32 p = qcb[j].p;
			uint32 r = qcb[j].r;
			int64 res = a % (int64)p;
			int32 symbol;

			if (res < 0)
				res += (int64)p;

			symbol = mp_legendre_1(mp_modsub_1((uint32)res,
					mp_modmul_1(b, r, p), p), p);

			/* symbol must be 1 or -1; if it's 0,
			   there's something wrong with the choice
			   of primes in the QCB but this isn't
			   a fatal error */

			if (symbol == -1)
				QCB_VALS(rel) |= 1 << (j % 32);
			else if (symbol == 0)
				printf("warning: zero character\n");
		}
	}
}

/*------------------------------------------------------------------*/
#define MAX_COL_IDEALS 1000

static uint32 combine_relations(la_col_t *col, relation_t *rlist,
				ideal_t *merged_ideals, uint32 *dense_rows,
				uint32 num_dense_rows) {

	uint32 i, j, k;
	uint32 num_merged = 0;
	ideal_t tmp_ideals[MAX_COL_IDEALS];
	uint32 num_tmp_ideals;

	/* form the matrix column corresponding to a 
	   collection of relations */

	for (i = 0; i < col->cycle.num_relations; i++) {
		relation_t *r = rlist + col->cycle.list[i];
		relation_lp_t new_ideals;

		/* fold in the quadratic characters for r */

		dense_rows[0] ^= QCB_VALS(r);

		/* if r is a free relation, modify the last dense row */

		if (r->b == 0) {
			dense_rows[(num_dense_rows - 1) / 32] ^=
					1 << ((num_dense_rows - 1) % 32);
		}

		/* get the ideal decomposition of relation i, sort
		   by size of prime */

		if (find_large_ideals(r, &new_ideals, 0, 0) > 
						TEMP_FACTOR_LIST_SIZE) {
			printf("error: overflow reading ideals\n");
			exit(-1);
		}
		if (num_merged + new_ideals.ideal_count >= MAX_COL_IDEALS) {
			printf("error: overflow merging ideals\n");
			exit(-1);
		}
		if (new_ideals.ideal_count > 1) {
			qsort(new_ideals.ideal_list, 
					(size_t)new_ideals.ideal_count,
					sizeof(ideal_t), compare_ideals);
		}

		/* merge it with the current list of ideals */

		j = k = 0;
		num_tmp_ideals = 0;
		while (j < num_merged && k < new_ideals.ideal_count) {
			int32 compare_result = compare_ideals(
						merged_ideals + j,
						new_ideals.ideal_list + k);
			if (compare_result < 0) {
				tmp_ideals[num_tmp_ideals++] = 
						merged_ideals[j++];
			}
			else if (compare_result > 0) {
				tmp_ideals[num_tmp_ideals++] = 
						new_ideals.ideal_list[k++];
			}
			else {
				j++; k++;
			}
		}
		while (j < num_merged) {
			tmp_ideals[num_tmp_ideals++] = merged_ideals[j++];
		}
		while (k < new_ideals.ideal_count) {
			tmp_ideals[num_tmp_ideals++] = 
						new_ideals.ideal_list[k++];
		}

		num_merged = num_tmp_ideals;
		memcpy(merged_ideals, tmp_ideals, 
				num_merged * sizeof(ideal_t));
	}

	/* fill in the parity row, and place at dense 
	   row position QCB_SIZE */

	if (col->cycle.num_relations % 2)
		dense_rows[QCB_SIZE / 32] |= 1 << (QCB_SIZE % 32);

	return num_merged;
}

/*------------------------------------------------------------------*/
#define LOG2_IDEAL_HASHTABLE_SIZE 21
#define MAX_DENSE_ROW_WORDS 32

static void build_matrix_core(msieve_obj *obj, la_col_t *cycle_list, 
			uint32 num_cycles, relation_t *rlist, 
			uint32 num_relations, uint32 num_dense_rows, 
			ideal_t *small_ideals, uint32 num_small_ideals, 
			FILE *matrix_fp) {

	uint32 i, j, k;
	hashtable_t unique_ideals;
	uint32 max_small_ideal;
	uint32 dense_rows[MAX_DENSE_ROW_WORDS];
	uint32 dense_row_words;
	size_t mem_use;

	logprintf(obj, "building initial matrix\n");

	dense_row_words = (num_dense_rows + 31) / 32;
	if (dense_row_words > MAX_DENSE_ROW_WORDS) {
		printf("error: too many dense rows\n");
		exit(-1);
	}

	max_small_ideal = 0;
	if (num_small_ideals > 0) {
		max_small_ideal = small_ideals[
					num_small_ideals - 1].i.compressed_p;
	}

	hashtable_init(&unique_ideals, LOG2_IDEAL_HASHTABLE_SIZE,
			10000, (uint32)(sizeof(ideal_t)/sizeof(uint32)));

	fseek(matrix_fp, 3 * sizeof(uint32), SEEK_SET);

	/* for each cycle */

	for (i = 0; i < num_cycles; i++) {
		la_col_t *c = cycle_list + i;
		ideal_t merged_ideals[MAX_COL_IDEALS];
		uint32 mapped_ideals[MAX_COL_IDEALS];
		uint32 num_merged;

		/* dense rows start off empty */

		for (j = 0; j < dense_row_words; j++)
			dense_rows[j] = 0;

		/* merge the relations and quadratic characters
		   in the cycle */

		num_merged = combine_relations(c, rlist, merged_ideals, 
						dense_rows, num_dense_rows);

		/* assign a unique number to each ideal in 
		   the cycle. This will automatically ignore
		   empty rows in the matrix */

		for (j = k = 0; j < num_merged; j++) {
			ideal_t *ideal = merged_ideals + j;
			uint32 p = ideal->i.compressed_p;

			if (max_small_ideal > 0 && (p == 0x7fffffff || 
					p <= max_small_ideal) ) {
				/* dense ideal; store in compressed format */
				ideal_t *loc = (ideal_t *)bsearch(ideal, 
						small_ideals,
						(size_t)num_small_ideals,
						sizeof(ideal_t),
						compare_ideals);
				uint32 idx = QCB_SIZE + 1 +
						(loc - small_ideals);
				if (loc == NULL) {
					printf("error: unexpected dense ideal "
						"(%08x,%08x) found\n",
						ideal->blob[0], ideal->blob[1]);
					exit(-1);
				}
				dense_rows[idx / 32] |= 1 << (idx % 32);
			}
			else {
				hash_t *entry = hashtable_find(&unique_ideals, 
							ideal->blob, NULL);
				mapped_ideals[k++] = num_dense_rows +
						hashtable_get_offset(
							&unique_ideals, entry);
			}
		}

		/* save the matrix entries to disk */

		fwrite(&k, sizeof(uint32), (size_t)1, matrix_fp);
		fwrite(mapped_ideals, sizeof(uint32), (size_t)k, matrix_fp);
		fwrite(dense_rows, sizeof(uint32), 
				(size_t)dense_row_words, matrix_fp);
	}

	/* save the matrix dimensions to disk */

	i = num_dense_rows + hashtable_getall(&unique_ideals, NULL);
	rewind(matrix_fp);
	fwrite(&i, sizeof(uint32), (size_t)1, matrix_fp);
	fwrite(&num_dense_rows, sizeof(uint32), (size_t)1, matrix_fp);
	fwrite(&num_cycles, sizeof(uint32), (size_t)1, matrix_fp);

	/* report memory use */

	mem_use = num_relations * sizeof(relation_t) +
			num_cycles * sizeof(la_col_t) +
			hashtable_sizeof(&unique_ideals);

	for (i = 0; i < num_cycles; i++) {
		la_col_t *c = cycle_list + i;
		mem_use += c->cycle.num_relations * sizeof(uint32);
	}
	for (i = 0; i < num_relations; i++) {
		relation_t *r = rlist + i;
		mem_use += (r->num_factors_r + r->num_factors_a) *
				sizeof(uint32);
	}
	logprintf(obj, "memory use: %.1f MB\n", (double)mem_use / 1048576);

	hashtable_free(&unique_ideals);
}

/*------------------------------------------------------------------*/
static void build_matrix(msieve_obj *obj, mp_t *n) {

	/* read in the relations that contribute to the
	   initial matrix, and form the quadratic characters
	   for each column */

	uint32 num_relations;
	relation_t *rlist;
	uint32 num_cycles;
	la_col_t *cycle_list;
	uint32 num_dense_rows;
	ideal_t *small_ideals;
	uint32 num_small_ideals;
	uint32 max_small_ideal;
	FILE *matrix_fp;
	char buf[256];
	factor_base_t fb;

	sprintf(buf, "%s.mat", obj->savefile.name);
	matrix_fp = fopen(buf, "w+b");
	if (matrix_fp == NULL) {
		logprintf(obj, "error: can't open matrix file '%s'\n", buf);
		exit(-1);
	}

	/* read in the NFS polynomials */

	memset(&fb, 0, sizeof(fb));
	if (read_poly(obj, n, &fb.rfb.poly, &fb.afb.poly)) {
		printf("error: failed to read NFS polynomials\n");
		exit(-1);
	}

	/* fill in the small ideals */

	small_ideals = fill_small_ideals(&fb, &num_small_ideals,
					&max_small_ideal);

	/* we need extra matrix rows to make sure that each
	   dependency has an even number of relations, and also an
	   even number of free relations. If the rational 
	   poly R(x) is monic, and we weren't using free relations,
	   the sign of R(x) is negative for all relations, meaning we 
	   already get the effect of the extra rows. However, in 
	   general we can't assume both of these are true */
	
	num_dense_rows = QCB_SIZE + 2 + num_small_ideals;

	/* read in the cycles that form the matrix columns,
	   and the relations they will need */

	nfs_read_cycles(obj, &fb, &num_cycles, &cycle_list, 
			&num_relations, &rlist, 1, 0);

	/* assign quadratic characters to each relation */

	fill_qcb(obj, &fb.afb.poly, rlist, num_relations);

	/* build the matrix columns, store to disk */

	build_matrix_core(obj, cycle_list, num_cycles, rlist, 
			num_relations, num_dense_rows, 
			small_ideals, num_small_ideals, 
			matrix_fp);

	nfs_free_relation_list(rlist, num_relations);
	free_cycle_list(cycle_list, num_cycles);
	free(small_ideals);
	fclose(matrix_fp);
}

/*--------------------------------------------------------------------*/
static void dump_cycles(msieve_obj *obj, la_col_t *cols, uint32 ncols) {

	uint32 i;
	char buf[256];
	FILE *cycle_fp;

	sprintf(buf, "%s.cyc", obj->savefile.name);
	cycle_fp = fopen(buf, "wb");
	if (cycle_fp == NULL) {
		logprintf(obj, "error: can't open cycle file\n");
		exit(-1);
	}

	fwrite(&ncols, sizeof(uint32), (size_t)1, cycle_fp);

	for (i = 0; i < ncols; i++) {
		la_col_t *c = cols + i;
		uint32 num = c->cycle.num_relations;
		
		fwrite(&num, sizeof(uint32), (size_t)1, cycle_fp);
		fwrite(c->cycle.list, sizeof(uint32), (size_t)num, cycle_fp);
	}
	fclose(cycle_fp);
}

/*--------------------------------------------------------------------*/
static void dump_matrix(msieve_obj *obj, 
			uint32 nrows, uint32 num_dense_rows,
			uint32 ncols, la_col_t *cols) {

	uint32 i;
	uint32 dense_row_words;
	char buf[256];
	FILE *matrix_fp;

	sprintf(buf, "%s.mat", obj->savefile.name);
	matrix_fp = fopen(buf, "wb");
	if (matrix_fp == NULL) {
		logprintf(obj, "error: can't open matrix file\n");
		exit(-1);
	}

	fwrite(&nrows, sizeof(uint32), (size_t)1, matrix_fp);
	fwrite(&num_dense_rows, sizeof(uint32), (size_t)1, matrix_fp);
	fwrite(&ncols, sizeof(uint32), (size_t)1, matrix_fp);
	dense_row_words = (num_dense_rows + 31) / 32;

	for (i = 0; i < ncols; i++) {
		la_col_t *c = cols + i;
		uint32 num = c->weight + dense_row_words;
		
		fwrite(&c->weight, sizeof(uint32), (size_t)1, matrix_fp);
		fwrite(c->data, sizeof(uint32), (size_t)num, matrix_fp);
	}
	fclose(matrix_fp);
}

/*--------------------------------------------------------------------*/
static void read_matrix(msieve_obj *obj, 
			uint32 *nrows_out, uint32 *num_dense_rows_out,
			uint32 *ncols_out, la_col_t **cols_out) {

	uint32 i;
	uint32 dense_row_words;
	uint32 ncols;
	la_col_t *cols;
	char buf[256];
	FILE *matrix_fp;

	nfs_read_cycles(obj, NULL, &ncols, &cols, NULL, NULL, 1, 0);

	sprintf(buf, "%s.mat", obj->savefile.name);
	matrix_fp = fopen(buf, "rb");
	if (matrix_fp == NULL) {
		logprintf(obj, "error: can't open matrix file\n");
		exit(-1);
	}

	fread(nrows_out, sizeof(uint32), (size_t)1, matrix_fp);
	fread(num_dense_rows_out, sizeof(uint32), (size_t)1, matrix_fp);
	fread(ncols_out, sizeof(uint32), (size_t)1, matrix_fp);
	dense_row_words = (*num_dense_rows_out + 31) / 32;
	if (*ncols_out != ncols) {
		printf("error: cycle file not in sync with matrix file\n");
		exit(-1);
	}

	for (i = 0; i < ncols; i++) {
		la_col_t *c = cols + i;
		uint32 num;
		
		fread(&num, sizeof(uint32), (size_t)1, matrix_fp);
		c->weight = num;
		c->data = (uint32 *)xmalloc((num + dense_row_words) * 
					sizeof(uint32));
		fread(c->data, sizeof(uint32), 
				(size_t)(num + dense_row_words), matrix_fp);
	}
	fclose(matrix_fp);
	*cols_out = cols;
}

/*--------------------------------------------------------------------*/
static void dump_dependencies(msieve_obj *obj, 
			uint64 *deps, uint32 ncols) {

	char buf[256];
	FILE *deps_fp;

	/* we allow up to 64 dependencies, even though the
	   average case will have (64 - POST_LANCZOS_ROWS) */

	sprintf(buf, "%s.dep", obj->savefile.name);
	deps_fp = fopen(buf, "wb");
	if (deps_fp == NULL) {
		logprintf(obj, "error: can't open deps file\n");
		exit(-1);
	}

	fwrite(deps, sizeof(uint64), (size_t)ncols, deps_fp);
	fclose(deps_fp);
}

/*------------------------------------------------------------------*/
void nfs_solve_linear_system(msieve_obj *obj, mp_t *n) {

	uint32 i;
	la_col_t *cols;
	uint32 nrows, ncols; 
	uint32 num_dense_rows;
	uint32 deps_found;
	uint64 *dependencies;

	logprintf(obj, "\n");
	logprintf(obj, "commencing linear algebra\n");

	/* convert the list of relations from the sieving 
	   stage into a matrix */

	if (!(obj->flags & MSIEVE_FLAG_NFS_LA_RESTART)) {
		build_matrix(obj, n);
		read_matrix(obj, &nrows, &num_dense_rows, &ncols, &cols);
		count_matrix_nonzero(obj, nrows, num_dense_rows, ncols, cols);
		reduce_matrix(obj, &nrows, num_dense_rows, &ncols, 
				cols, NUM_EXTRA_RELATIONS);
		if (ncols == 0) {
			logprintf(obj, "matrix is corrupt; skipping "
					"linear algebra\n");
			free(cols);
			return;
		}
		dump_cycles(obj, cols, ncols);
		dump_matrix(obj, nrows, num_dense_rows, ncols, cols);

		/* clear off in-memory structures to defragment the heap */
		for (i = 0; i < ncols; i++) {
			free(cols[i].data);
			free(cols[i].cycle.list);
		}
		free(cols);
	}

	read_matrix(obj, &nrows, &num_dense_rows, &ncols, &cols);
	count_matrix_nonzero(obj, nrows, num_dense_rows, ncols, cols);

	/* the matrix is read-only from here on, so conserve memory
	   by deleting the list of relations that comprise each column */

	for (i = 0; i < ncols; i++) {
		free(cols[i].cycle.list);
		cols[i].cycle.list = NULL;
	}

	/* solve the linear system */

	dependencies = block_lanczos(obj, nrows, num_dense_rows,
					ncols, cols, &deps_found);
	if (deps_found)
		dump_dependencies(obj, dependencies, ncols);
	free(dependencies);
	free(cols);
}
