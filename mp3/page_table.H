/*
    File: page_table.H

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 16/12/07

    Description: Basic Paging.

*/

#ifndef _page_table_H_                   // include file only once
#define _page_table_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H"
#include "exceptions.H"
#include "cont_frame_pool.H"

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* P A G E - T A B L E  */
/*--------------------------------------------------------------------------*/

class PageTable {

private:

  /* THESE MEMBERS ARE COMMON TO ENTIRE PAGING SUBSYSTEM */
  static PageTable     * current_page_table; /* pointer to currently loaded page table object */
  static unsigned int    paging_enabled;     /* is paging turned on (i.e. are addresses logical)? */
  static ContFramePool * kernel_mem_pool;    /* Frame pool for the kernel memory */
  static ContFramePool * process_mem_pool;   /* Frame pool for the process memory */
  static unsigned long   shared_size;        /* size of shared address space */

  /* DATA FOR CURRENT PAGE TABLE */
  unsigned long        * page_directory;     /* where is page directory located? */

  static unsigned short calculate_offset(unsigned int page_size) {
      if(page_size == 0) {
          return 0;
      }
      unsigned short ret = 1;
      while ((page_size = page_size >> 1) > 0)
          ret ++;
      return ret;
  }

  void add_frame_to_entry(unsigned long * page_table, unsigned long entry_number, unsigned long frame_addr);
  void init_page_table_entries(unsigned long * page_table);
  unsigned long get_new_frame();
  bool is_valid_entry(unsigned long page_entry);
  unsigned long * get_pd_entry(unsigned long l_addr);
  unsigned long get_page_entry(unsigned long * page_table, unsigned long l_addr);
  void set_page_entry(unsigned long * page_table, unsigned long l_addr, unsigned long p_addr);
  void direct_map_memory(unsigned long l_addr_start, unsigned long l_addr_end);

public:
  static const unsigned int PAGE_SIZE        = Machine::PAGE_SIZE;
  static const unsigned short FRAME_OFFSET;
  /* in bytes */
  static const unsigned int ENTRIES_PER_PAGE = Machine::PT_ENTRIES_PER_PAGE;
  static const unsigned short ENTRIES_OFFSET;
  /* in entries, duh! */
  static const unsigned long FRAME_MASK;

  static void init_paging(ContFramePool * _kernel_mem_pool,
                          ContFramePool * _process_mem_pool,
                          const unsigned long _shared_size);
  /* Set the global parameters for the paging subsystem. */

  PageTable();
  /* Initializes a page table with a given location for the directory and the
     page table proper.
     NOTE: The PageTable object still needs to be stored somewhere! 
     Probably it is best to have it on the stack, as there is no 
     memory manager yet...
     NOTE2: It may also be simpler to create the first page table *before* 
     paging has been enabled.
  */

  void load();
  /* Makes the given page table the current table. This must be done once during
     system startup and whenever the address space is switched (e.g. during
     process switching). */

  static void enable_paging();
  /* Enable paging on the CPU. Typically, a CPU start with paging disabled, and
     memory is accessed by addressing physical memory directly. After paging is
     enabled, memory is addressed logically. */

  static void handle_fault(REGS * _r);
  /* The page fault handler. */

};

#endif

