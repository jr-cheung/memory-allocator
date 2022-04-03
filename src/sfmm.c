/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"

int heap_initialization_flag = -1;

/*
* Clears the contents of sf_block header/prev_footer
*/
void init_header(sf_block **sf_block_ptr){
    sf_header *header = &(*sf_block_ptr)->header;
    *header = 0;
}
void init_prev_footer(sf_block **sf_block_ptr){
    sf_footer *prev_footer = &(*sf_block_ptr)->prev_footer;
    *prev_footer = 0;
}

/*
* Gets block_size of sf_block header/prev_footer
*/
size_t get_header_block_size(sf_block **sf_block_ptr){
    sf_header *header = &(*sf_block_ptr)->header;
    sf_header temp = *header;
    temp = temp >> 6;
    temp = temp << 6;
    return temp;
}
size_t get_prev_footer_block_size(sf_block **sf_block_ptr){
    sf_footer *prev_footer = &(*sf_block_ptr)->prev_footer;
    sf_footer temp = *prev_footer;
    temp = temp >> 6;
    temp = temp << 6;
    return temp;
}

/*
* Sets block_size of an sf_block header/prev_footer while preserving the alloc bits
*/
void set_header_block_size(sf_block **sf_block_ptr, size_t size){
    sf_header *header = &(*sf_block_ptr)->header;
    sf_header preserved_bits = *header & 0x3;
    *header = size;
    *header = *header | preserved_bits;
}
void set_prev_footer_block_size(sf_block **sf_block_ptr, size_t size){
    sf_footer *prev_footer = &(*sf_block_ptr)->prev_footer;
    sf_footer preserved_bits = *prev_footer & 0x3;
    *prev_footer = size;
    *prev_footer = *prev_footer | preserved_bits;
}

/*
* Determines if alloc bit in header/prev_footer is set.
* Returns 0 if set, -1 otherwise.
*/
int header_alloc_bit_status(sf_block **sf_block_ptr){
    sf_header *header = &(*sf_block_ptr)->header;
    if ((*header & 0x1) == 1){
        return 0;
    }
    return -1;
}
int prev_footer_alloc_bit_status(sf_block **sf_block_ptr){
    sf_footer *prev_footer = &(*sf_block_ptr)->prev_footer;
    if ((*prev_footer & 0x1) == 1){
        return 0;
    }
    return -1;
}

/*
* Sets/clears alloc bit in header/prev_footer
*/
void set_header_alloc_bit(sf_block **sf_block_ptr){
    sf_header *header = &(*sf_block_ptr)->header;
    *header = *header | 1;
}
void set_prev_footer_alloc_bit(sf_block **sf_block_ptr){
    sf_footer *prev_footer = &(*sf_block_ptr)->prev_footer;
    *prev_footer = *prev_footer | 1;
}
void clear_header_alloc_bit(sf_block **sf_block_ptr){
    sf_header *header = &(*sf_block_ptr)->header;
    *header = *header & ~1;
}
void clear_prev_footer_alloc_bit(sf_block **sf_block_ptr){
    sf_footer *prev_footer = &(*sf_block_ptr)->prev_footer;
    *prev_footer = *prev_footer & ~1;
}

/*
* Determines if alloc bit in header/prev_footer is set.
* Returns 0 if set, -1 otherwise.
*/
int header_prev_alloc_bit_status(sf_block **sf_block_ptr){
    sf_header *header = &(*sf_block_ptr)->header;
    if ((*header & 0x2) == 2){
        return 0;
    }
    return -1;
}
int prev_footer_prev_alloc_bit_status(sf_block **sf_block_ptr){
    sf_footer *prev_footer = &(*sf_block_ptr)->prev_footer;
    if ((*prev_footer & 0x2) == 2){
        return 0;
    }
    return -1;
}

