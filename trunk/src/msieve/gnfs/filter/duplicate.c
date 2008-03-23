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

#include "filter.h"

/* produce <savefile_name>.d, a binary file containing the
   line numbers of relations in the savefile that should *not*
   graduate to the singleton removal (or are just plain invalid).

   This code has to touch all of the relations, and when the
   dataset is large avoiding excessive memory use is tricky.
   Cavallar's paper suggests dropping relations into a hashtable
   and ignoring relations that map to the same hash bin. This keeps
   memory use down but causes good relations to be thrown away.
   Another option is to put the (a,b) values of relations into the
   hashtable, so that hash collisions can be resolved rigorously.
   Unfortunately this means we have to budget 12 bytes for each
   unique relation, and there could be tens (hundreds!) of millions 
   of them.

   The implementation here is a compromise: we do duplicate removal
   in two passes. The first pass maps relations into a hashtable
   of bits, and we save (on disk) the list of hash bins where two 
   or more relations collide. The second pass refills the hashtable
   of bits with just these entries, then reads through the complete
   dataset again and saves the (a,b) values of any relation that
   maps to one of the filled-in hash bins. The memory use in the
   first pass is constant, and the memory use of the second pass
   is 12 bytes per duplicate relation. Assuming unique relations
   greatly outnumber duplicates, this solution finds all the duplicates
   with no false positives, and the memory use is low enough so
   that singleton filtering is a larger memory bottleneck 
   
   One useful optimization for really big problems would turn the
   first-pass hashtable into a Bloom filter using several hash
   functions. This would make it much more effective at avoiding
   false positives as the hashtable gets more congested */

#define LOG2_DUP_HASHTABLE1_SIZE 28   /* first pass hashtable size */
#define LOG2_DUP_HASHTABLE2_SIZE 20   /* second pass hashtable size */

static const uint8 hashmask[] = {0x01, 0x02, 0x04, 0x08,
				 0x10, 0x20, 0x40, 0x80};

