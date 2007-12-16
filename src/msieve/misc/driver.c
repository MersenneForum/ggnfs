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

$Id: driver.c,v 1.2 2007-12-16 18:24:24 jasonp_sf Exp $
----------------------------------------------------------------------*/

#include <common.h>

/*--------------------------------------------------------------------*/
msieve_obj * msieve_obj_new(char *input_integer, uint32 flags,
			    char *savefile_name, char *logfile_name,
			    char *nfs_fbfile_name,
			    uint32 seed1, uint32 seed2, uint32 max_relations,
			    uint32 nfs_lower, uint32 nfs_upper,
			    enum cpu_type cpu,
			    uint32 cache_size1, uint32 cache_size2,
			    uint32 num_threads) {

	msieve_obj *obj = (msieve_obj *)xcalloc((size_t)1, sizeof(msieve_obj));

	if (obj == NULL)
		return obj;
	
	obj->input = input_integer;
	obj->flags = flags;
	obj->seed1 = seed1;
	obj->seed2 = seed2;
	obj->max_relations = max_relations;
	obj->nfs_lower = nfs_lower;
	obj->nfs_upper = nfs_upper;
	obj->cpu = cpu;
	obj->cache_size1 = cache_size1;
	obj->cache_size2 = cache_size2;
	obj->num_threads = num_threads;
	obj->savefile_name = MSIEVE_DEFAULT_SAVEFILE;
	if (savefile_name)
		obj->savefile_name = savefile_name;
	obj->logfile_name = MSIEVE_DEFAULT_LOGFILE;
	if (logfile_name)
		obj->logfile_name = logfile_name;
	obj->nfs_fbfile_name = MSIEVE_DEFAULT_NFS_FBFILE;
	if (nfs_fbfile_name)
		obj->nfs_fbfile_name = nfs_fbfile_name;
	
	obj->savefile_buf = (char *)xmalloc((size_t)SAVEFILE_BUF_SIZE);
	if (obj->savefile_buf == NULL) {
		free(obj);
		return NULL;
	}
	obj->savefile_buf[0] = 0;
	return obj;
}

/*--------------------------------------------------------------------*/
msieve_obj * msieve_obj_free(msieve_obj *obj) {

	msieve_factor *curr_factor;

	curr_factor = obj->factors;
	while (curr_factor != NULL) {
		msieve_factor *next_factor = curr_factor->next;
		free(curr_factor->number);
		free(curr_factor);
		curr_factor = next_factor;
	}

	free(obj->savefile_buf);
	free(obj);
	return NULL;
}

/*--------------------------------------------------------------------*/
void msieve_run(msieve_obj *obj) {

	char *n_string;
	int32 status;
	uint32 i;
	mp_t n;
	factor_list_t factor_list;

	/* convert the input number to an mp_t */

	status = evaluate_expression(obj->input, &n);
	if (status < 0 || mp_is_zero(&n)) {
		printf("error %d converting '%s'\n", status, obj->input);
		obj->flags |= MSIEVE_FLAG_FACTORIZATION_DONE;
		return;
	}
	n_string = mp_sprintf(&n, 10, obj->mp_sprintf_buf);

	/* print startup banner */

	logprintf(obj, "\n");
	logprintf(obj, "\n");
	logprintf(obj, "Msieve v. %d.%02d\n", MSIEVE_MAJOR_VERSION, 
					MSIEVE_MINOR_VERSION);
	obj->timestamp = time(NULL);
	if (obj->flags & MSIEVE_FLAG_LOG_TO_STDOUT) {
		printf("%s", ctime(&obj->timestamp));
	}

	logprintf(obj, "random seeds: %08x %08x\n", obj->seed1, obj->seed2);
	logprintf(obj, "factoring %s (%d digits)\n", 
				n_string, strlen(n_string));

	/* handle trivial inputs */

	if (mp_is_zero(&n) || mp_is_one(&n)) {
		add_next_factor(obj, &n, MSIEVE_PRIME);
		obj->flags |= MSIEVE_FLAG_FACTORIZATION_DONE;
		return;
	}

	/* perform NFS */

	factor_list_init(&factor_list);
	if (factor_list_add(obj, &factor_list, &n) > 0)
		factor_gnfs(obj, &n, &factor_list);

	factor_list_free(&n, &factor_list, obj);
	if (!(obj->flags & MSIEVE_FLAG_STOP_SIEVING))
		obj->flags |= MSIEVE_FLAG_FACTORIZATION_DONE;
	obj->timestamp = i = (uint32)(time(NULL) - obj->timestamp);
	logprintf(obj, "elapsed time %02d:%02d:%02d\n", i / 3600,
				(i % 3600) / 60, i % 60);
}

