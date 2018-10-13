/*
    File: vm_pool.H

    Author: R. Bettati, Nikhil Gupta
            Department of Computer Science
            Texas A&M University
    Date  : 10/26/2010

    Description: Management of the Virtual Memory Pool


*/

#ifndef _VM_POOL_H_                   // include file only once
#define _VM_POOL_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define FREE_SPACE 0

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "utils.H"
#include "cont_frame_pool.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* Forward declaration of class PageTable */
/* We need this to break a circular include sequence. */
class PageTable;

/*--------------------------------------------------------------------------*/
/* V M  P o o l  */
/*--------------------------------------------------------------------------*/

class VMPool { /* Virtual Memory Pool */
private:
    ContFramePool * frame_pool;
    unsigned long start_page;
    unsigned long num_pages;
    PageTable * page_table;

    // data structure, odd placeholders store start address,
    // the corresponding even placeholders store end address (exclusive)
    unsigned long * assigned_frames;

    unsigned long total_assgns;

    // helper value for the hash function
    unsigned long _hash_seed;

    // generates a random hash value from 0 to _size (exclusive)
    unsigned long calculate_hash(unsigned long _size);

    /*
     * Check if all the candidate pages that we have selected in the hash step can be allocated.
     * This checks with all the previous allotements and looks for any overlap with the current candidate
     */
    bool check_feasible_assgn(unsigned long _start_page, unsigned long _num_pages);

    /*
     * Assign pages will simply add the page to our allotment table (how lazy this vm pool is)
     */
    void assign_pages(unsigned long _start_page, unsigned long _num_pages);

    /*
     * initialize the array of allotments with all free
     */
    void init_pool(unsigned long* bitmap, unsigned int size);

public:
    /*
     * Gives you this vm pools preferred frame pool from where we get the frames in case of page faults
     */
    ContFramePool * get_frame_pool() {
        return frame_pool;
    }

   VMPool(unsigned long  _base_address,
          unsigned long  _size,
          ContFramePool *_frame_pool,
          PageTable     *_page_table);
   /* Initializes the data structures needed for the management of this
    * virtual-memory pool.
    * _base_address is the logical start address of the pool.
    * _size is the size of the pool in bytes.
    * _frame_pool points to the frame pool that provides the virtual
    * memory pool with physical memory frames.
    * _page_table points to the page table that maps the logical memory
    * references to physical addresses. */

   unsigned long allocate(unsigned long _size);
   /* Allocates a region of _size bytes of memory from the virtual
    * memory pool. If successful, returns the virtual address of the
    * start of the allocated region of memory. If fails, returns 0. */

   void release(unsigned long _start_address);
   /* Releases a region of previously allocated memory. The region
    * is identified by its start address, which was returned when the
    * region was allocated. */

   bool is_legitimate(unsigned long _address);
   /* Returns false if the address is not valid. An address is not valid
    * if it is not part of a region that is currently allocated. */

 };

#endif