static void purge_duplicates_pass2(msieve_obj *obj) {

	savefile_t *savefile = &obj->savefile;
	FILE *bad_relation_fp;
	FILE *collision_fp;
	FILE *out_fp;
	uint32 i;
	char buf[LINE_BUF_SIZE];
	uint32 num_duplicates;
	uint32 num_relations;
	uint32 next_bad_relation;
	uint32 curr_relation;
	uint8 *bit_table;
	hashtable_t duplicates;

	logprintf(obj, "commencing duplicate removal, pass 2\n");

	/* fill in the list of hash collisions */

	sprintf(buf, "%s.hc", savefile->name);
	collision_fp = fopen(buf, "rb");
	if (collision_fp == NULL) {
		logprintf(obj, "error: dup2 can't open collision file\n");
		exit(-1);
	}
	bit_table = (uint8 *)xcalloc(
			(size_t)1 << (LOG2_DUP_HASHTABLE1_SIZE - 3), 
			(size_t)1);

	while (fread(&i, (size_t)1, sizeof(uint32), collision_fp) != 0) {
		if (i < (1 << LOG2_DUP_HASHTABLE1_SIZE)) {
			bit_table[i / 8] |= 1 << (i % 8);
		}
	}
	fclose(collision_fp);

	/* set up for reading the list of relations */

	savefile_open(savefile, SAVEFILE_READ);
	sprintf(buf, "%s.br", savefile->name);
	bad_relation_fp = fopen(buf, "rb");
	if (bad_relation_fp == NULL) {
		logprintf(obj, "error: dup2 can't open rel file\n");
		exit(-1);
	}
	sprintf(buf, "%s.d", savefile->name);
	out_fp = fopen(buf, "wb");
	if (out_fp == NULL) {
		logprintf(obj, "error: dup2 can't open output file\n");
		exit(-1);
	}
	hashtable_init(&duplicates, LOG2_DUP_HASHTABLE2_SIZE, 10000, 2);

	num_duplicates = 0;
	num_relations = 0;
	curr_relation = (uint32)(-1);
	next_bad_relation = (uint32)(-1);
	fread(&next_bad_relation, (size_t)1, 
			sizeof(uint32), bad_relation_fp);
	savefile_read_line(buf, sizeof(buf), savefile);

	while (!savefile_eof(savefile)) {
		
		uint32 hashval;
		int64 a;
		uint32 b;
		uint32 key[2];
		char *next_field;

		if (buf[0] != '-' && !isdigit(buf[0])) {

			/* no relation on this line */

			savefile_read_line(buf, sizeof(buf), savefile);
			continue;
		}
		if (++curr_relation == next_bad_relation) {

			/* this relation isn't valid; save it and
			   read in the next invalid relation line number */

			fwrite(&curr_relation, (size_t)1, 
					sizeof(uint32), out_fp);
			fread(&next_bad_relation, (size_t)1, 
					sizeof(uint32), bad_relation_fp);
			savefile_read_line(buf, sizeof(buf), savefile);
			continue;
		}

		/* determine if the (a,b) coordinates of the
		   relation collide in the table of bits */

		a = (int64)strtod(buf, &next_field);
		b = strtoul(next_field + 1, NULL, 10);
		key[0] = (uint32)a;
		key[1] = ((a >> 32) & 0x1f) | (b << 5);

		hashval = (HASH1(key[0]) ^ HASH2(key[1])) >>
				(32 - LOG2_DUP_HASHTABLE1_SIZE);

		if (bit_table[hashval/8] & hashmask[hashval % 8]) {

			/* relation collides in the first hashtable;
			   use the second hashtable to determine 
			   rigorously if the relation was previously seen */

			uint32 is_dup;
			hashtable_find(&duplicates, key, &is_dup);

			if (!is_dup) {

				/* relation was seen for the first time;
				   doesn't count as a duplicate */

				num_relations++;
			}
			else {
				/* relation was previously seen; this
				   time it's a duplicate */

				fwrite(&curr_relation, (size_t)1, 
						sizeof(uint32), out_fp);
				num_duplicates++;
			}
		}
		else {
			/* no collision; relation is unique */

			num_relations++;
		}

		savefile_read_line(buf, sizeof(buf), savefile);
	}

	logprintf(obj, "found %u duplicates and %u unique relations\n", 
				num_duplicates, num_relations);
	logprintf(obj, "memory use: %.1f MB\n", 
			(double)((1 << (LOG2_DUP_HASHTABLE1_SIZE-3)) +
			hashtable_sizeof(&duplicates)) / 1048576);

	/* clean up and finish */

	savefile_close(savefile);
	fclose(bad_relation_fp);
	fclose(out_fp);
	sprintf(buf, "%s.hc", savefile->name);
	remove(buf);
	sprintf(buf, "%s.br", savefile->name);
	remove(buf);

	free(bit_table);
	hashtable_free(&duplicates);
}

/*--------------------------------------------------------------------*/
#define LOG2_BIN_SIZE 17
#define BIN_SIZE (1 << (LOG2_BIN_SIZE))
#define TARGET_HITS_PER_PRIME 40.0