/*
* Sets/clears prev_alloc bit in header/prev_footer
*/
void set_header_prev_alloc_bit(sf_block **sf_block_ptr){
    sf_header *header = &(*sf_block_ptr)->header;
    *header = *header | 2;
}
void set_prev_footer_prev_alloc_bit(sf_block **sf_block_ptr){
    sf_footer *prev_footer = &(*sf_block_ptr)->prev_footer;
    *prev_footer = *prev_footer | 2;
}
void clear_header_prev_alloc_bit(sf_block **sf_block_ptr){
    sf_header *header = &(*sf_block_ptr)->header;
    *header = *header & ~(1 << 1);
}
void clear_prev_footer_prev_alloc_bit(sf_block **sf_block_ptr){
    sf_footer *prev_footer = &(*sf_block_ptr)->prev_footer;
    *prev_footer = *prev_footer & ~(1 << 1);
}

/*
* Initializes the prologue, with block_size = 64, prev_alloc clear, and alloc set
*/
void init_prologue(sf_block **sf_block_ptr){
    init_header(sf_block_ptr);
    set_header_block_size(sf_block_ptr, 64);
    set_header_alloc_bit(sf_block_ptr);
}

/*
* Initializes the epilogue, with block_size = 0, prev_alloc clear, alloc set
*/
void init_epilogue(sf_block **sf_block_ptr){
    init_header(sf_block_ptr);
    set_header_alloc_bit(sf_block_ptr);
}

/*
* Initializes the free list heads array, with next and prev pointing to &sf_free_list_heads[i]
*/
void init_sf_free_list_heads(){
    for (int i=0; i<NUM_FREE_LISTS; i++){
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }
}

/*
* Returns the ith Fibonacci number for i>=1
*/
int fibOf(int i){
    if (i==1){
        return 1;
    }
    if (i==2){
        return 2;
    }
    return fibOf(i-1) + fibOf(i-2);
}

/*
* Determines the index of the free list to insert a free block based on block_size
*/
int index_of_free_list(size_t size){
    int m = 64;
    for (int i=0; i<NUM_FREE_LISTS; i++){
        if (i == NUM_FREE_LISTS-1){
            break;
        }
        if (i==0 && size == m){
            return i;
        }
        if (i==1 && size == 2*m){
            return i;
        }
        if (i==2 && size == 3*m){
            return i;
        }
        if (i>2){
            if ((size > fibOf(i)*m) && (size <= fibOf(i+1)*m)){
                return i;
            }
        }
    }
    return NUM_FREE_LISTS-1;
}

/*
* Inserts a free block into the free list heads array
*/
void insert_free_block_into_sf_free_list_heads(sf_block **sf_block_ptr){
    size_t size = get_header_block_size(sf_block_ptr);
    int i = index_of_free_list(size);

    // Case: empty list
    if (sf_free_list_heads[i].body.links.next == &sf_free_list_heads[i]
      && sf_free_list_heads[i].body.links.prev == &sf_free_list_heads[i]){

        // Adjust sentinel pointers
        sf_free_list_heads[i].body.links.next = *sf_block_ptr;
        sf_free_list_heads[i].body.links.prev = *sf_block_ptr;

        // Adjust block pointers
        (*sf_block_ptr)->body.links.next = &sf_free_list_heads[i];
        (*sf_block_ptr)->body.links.prev = &sf_free_list_heads[i];
    }

    // Case: non-empty list, insert at front
    // TODO Might need to test this part
    else{
        // new_node.next = sentinel.next
        (*sf_block_ptr)->body.links.next = sf_free_list_heads[i].body.links.next;

        // new_node.prev = sentinel
        (*sf_block_ptr)->body.links.prev = &sf_free_list_heads[i];

        // sentinel.next = new_node
        sf_free_list_heads[i].body.links.next = *sf_block_ptr;

        // new_node.next.prev = new_node
        sf_block *new_node_next = (*sf_block_ptr)->body.links.next;
        new_node_next->body.links.prev = *sf_block_ptr;
    }
}