/*--------------------------------------------------------------------*/
void add_next_factor(msieve_obj *obj, mp_t *n, 
			enum msieve_factor_type factor_type) {

	msieve_factor *new_factor = (msieve_factor *)xmalloc(
					sizeof(msieve_factor));
	char *type_string;
	char *tmp;
	size_t len;

	if (factor_type == MSIEVE_PRIME)
		type_string = "p";
	else if (factor_type == MSIEVE_COMPOSITE)
		type_string = "c";
	else
		type_string = "prp";

	/* Copy n. We could use strdup(), but that causes
	   warnings for gcc on AMD64 */

	tmp = mp_sprintf(n, 10, obj->mp_sprintf_buf);
	len = strlen(tmp) + 1;
	new_factor->number = (char *)xmalloc((size_t)len);
	memcpy(new_factor->number, tmp, len);

	new_factor->factor_type = factor_type;
	new_factor->next = NULL;

	if (obj->factors != NULL) {
		msieve_factor *curr_factor = obj->factors;
		while (curr_factor->next != NULL)
			curr_factor = curr_factor->next;
		curr_factor->next = new_factor;
	}
	else {
		obj->factors = new_factor;
	}

	logprintf(obj, "%s%d factor: %s\n", type_string, 
				(int32)(len - 1),
				new_factor->number);
}

/*--------------------------------------------------------------------*/
void logprintf(msieve_obj *obj, char *fmt, ...) {

	va_list ap;

	/* do *not* initialize 'ap' and use it twice; this
	   causes crashes on AMD64 */

	if (obj->flags & MSIEVE_FLAG_USE_LOGFILE) {
		time_t t = time(NULL);
		char buf[64];
		FILE *logfile = fopen(obj->logfile_name, "a");

		if (logfile == NULL) {
			fprintf(stderr, "cannot open logfile\n");
			exit(-1);
		}

		va_start(ap, fmt);
		buf[0] = 0;
		strcpy(buf, ctime(&t));
		*(strchr(buf, '\n')) = 0;
		fprintf(logfile, "%s  ", buf);
		vfprintf(logfile, fmt, ap);
		fclose(logfile);
		va_end(ap);
	}
	if (obj->flags & MSIEVE_FLAG_LOG_TO_STDOUT) {
		va_start(ap, fmt);
		vfprintf(stdout, fmt, ap);
		va_end(ap);
	}
}

/*--------------------------------------------------------------------*/
void fill_prime_list(prime_list_t *prime_list,
			uint32 max_list_size,
			uint32 max_prime) {
	uint32 i, j;
	uint32 num_alloc;
	prime_sieve_t prime_sieve;
	uint32 *list;

	num_alloc = 1500;
	list = prime_list->list = (uint32 *)xmalloc(
					num_alloc * sizeof(uint32));

	init_prime_sieve(&prime_sieve, 0, max_prime);
	for (i = j = 0; i < max_list_size; i++) {
		uint32 prime = get_next_prime(&prime_sieve);

		if (prime > max_prime)
			break;
		if (j == num_alloc) {
			num_alloc *= 2;
			list = prime_list->list = (uint32 *)xrealloc(
					prime_list->list,
					num_alloc * sizeof(uint32));
		}
		list[j++] = prime;
	}
	prime_list->num_primes = j;
	free_prime_sieve(&prime_sieve);
}

/*--------------------------------------------------------------------*/
void factor_list_init(factor_list_t *list) {

	memset(list, 0, sizeof(factor_list_t));
}

/*--------------------------------------------------------------------*/
uint32 factor_list_max_composite(factor_list_t *list) {

	uint32 i, bits;

	/* Find the number of bits in the largest composite factor, 
	   and return that (to give calling code an estimate of 
	   how much work would be left if it stopped trying to 
	   find new factors now) */

	for (i = bits = 0; i < list->num_factors; i++) {
		final_factor_t *curr_factor = list->final_factors[i];

		if (curr_factor->type == MSIEVE_COMPOSITE) {
			uint32 curr_bits = mp_bits(&curr_factor->factor);
			bits = MAX(bits, curr_bits);
		}
	}

	return bits;
}

/*--------------------------------------------------------------------*/
static int compare_factors (const void *x, const void *y) {
	final_factor_t **xx = (final_factor_t **)x;
	final_factor_t **yy = (final_factor_t **)y;
	return mp_cmp(&((*xx)->factor), &((*yy)->factor));
}

void factor_list_free(mp_t *n, factor_list_t *list, msieve_obj *obj) {

	uint32 i;
	mp_t q, r, tmp;

	/* if there's only one factor and it's listed as composite,
	   don't save anything (it would just confuse people) */

	if (list->num_factors == 1 && 
	    list->final_factors[0]->type == MSIEVE_COMPOSITE) {
		free(list->final_factors[0]);
		return;
	}	

	/* sort the factors in order of increasing size */

	if (list->num_factors > 1)
		qsort(list->final_factors, (size_t)list->num_factors, 
				sizeof(final_factor_t *), compare_factors);

	/* report each factor every time it appears in n */

	mp_copy(n, &tmp);
	for (i = 0; i < list->num_factors; i++) {
		final_factor_t *curr_factor = list->final_factors[i];

		while (1) {
			mp_divrem(&tmp, &curr_factor->factor, &q, &r);
			if (mp_is_zero(&q) || !mp_is_zero(&r))
				break;

			mp_copy(&q, &tmp);
			add_next_factor(obj, &curr_factor->factor, 
					curr_factor->type);
		}

		free(curr_factor);
	}
}

