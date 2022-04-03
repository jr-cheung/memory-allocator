#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#define TEST_TIMEOUT 15

size_t get_header_block_size();
int header_alloc_bit_status();
int heap_initialization_flag;

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int index, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	while(bp != &sf_free_list_heads[i]) {
	    if(size == 0 || size == (bp->header & ~0x3f)) {
		cnt++;
		if(size != 0) {
		    cr_assert_eq(index, i, "Block %p (size %ld) is in wrong list for its size "
				 "(expected %d, was %d)",
				 (long *)(bp) + 1, bp->header & ~0x3f, index, i);
		}
	    }
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of free blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

/*
* Asserts the block_size in a header to the expected size
*/
void assert_header_block_size(sf_block **sf_block_ptr, size_t size){
    sf_header *header = &(*sf_block_ptr)->header;
    sf_header temp = *header;
    temp = temp >> 6;
    temp = temp << 6;
    cr_assert_eq(temp, size, "Block sizes are not equal (exp=%d, found=%d)", size, temp);
}

/*
* Asserts the header and footer of a free block are the same
*/
void assert_header_and_footer_same(sf_block **sf_block_ptr, size_t block_size){
	sf_block *block = *sf_block_ptr;

	sf_header header = block->header;
	size_t size = get_header_block_size(sf_block_ptr);

	sf_block *next_block = (sf_block *)((char *)block + size);
	sf_footer footer = next_block->prev_footer;

	cr_assert_eq(header, footer, "Header does not match footer");
}

/*
* Asserts the prev_alloc bit in a header
*/
void assert_prev_alloc_bit(sf_block **sf_block_ptr, size_t prev_alloc_bit){
	if (prev_alloc_bit == 0){
		cr_assert(header_alloc_bit_status(&sf_block_ptr) != 0, "prev_alloc_bit should be unset");
	}
	else{
		cr_assert(header_alloc_bit_status(&sf_block_ptr) != -1, "prev_alloc_bit should be set");
	}
}

/*
* Calculates the actual block_size (header + payload size + any padding)
*/
size_t calculate_block_size(size_t payload_size){
    size_t result = payload_size + 8; // For the header

    // Determine padding
    size_t n = result / 64; // Integer division
    size_t r = result % 64;

    if (n == 0){
        result = 64;
    }
    else{
        if (r == 0){
            result = n*64;
        }
        else{
            result = (n+1) * 64;
        }
    }
    return result;
}

Test(sfmm_basecode_suite, malloc_an_int, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz = sizeof(int);
	int *x = sf_malloc(sz);

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_free_block_count(0, 0, 1);
	assert_free_block_count(8000, 8, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

Test(sfmm_basecode_suite, malloc_four_pages, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;

	void *x = sf_malloc(32624);
	cr_assert_not_null(x, "x is NULL!");
	assert_free_block_count(0, 0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sfmm_basecode_suite, malloc_too_large, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(524288);

	cr_assert_null(x, "x is not NULL!");
	assert_free_block_count(0, 0, 1);
	assert_free_block_count(130944, 8, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sfmm_basecode_suite, free_no_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_x = 8, sz_y = 200, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);

	assert_free_block_count(0, 0, 2);
	assert_free_block_count(256, 3, 1);
	assert_free_block_count(7680, 8, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_w = 8, sz_x = 200, sz_y = 300, sz_z = 4;
	/* void *w = */ sf_malloc(sz_w);
	void *x = sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);
	sf_free(x);

	assert_free_block_count(0, 0, 2);
	assert_free_block_count(576, 5, 1);
	assert_free_block_count(7360, 8, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, freelist, .timeout = TEST_TIMEOUT) {
        size_t sz_u = 200, sz_v = 300, sz_w = 200, sz_x = 500, sz_y = 200, sz_z = 700;
	void *u = sf_malloc(sz_u);
	/* void *v = */ sf_malloc(sz_v);
	void *w = sf_malloc(sz_w);
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_free_block_count(0, 0, 4);
	assert_free_block_count(256, 3, 3);
	assert_free_block_count(5696, 8, 1);

	// First block in list should be the most recently freed block.
	int i = 3;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(bp, (char *)y - 16,
		     "Wrong first block in free list %d: (found=%p, exp=%p)",
                     i, bp, (char *)y - 16);
}

Test(sfmm_basecode_suite, realloc_larger_block, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(int), sz_y = 10, sz_x1 = sizeof(int) * 20;
	void *x = sf_malloc(sz_x);
	/* void *y = */ sf_malloc(sz_y);
	x = sf_realloc(x, sz_x1);

	cr_assert_not_null(x, "x is NULL!");
	sf_block *bp = (sf_block *)((char *)x - 16);
	cr_assert(*((long *)(bp) + 1) & 0x1, "Allocated bit is not set!");
	cr_assert((*((long *)(bp) + 1) & ~0x3f) == 128,
		  "Realloc'ed block size (%ld) not what was expected (%ld)!",
		  *((long *)(bp) + 1) & ~0x3f, 128);

	assert_free_block_count(0, 0, 2);
	assert_free_block_count(64, 0, 1);
	assert_free_block_count(7808, 8, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_splinter, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(int) * 20, sz_y = sizeof(int) * 16;
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char*)y - 16);
	cr_assert(*((long *)(bp) + 1) & 0x1, "Allocated bit is not set!");
	cr_assert((*((long *)(bp) + 1) & ~0x3f) == 128,
		  "Block size (%ld) not what was expected (%ld)!",
	          *((long *)(bp) + 1) & ~0x3f, 128);

	assert_free_block_count(0, 0, 1);
	assert_free_block_count(7936, 8, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_free_block, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(double) * 8, sz_y = sizeof(int);
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char *)y - 16);
	cr_assert(*((long *)(bp) + 1) & 0x1, "Allocated bit is not set!");
	cr_assert((*((long *)(bp) + 1) & ~0x3f) == 64,
		  "Realloc'ed block size (%ld) not what was expected (%ld)!",
		  *((long *)(bp) + 1) & ~0x3f, 64);

	assert_free_block_count(0, 0, 1);
	assert_free_block_count(8000, 8, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

Test(sfmm_student_suite, student_test_1, .timeout = TEST_TIMEOUT) {
	// Freeing and coalescing when previous allocated, next free
	int *x = sf_malloc(sizeof(int));	// 4 bytes
	int *y = sf_malloc(10 * sizeof(int));	// 40 bytes
	double *z = sf_malloc(10 * sizeof(double));	// 80 bytes

	*x = 100;

	sf_free(z);
	sf_free(y);

	sf_block *coalesced_blk_ptr = (sf_block *)((char *)y - 16);
	assert_header_and_footer_same(&coalesced_blk_ptr, calculate_block_size(120));

	assert_free_block_count(0, 0, 1);
	assert_free_block_count(8000, 8, 1);
}

Test(sfmm_student_suite, student_test_2, .timeout = TEST_TIMEOUT) {
	// Freeing and coalescing when previous free, next allocated
	int *x = sf_malloc(sizeof(int));	// 4 bytes
	int *y = sf_malloc(10 * sizeof(int));	// 40 bytes
	double *z = sf_malloc(10 * sizeof(double));	// 80 bytes

	*z = 100;

	sf_free(x);
	sf_free(y);

	sf_block *coalesced_blk_ptr = (sf_block *)((char *)x - 16);
	assert_header_and_footer_same(&coalesced_blk_ptr, calculate_block_size(44));

	sf_block *next_block_ptr = (sf_block *)((char *)x + get_header_block_size(&coalesced_blk_ptr));
	assert_prev_alloc_bit(&next_block_ptr, 0);

	assert_free_block_count(128, 1, 1);
}

Test(sfmm_student_suite, student_test_3, .timeout = TEST_TIMEOUT, .signal = SIGABRT) {
	// Freeing an invalid pointer
	int *x = sf_malloc(sizeof(int));

	int *invalid_x = x + 1;
	int *null_ptr = NULL;

	sf_free(invalid_x);
	sf_free(null_ptr);
}

Test(sfmm_student_suite, student_test_4, .timeout = TEST_TIMEOUT) {
	// Reallocing the same size malloced
	int *x = sf_malloc(100);
	*x = 987654321;

	int *y = sf_realloc(x, 100);

	cr_assert_eq(x, y, "Pointers returned are different");
	cr_assert_eq(*y, *x, "Payload has been corrupted");
}

Test(sfmm_student_suite, student_test_5, .timeout = TEST_TIMEOUT) {
	// Using a free block not at the beginning of the list, tests if block removed correctly
	double *x = sf_malloc(5000);
	int *y = sf_malloc(5000);
	int *z = sf_malloc(4521);
	double *zz = sf_malloc(2000);
	*x = 9.8;
	*z = 3.14;
	*zz = 29.92;

	sf_free(y);

	int *a = sf_malloc(7680);
	*a = 0;

	sf_block *next = sf_free_list_heads[8].body.links.next;

	cr_assert_eq((sf_free_list_heads[8].body.links.prev), next, "Sentinel prev does not point to last free block");
	cr_assert_eq(&(*next->body.links.next), &sf_free_list_heads[8], "Remaining block next does not point to sentinel");
	assert_free_block_count(5056, 8, 1);
}