/*
* Calculates the actual block_size to allocate (header + requested size + any padding)
*/
size_t calculate_alloc_block_size(size_t requested_size){
    size_t result = requested_size + 8; // For the header

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

/*
* Search for a block in the free lists array and removes it from the list.
* This function is mainly used during coalescing.
*/
void remove_free_block_from_sf_free_list_heads(sf_block *block_to_remove){
    for (int i=0; i<NUM_FREE_LISTS; i++){
        sf_block *block_ptr = &sf_free_list_heads[i];

        // Loop through the list of free blocks
        while (&(*block_ptr->body.links.next) != &sf_free_list_heads[i]){
            block_ptr = &(*block_ptr->body.links.next);

            // If block found, remove block from free list
            if (block_ptr == block_to_remove){

                // Remove the free block from the free list
                // Case: Block is at the end of the list
                if (&(*block_ptr->body.links.next) == &sf_free_list_heads[i]){
                    // sentinel.prev = block_ptr.prev
                    sf_free_list_heads[i].body.links.prev = &(*block_ptr->body.links.prev);

                    // block_ptr.prev.next = sentinel
                    sf_block *block_ptr_prev = &(*block_ptr->body.links.prev);
                    block_ptr_prev->body.links.next = &sf_free_list_heads[i];
                }

                // Case: Block is not at the end of the list
                else{
                    // block_ptr.prev.next = block_ptr.next
                    sf_block *block_ptr_prev = &(*block_ptr->body.links.prev);
                    block_ptr_prev->body.links.next = &(*block_ptr->body.links.next);

                    // block_ptr.next.prev = block_ptr.prev
                    sf_block *block_ptr_next = &(*block_ptr->body.links.next);
                    block_ptr_next->body.links.prev = &(*block_ptr->body.links.prev);
                }

                return;
            }
        }
    }
}

/*
* Search for a block in the free lists array and removes it. Returns a pointer to the payload area
* of the block that first fits the block, NULL if no blocks found
*/
sf_block* get_free_block_from_sf_free_list_heads(size_t size){
    for (int i = index_of_free_list(size); i<NUM_FREE_LISTS; i++){
        sf_block *block_ptr = &sf_free_list_heads[i];

        // Loop through the list of free blocks
        while (&(*block_ptr->body.links.next) != &sf_free_list_heads[i]){
            block_ptr = &(*block_ptr->body.links.next);

            // If block found, remove block from free list, split block if able to (remaining size >= 64)
            if (get_header_block_size(&block_ptr) >= size){
                size_t total_size = get_header_block_size(&block_ptr);
                size_t remaining_block_size = total_size - size;

                // Remove the free block from the free list
                // Case: Block is at the end of the list
                if (&(*block_ptr->body.links.next) == &sf_free_list_heads[i]){
                    // sentinel.prev = block_ptr.prev
                    sf_free_list_heads[i].body.links.prev = &(*block_ptr->body.links.prev);

                    // block_ptr.prev.next = sentinel
                    sf_block *block_ptr_prev = &(*block_ptr->body.links.prev);
                    block_ptr_prev->body.links.next = &sf_free_list_heads[i];
                }

                // Case: Block is not at the end of the list
                else{
                    // block_ptr.prev.next = block_ptr.next
                    sf_block *block_ptr_prev = &(*block_ptr->body.links.prev);
                    block_ptr_prev->body.links.next = &(*block_ptr->body.links.next);

                    // block_ptr.next.prev = block_ptr.prev
                    sf_block *block_ptr_next = &(*block_ptr->body.links.next);
                    block_ptr_next->body.links.prev = &(*block_ptr->body.links.prev);
                }

                // Split the block if able to
                // Case: Able to split
                if (remaining_block_size >= 64){
                    char *rem_blk_ptr = (char *)block_ptr + size;
                    sf_block *remaining_block_ptr = (sf_block *)rem_blk_ptr;

                    // Alloc block header set
                    set_header_block_size(&block_ptr, size);
                    set_header_alloc_bit(&block_ptr);

                    // Free block header init
                    init_header(&remaining_block_ptr);
                    set_header_block_size(&remaining_block_ptr, remaining_block_size);
                    set_header_prev_alloc_bit(&remaining_block_ptr);

                    // Free block footer init (same as header)
                    char *next_blk_ptr = (char *)remaining_block_ptr + remaining_block_size;
                    sf_block *next_block_ptr = (sf_block *)next_blk_ptr;
                    init_prev_footer(&next_block_ptr);
                    set_prev_footer_block_size(&next_block_ptr, remaining_block_size);
                    set_prev_footer_prev_alloc_bit(&next_block_ptr);

                    // Clear prev_alloc bit of the next block
                    clear_header_prev_alloc_bit(&next_block_ptr);

                    // Add free block to the free list
                    insert_free_block_into_sf_free_list_heads(&remaining_block_ptr);
                }

                // Case: Unable to split
                else{
                    // Set alloc_bit
                    set_header_alloc_bit(&block_ptr);

                    // Set prev_alloc bit of the next block
                    char *next_blk_ptr = (char *)block_ptr + total_size;
                    sf_block *next_block_ptr = (sf_block *)next_blk_ptr;
                    set_header_prev_alloc_bit(&next_block_ptr);
                }

                // Return pointer to the payload area
                char *result = (char *)block_ptr + 8 + 8;   // prev_footer to header to payload
                return (sf_block *)result;
            }
        }
    }
    return NULL;
}

/*
* Checks if a pointer is valid to free. Returns 0 if valid, -1 otherwise.
*/
int is_valid_ptr_to_free(sf_block **block_ptr){
    // Pointer to the payload not 64 byte aligned
    // (Check for multiples of 64, where address of prologue payload area
    // represents "0", so do (&block_payload - &prologue_payload) % 64)
    char *prologue_payload_ptr = sf_mem_start();
    prologue_payload_ptr += 8*8;
    size_t prologue_payload_address = (size_t)prologue_payload_ptr;
    size_t block_payload_address = (size_t)&(*block_ptr)->body.payload;

    if ((block_payload_address - prologue_payload_address) % 64 != 0){
        return -1;
    }

    // Header of the block is before the start of the first block of heap
    // which means header_address < first_block_header_address
    size_t header_address = (size_t)&(*block_ptr)->header;

    char *first_blk_ptr = sf_mem_start();
    first_blk_ptr += (7+1+7) * 8;
    first_blk_ptr -= 8; // Back up to "prev_footer"
    sf_block *first_block_ptr = (sf_block *)first_blk_ptr;
    size_t first_block_header_address = (size_t)&(first_block_ptr->header);

    if (header_address < first_block_header_address){
        return -1;
    }

    // Footer of the block is after the end of the last block in the heap
    // which means footer address (next_block's prev_footer address) is >= epilogue_header_address
    size_t block_ptr_size = get_header_block_size(block_ptr);
    char *next_blk_ptr = (char *)*block_ptr + block_ptr_size;
    sf_block *next_block_ptr = (sf_block *)next_blk_ptr;
    size_t footer_address = (size_t)&(next_block_ptr->prev_footer);

    char *epi_ptr = sf_mem_end();
    epi_ptr -= 8;
    epi_ptr -= 8;
    sf_block *epilogue_ptr = (sf_block *)epi_ptr;
    size_t epilogue_header_address = (size_t)&(epilogue_ptr->header);
    if (footer_address >= epilogue_header_address){
        return -1;
    }

    // Alloc bit in header is 0, unset
    if (header_alloc_bit_status(block_ptr) == -1){
        return -1;
    }

    // prev_alloc bit in header is 0, unset and alloc bit in prev_header is 1, set
    if (header_prev_alloc_bit_status(block_ptr) == -1){
        size_t prev_block_size = get_prev_footer_block_size(block_ptr);
        char *prev_blk_ptr = (char *)*block_ptr - prev_block_size;
        sf_block *previous_block_ptr = (sf_block *)prev_blk_ptr;
        if (header_alloc_bit_status(&previous_block_ptr) == 0){
            return -1;
        }
    }

    return 0;
}

void *sf_malloc(size_t size) {
    if (size > 0){
        // Initialize heap if necessary
        if (heap_initialization_flag == -1){
            heap_initialization_flag = 0;
            char *page_start_ptr = sf_mem_grow();
            if (page_start_ptr == NULL){
                sf_errno = ENOMEM;
                return NULL;
            }
            // Init prologue
            page_start_ptr += 7*8;  // 7 rows unused
            page_start_ptr -= 8;    // Adjust ptr so it starts pointing at "prev_footer"
            sf_block *prologue_block = (sf_block *)page_start_ptr;  // Cast to sf_block ptr
            init_prologue(&prologue_block);

            // Init epilogue
            char *heap_end = sf_mem_end();
            heap_end -= 8;  // Go to start of epilogue
            heap_end -= 8;  // Go to "prev_footer"
            sf_block *epilogue_block = (sf_block *)heap_end;
            init_epilogue(&epilogue_block);

            // Insert remaining block into freelist
            // Initialize header and footer of the free block
            char *first_block_ptr = (char *)prologue_block + 64;    // 1+7 rows unused payload area
            sf_block *remaining_block = (sf_block *)first_block_ptr;
            size_t sz = PAGE_SZ - (16 * 8);   // 16 rows currently taken up

            init_header(&remaining_block);
            set_header_block_size(&remaining_block, sz);
            set_header_prev_alloc_bit(&remaining_block);

            init_prev_footer(&epilogue_block);
            set_prev_footer_block_size(&epilogue_block, sz);
            set_prev_footer_prev_alloc_bit(&epilogue_block);

            // Insert block into free list array
            init_sf_free_list_heads();
            insert_free_block_into_sf_free_list_heads(&remaining_block);
        }

        // Actual mallocing starts here
        size_t size_to_alloc = calculate_alloc_block_size(size);
        void *result = get_free_block_from_sf_free_list_heads(size_to_alloc);
        if (result != NULL){
            // sf_show_heap();
            return result;
        }

        // If result == NULL, need to grow the heap here
        else{
            while (result == NULL){

                // Save the old epilogue block, which will be used as header of the new block
                char *old_heap_end = sf_mem_end();
                old_heap_end -= 8;  // Go to start of epilogue
                old_heap_end -= 8;  // Go to "prev_footer"
                sf_block *old_epilogue_block = (sf_block *)old_heap_end;

                // Extend the heap by a page
                char *page_start_ptr = sf_mem_grow();
                if (page_start_ptr == NULL){
                    sf_errno = ENOMEM;
                    return NULL;
                }

                // Create new epilogue at the end of the heap
                char *new_heap_end = sf_mem_end();
                new_heap_end -= 8;  // Go to start of epilogue
                new_heap_end -= 8;  // Go to "prev_footer"
                sf_block *new_epilogue_block = (sf_block *)new_heap_end;
                init_epilogue(&new_epilogue_block);

                // Set header block_size of the new free block, without changing prev_alloc bit
                sf_block *new_block_ptr = old_epilogue_block;
                set_header_block_size(&new_block_ptr, PAGE_SZ + 8 - 8); // +Header - Epilogue
                clear_header_alloc_bit(&new_block_ptr);

                // Set footer of new free block same as header
                init_prev_footer(&new_epilogue_block);
                set_prev_footer_block_size(&new_epilogue_block, PAGE_SZ + 8 - 8);

                if (header_prev_alloc_bit_status(&new_block_ptr) == 0){
                    set_prev_footer_prev_alloc_bit(&new_epilogue_block);
                }

                // If new block's preceding block is free, coalesce (Textbook case 3)
                if (header_prev_alloc_bit_status(&new_block_ptr) == -1){
                    // Update header of previous_block and footer of new_block with combined sizes
                    size_t previous_block_size = get_prev_footer_block_size(&new_block_ptr);
                    size_t current_block_size = get_header_block_size(&new_block_ptr);
                    size_t new_size = previous_block_size + current_block_size;

                    char *prev_blk_ptr = (char *)new_block_ptr - previous_block_size;
                    sf_block *previous_block_ptr = (sf_block *)prev_blk_ptr;
                    set_header_block_size(&previous_block_ptr, new_size);

                    set_prev_footer_block_size(&new_epilogue_block, new_size);
                    if (header_prev_alloc_bit_status(&previous_block_ptr) == 0){
                        set_prev_footer_prev_alloc_bit(&new_epilogue_block);
                    }

                    // Update free list with the newly coalesced block by first removing
                    // previous_block (which is the new_block) from the list and then re-inserting
                    // it to account for the change in block_size
                    // sf_show_heap();
                    remove_free_block_from_sf_free_list_heads(previous_block_ptr);
                    // sf_show_heap();
                    insert_free_block_into_sf_free_list_heads(&previous_block_ptr);

                }
                // Loop counter
                result = get_free_block_from_sf_free_list_heads(size_to_alloc);
            }
            // sf_show_heap();
            return result;
        }
    }
    return NULL;
}

/*
* Coalesces block being freed with adjacent blocks if possible and inserts newly
* freed block into free list.
*/
void coalesce_and_insert_block_into_free_list(sf_block **block_to_free_ptr){
    size_t block_to_free_size = get_header_block_size(block_to_free_ptr);
    char *next_blk_ptr = (char *)*block_to_free_ptr + block_to_free_size;
    sf_block *next_block_ptr = (sf_block *)next_blk_ptr;

    int prev_alloc_bit_status = header_prev_alloc_bit_status(block_to_free_ptr);
    int next_alloc_bit_status = header_alloc_bit_status(&next_block_ptr);

    // Case 1: Previous and next both allocated
    if (prev_alloc_bit_status == 0 && next_alloc_bit_status == 0){
        clear_header_alloc_bit(block_to_free_ptr);

        // Set footer same as the header
        sf_footer new_footer = (sf_footer)(*block_to_free_ptr)->header;
        next_block_ptr->prev_footer = new_footer;

        clear_header_prev_alloc_bit(&next_block_ptr);

        insert_free_block_into_sf_free_list_heads(block_to_free_ptr);
    }

    // Case 2: Previous allocated, next free
    else if (prev_alloc_bit_status == 0 && next_alloc_bit_status == -1){
        clear_header_alloc_bit(block_to_free_ptr);

        size_t next_block_size = get_header_block_size(&next_block_ptr);
        size_t new_size = block_to_free_size + next_block_size;

        set_header_block_size(block_to_free_ptr, new_size);

        // Set footer same as the header
        char *new_next_blk_ptr = (char *)*block_to_free_ptr + new_size;
        sf_block *new_next_block_ptr = (sf_block *)new_next_blk_ptr;
        sf_footer new_footer = (sf_footer)(*block_to_free_ptr)->header;
        new_next_block_ptr->prev_footer = new_footer;

        remove_free_block_from_sf_free_list_heads(next_block_ptr);
        insert_free_block_into_sf_free_list_heads(block_to_free_ptr);
    }

    // Case 3: Previous free, next allocated
    else if (prev_alloc_bit_status == -1 && next_alloc_bit_status == 0){
        size_t previous_block_size = get_prev_footer_block_size(block_to_free_ptr);
        size_t new_size = previous_block_size + block_to_free_size;

        char *prev_blk_ptr = (char *)*block_to_free_ptr - previous_block_size;
        sf_block *previous_block_ptr = (sf_block *)prev_blk_ptr;

        set_header_block_size(&previous_block_ptr, new_size);

        char *new_next_blk_ptr = (char *)previous_block_ptr + new_size;
        sf_block *new_next_block_ptr = (sf_block *)new_next_blk_ptr;

        sf_footer new_footer = (sf_footer)previous_block_ptr->header;
        new_next_block_ptr->prev_footer = new_footer;

        clear_header_prev_alloc_bit(&new_next_block_ptr);

        remove_free_block_from_sf_free_list_heads(previous_block_ptr);
        insert_free_block_into_sf_free_list_heads(&previous_block_ptr);
    }

    // Case 4: Previous and next both free
    else if (prev_alloc_bit_status == -1 && next_alloc_bit_status == -1){
        size_t previous_block_size = get_prev_footer_block_size(block_to_free_ptr);
        size_t next_block_size = get_header_block_size(&next_block_ptr);
        size_t new_size = block_to_free_size + previous_block_size + next_block_size;

        char *prev_blk_ptr = (char *)*block_to_free_ptr - previous_block_size;
        sf_block *previous_block_ptr = (sf_block *)prev_blk_ptr;

        set_header_block_size(&previous_block_ptr, new_size);

        char *new_next_blk_ptr = (char *)previous_block_ptr + new_size;
        sf_block *new_next_block_ptr = (sf_block *)new_next_blk_ptr;

        sf_footer new_footer = (sf_footer)previous_block_ptr->header;
        new_next_block_ptr->prev_footer = new_footer;

        remove_free_block_from_sf_free_list_heads(next_block_ptr);
        remove_free_block_from_sf_free_list_heads(previous_block_ptr);
        insert_free_block_into_sf_free_list_heads(&previous_block_ptr);
    }
}

void sf_free(void *pp) {
    // Check if pointer is valid
    if (pp == NULL){
        abort();
    }

    // Need to adjust pointer to point back to prev_footer instead of the payload
    char *blk_ptr = pp - 8 - 8;
    sf_block *block_ptr = (sf_block *)blk_ptr;
    if (is_valid_ptr_to_free(&block_ptr) == -1){
        abort();
    }

    // sf_show_heap();
    coalesce_and_insert_block_into_free_list(&block_ptr);
    // sf_show_heap();
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    // Check if pointer is valid
    if (pp == NULL){
        abort();
        sf_errno = EINVAL;
        return NULL;
    }

    // Need to adjust pointer to point back to prev_footer instead of the payload
    char *blk_ptr = pp - 8 - 8;
    sf_block *block_ptr = (sf_block *)blk_ptr;
    if (is_valid_ptr_to_free(&block_ptr) == -1){
        abort();
        sf_errno = EINVAL;
        return NULL;
    }

    if (rsize == 0){
        sf_free(pp);
        return NULL;
    }

    size_t new_block_size = calculate_alloc_block_size(rsize);
    size_t old_block_size = get_header_block_size(&block_ptr);

    if (new_block_size > old_block_size){
        char *new_blk_ptr = sf_malloc(rsize);
        if (new_blk_ptr == NULL) return NULL;

        void *result = new_blk_ptr;

        new_blk_ptr = new_blk_ptr - 8 - 8;
        sf_block *new_block_ptr = (sf_block *)new_blk_ptr;

        char *new_payload_area = (char *)new_block_ptr + 8 + 8;
        memcpy(new_payload_area, pp, old_block_size - 8);
        sf_free(pp);
        return result;
    }

    else if (new_block_size < old_block_size){
        // Update old block's block_size with new smaller size
        set_header_block_size(&block_ptr, new_block_size);

        // Free the remaining block and coalesce
        char *new_blk_ptr = (char *)block_ptr + new_block_size;
        sf_block *new_block_ptr = (sf_block *)new_blk_ptr;

        // Init remaining block as an "allocated" block, then call coalesce_and_insert_block_into_free_list()
        // which will make it into a free block
        init_header(&new_block_ptr);
        size_t splitted_block_size = old_block_size - new_block_size;
        set_header_block_size(&new_block_ptr, splitted_block_size);
        set_header_prev_alloc_bit(&new_block_ptr);
        set_header_alloc_bit(&new_block_ptr);
        coalesce_and_insert_block_into_free_list(&new_block_ptr);
    }

    return pp;
}