uint32 nfs_purge_duplicates(msieve_obj *obj, factor_base_t *fb,
				uint32 max_relations) {

	uint32 i;
	savefile_t *savefile = &obj->savefile;
	FILE *bad_relation_fp;
	FILE *collision_fp;
	uint32 curr_relation;
	char buf[LINE_BUF_SIZE];
	uint32 num_relations;
	uint32 num_collisions;
	uint8 *hashtable;
	uint32 blob[2];

	uint32 *prime_bins;
	double bin_max;

	uint32 tmp_factors[TEMP_FACTOR_LIST_SIZE];
	relation_t tmp_relation;

	tmp_relation.factors = tmp_factors;

	logprintf(obj, "commencing duplicate removal, pass 1\n");

	savefile_open(savefile, SAVEFILE_READ);
	sprintf(buf, "%s.br", savefile->name);
	bad_relation_fp = fopen(buf, "wb");
	if (bad_relation_fp == NULL) {
		logprintf(obj, "error: dup1 can't open relation file\n");
		exit(-1);
	}
	sprintf(buf, "%s.hc", savefile->name);
	collision_fp = fopen(buf, "wb");
	if (collision_fp == NULL) {
		logprintf(obj, "error: dup1 can't open collision file\n");
		exit(-1);
	}
	hashtable = (uint8 *)xcalloc((size_t)1 << 
				(LOG2_DUP_HASHTABLE1_SIZE - 3), (size_t)1);
	prime_bins = (uint32 *)xcalloc((size_t)1 << (32 - LOG2_BIN_SIZE),
					sizeof(uint32));

	curr_relation = (uint32)(-1);
	num_relations = 0;
	num_collisions = 0;
	savefile_read_line(buf, sizeof(buf), savefile);
	while (!savefile_eof(savefile)) {

		int32 status;
		uint32 hashval;

		if (buf[0] != '-' && !isdigit(buf[0])) {

			/* no relation on this line */

			savefile_read_line(buf, sizeof(buf), savefile);
			continue;
		}

		/* read and verify the relation */

		curr_relation++;
		if (max_relations && curr_relation >= max_relations)
			break;

		status = nfs_read_relation(buf, fb, &tmp_relation, 1);
		if (status != 0) {

			/* save the line number of bad relations (hopefully
			   there are very very few of them) */

			fwrite(&curr_relation, (size_t)1, 
					sizeof(uint32), bad_relation_fp);
			logprintf(obj, "error %d reading relation %u\n",
					status, curr_relation);
			savefile_read_line(buf, sizeof(buf), savefile);
			continue;
		}

		/* relation is good; find the value to which it
		   hashes. Note that only the bottom 35 bits of 'a'
		   and the bottom 29 bits of 'b' figure into the hash,
		   so that spurious hash collisions are possible
		   (though highly unlikely) */

		num_relations++;
		blob[0] = (uint32)tmp_relation.a;
		blob[1] = ((tmp_relation.a >> 32) & 0x1f) |
			  (tmp_relation.b << 5);

		hashval = (HASH1(blob[0]) ^ HASH2(blob[1])) >>
			   (32 - LOG2_DUP_HASHTABLE1_SIZE);

		/* save the hash bucket if there's a collision. We
		   don't need to save any more collisions to this bucket,
		   but future duplicates could cause the same bucket to
		   be saved more than once. We can cut the number of
		   redundant bucket reports in half by resetting the
		   bit to zero */

		if (hashtable[hashval / 8] & hashmask[hashval % 8]) {
			fwrite(&hashval, (size_t)1, 
					sizeof(uint32), collision_fp);
			num_collisions++;
			hashtable[hashval / 8] &= ~hashmask[hashval % 8];
		}
		else {
			hashtable[hashval / 8] |= hashmask[hashval % 8];
		}

		/* add the factors of tmp_relation to the counts of primes */
		   
		for (i = 0; i < tmp_relation.num_factors_r +
				tmp_relation.num_factors_a; i++) {
			prime_bins[tmp_relation.factors[i] / BIN_SIZE]++;
		}

		/* get the next line */

		savefile_read_line(buf, sizeof(buf), savefile);
	}

	free(hashtable);
	savefile_close(savefile);
	fclose(bad_relation_fp);
	fclose(collision_fp);
		
	logprintf(obj, "found %u hash collisions in %u relations\n", 
				num_collisions, num_relations);

	if (num_collisions == 0) {

		/* no duplicates; no second pass is necessary */

		char buf2[256];
		sprintf(buf, "%s.hc", savefile->name);
		remove(buf);
		sprintf(buf, "%s.br", savefile->name);
		sprintf(buf2, "%s.d", savefile->name);
		if (rename(buf, buf2) != 0) {
			logprintf(obj, "error: dup1 can't rename outfile\n");
			exit(-1);
		}
	}
	else {
		purge_duplicates_pass2(obj);
	}

	/* the large prime cutoff for the rest of the filtering
	   process should be chosen here. We don't want the bound
	   to depend on an arbitrarily chosen factor base, since
	   that bound maybe too large or much too small. The former
	   would make filtering take too long, and the latter 
	   could make filtering impossible.

	   Conceptually, we want the bound to be the point below
	   which large primes appear too often in the dataset. */

	i = 1 << (32 - LOG2_BIN_SIZE);
	bin_max = (double)BIN_SIZE * i /
			log((double)BIN_SIZE * i);
	for (i--; i > 2; i--) {
		double bin_min = (double)BIN_SIZE * i /
				log((double)BIN_SIZE * i);
		double hits_per_prime = (double)prime_bins[i] /
						(bin_max - bin_min);
		if (hits_per_prime > TARGET_HITS_PER_PRIME)
			break;
		bin_max = bin_min;
	}
	free(prime_bins);
	return BIN_SIZE * (i + 0.5);
}
