/* to be included into test_fanom.cpp and test_lucky777.cpp */
/* hashsum(str, len) should be defined */

#ifndef hashsum
/* satisfy editor */
#include <inttypes.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <map>

#define hashsum(str, len) (1)
#endif

struct entry {
	uint64_t hash;
	struct entry *next;
	uint32_t count;
	size_t len;
	char str[1];
};

struct table {
	uint32_t size;
	uint32_t bins;
	struct entry **entries;
};

void table_rehash(table *table) {
	uint32_t old_bins = table->bins;
	uint32_t i, mask, pos;
	entry **pe, *e;
	table->bins = old_bins == 0 ? 8 : old_bins * 2;
	mask = table->bins - 1;
	table->entries = (entry**)realloc(table->entries, table->bins*sizeof(entry*));
	memset(table->entries+old_bins, 0, sizeof(entry*)*(table->bins - old_bins));
	assert(table->entries != NULL);
	for (i = 0; i < old_bins; i++) {
		pe = &table->entries[i];
		while (*pe != NULL) {
			e = *pe;
			pos = e->hash & mask;
			if (pos != i) {
				*pe = e->next;
				e->next = table->entries[pos];
				table->entries[pos] = e;
			} else {
				pe = &e->next;
			}
		}
	}
}

void table_insert(table *table, const char* str, size_t len) {
	uint64_t hash;
	uint32_t pos;
	entry* en;
	/* lets fill factor to be 3x, so it will be easier to get collisions */
	if (table->size == table->bins) {
		table_rehash(table);
	}
	hash = hashsum(str, len);
	pos = (uint32_t)hash & (table->bins - 1);
	en = table->entries[pos];
	while (en != NULL) {
		if (en->hash == hash && en->len == len && memcmp(en->str, str, len) == 0) {
			/* found match */
			return;
		}
		en = en->next;
	}
	en = (entry*)malloc(sizeof(entry) + len);
	en->hash = hash;
	en->next = table->entries[pos];
	en->len = len;
	memcpy(en->str, str, len);
	en->str[len] = 0;
	table->entries[pos] = en;
	table->size++;
}

uint32_t checksum[2] = {0, 0};
#define rotl(x, n) (((x) << (n)) | ((x) >> (sizeof(x)*8 - (n))))
void checksum_add(const char* p, size_t len) {
	size_t i;
	uint64_t h0 = 0x100, h1 = 0x200;
	for (i = 0; i < len; i++) {
		h0 += (uint8_t)p[i];
		h1 += (uint8_t)p[i];
		h0 *= 0x1234567;
		h1 *= 0xabcdef9;
		h0 = rotl(h0, 13);
		h1 = rotl(h1, 19);
	}
	checksum[0] ^= h0;
	checksum[1] ^= h1;
}

static char dehex[256];
void fill_dehex() {
	int i;
	for (i=0; i<255; i++) dehex[i] = -1;
	for (i='0'; i<='9'; i++) dehex[i] = i-'0';
	for (i='a'; i<='f'; i++) dehex[i] = i-'a'+10;
	for (i='A'; i<='F'; i++) dehex[i] = i-'A'+10;
}
void dehexify(char* s, ssize_t len) {
	ssize_t i, j;
	for (i=0,j=0; i<len; i+=2, j++) {
		char c1 = s[i];
		char c2 = s[i+1];
		assert(c1 != -1 && c2 != -1);
		s[j] = (c1<<4) + c2;
	}
}

int check_table(table *tbl, int use_32) {
	uint32_t maxchain = 0;
	double expectedmaxchain = 2 * log(tbl->bins) / log(log(tbl->bins));
	std::map<uint64_t, uint32_t> counter;
	int ret = 0;
	int i;
	for (i = 0; i < tbl->bins; i++) {
		uint32_t chain = 0;
		entry* e = tbl->entries[i];
		while (e != NULL) {
			chain++;
			checksum_add(e->str, e->len);
			counter[e->hash]++;
			e = e->next;
		}
		if (chain > maxchain)
			maxchain = chain;
	}
	printf("checksum %08x %08x\n", checksum[0], checksum[1]);
	printf("maxchain: %u (expected %f)", maxchain, expectedmaxchain);
	if (maxchain > expectedmaxchain*2) {
		printf("(!!! maxchain overflow)");
		ret = 1;
	}
	printf("\n");
	if (counter.size() != tbl->size) {
		double ncoll = tbl->size - (uint32_t)counter.size();
		double expected = 0;
		uint32_t maxcnt = 0;
		if (use_32) {
			double d = pow(2,32);
			double n = tbl->size;
			/* https://en.wikipedia.org/wiki/Birthday_problem#Collision_counting */
			expected = n - d + d*pow((d-1)/d, n);
			if (expected > 0.01 && ncoll <= 2) {
				ret = 0;
			} else {
				ret = expected*4 < ncoll;
			}
		} else {
			ret = 1;
		}
		for (auto& pair: counter) {
			if (pair.second-1 > maxcnt) {
				maxcnt = pair.second-1;
			}
		}
		printf("Has full hash collision: %u same hashes for %u values (expected %f)\n",
				(uint32_t)ncoll, tbl->size, expected);
		if (maxcnt > 1) {
			printf("Max full collision: %u\n", maxcnt);
			ret = 1;
		}
	}
	return ret;
}