/*--------------------------------------------------------------------*/
static void factor_list_add_core(msieve_obj *obj, 
				factor_list_t *list, 
				mp_t *new_factor) {

	/* recursive routine to do the actual adding of factors.
	   Upon exit, the factors in 'list' will be mutually coprime */

	uint32 i;
	mp_t tmp1, tmp2, common, q, r;
	uint32 num_factors = list->num_factors;

	mp_clear(&tmp1); tmp1.nwords = tmp1.val[0] = 1;
	mp_clear(&tmp2); tmp2.nwords = tmp2.val[0] = 1;
	mp_clear(&common);

	/* compare new_factor to all the factors 
	   already in the list */

	for (i = 0; i < num_factors; i++) {
		final_factor_t *curr_factor = list->final_factors[i];

		/* skip new_factor if element i of the current
		   list would duplicate it */

		if (mp_cmp(new_factor, &curr_factor->factor) == 0)
			return;

		/* if new_factor has a factor C in common with element
		   i in the list, remove all instances of C, remove 
		   factor i, compress the list and postprocess */

		mp_gcd(new_factor, &curr_factor->factor, &common);
		if (!mp_is_one(&common)) {

			mp_copy(new_factor, &tmp1);
			while (1) {
				mp_divrem(&tmp1, &common, &q, &r);
				if (mp_is_zero(&q) || !mp_is_zero(&r))
					break;
				mp_copy(&q, &tmp1);
			}

			mp_copy(&curr_factor->factor, &tmp2);
			while (1) {
				mp_divrem(&tmp2, &common, &q, &r);
				if (mp_is_zero(&q) || !mp_is_zero(&r))
					break;
				mp_copy(&q, &tmp2);
			}

			free(list->final_factors[i]);
			list->final_factors[i] = 
				list->final_factors[--list->num_factors];
			break;
		}
	}

	if (i < num_factors) {

		/* there is overlap between new_factor and one
		   of the factors previously found. In the worst
		   case there are three new factors to deal with */

		if (!mp_is_one(&tmp1))
			factor_list_add_core(obj, list, &tmp1);
		if (!mp_is_one(&tmp2))
			factor_list_add_core(obj, list, &tmp2);
		if (!mp_is_one(&common))
			factor_list_add_core(obj, list, &common);
	}
	else {
		/* list doesn't need to be modified, except
		   to append new_factor. We go to some trouble to
		   avoid unnecessary primality tests when new_factor
		   is small */

		i = list->num_factors++;
		list->final_factors[i] = (final_factor_t *)xmalloc(
						sizeof(final_factor_t));
		if (new_factor->nwords <= 2 &&
		    ((uint64)new_factor->val[1] << 32 | 
					new_factor->val[0]) <
		    ((uint64)PRECOMPUTED_PRIME_BOUND * 
		      			PRECOMPUTED_PRIME_BOUND)) {
			list->final_factors[i]->type = MSIEVE_PRIME;
		}
		else {
			list->final_factors[i]->type = (mp_is_prime(
						new_factor, 
						&obj->seed1, &obj->seed2)) ?
						MSIEVE_PROBABLE_PRIME : 
						MSIEVE_COMPOSITE;
		}
		mp_copy(new_factor, &(list->final_factors[i]->factor));
	}
}

/*--------------------------------------------------------------------*/
uint32 factor_list_add(msieve_obj *obj, factor_list_t *list, 
				mp_t *new_factor) {

	if (!mp_is_zero(new_factor) && !mp_is_one(new_factor))
		factor_list_add_core(obj, list, new_factor);

	return factor_list_max_composite(list);
}

/*--------------------------------------------------------------------*/
void free_cycle_list(la_col_t *cycle_list, uint32 num_cycles) {

	uint32 i;

	for (i = 0; i < num_cycles; i++)
		free(cycle_list[i].cycle.list);
	free(cycle_list);
}

/*------------------------------------------------------------------*/
uint32 merge_relations(uint32 *merge_array,
		  uint32 *src1, uint32 n1,
		  uint32 *src2, uint32 n2) {

	/* Given two sorted lists of integers, merge
	   the lists into a single sorted list. We assume
	   each list contains no duplicate entries.
	   If a particular entry occurs in both lists,
	   don't add it to the final list at all. Returns 
	   the number of elements in the resulting list */

	uint32 i1, i2;
	uint32 num_merge;

	i1 = i2 = 0;
	num_merge = 0;

	while (i1 < n1 && i2 < n2) {
		uint32 val1 = src1[i1];
		uint32 val2 = src2[i2];

		if (val1 < val2) {
			merge_array[num_merge++] = val1;
			i1++;
		}
		else if (val1 > val2) {
			merge_array[num_merge++] = val2;
			i2++;
		}
		else {
			i1++;
			i2++;
		}
	}

	while (i1 < n1)
		merge_array[num_merge++] = src1[i1++];
	while (i2 < n2)
		merge_array[num_merge++] = src2[i2++];

	return num_merge;
}

