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

#define DEBUG_MODE

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
#include "vm_pool.H"
#include "paging_low.H"

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* P A G E - T A B L E  */
/*--------------------------------------------------------------------------*/

class PageAttributes {
private:
    bool valid;
    bool rw;
    bool umode;

public:
    // some frequently used statics
    static const PageAttributes DEFAULT_USER_PAGE;          // 111
    static const PageAttributes DEFAULT_SUPERVISOR_PAGE;    // 011
    static const PageAttributes NOT_PRESENT_USER_PAGE;      // 110
    static const PageAttributes NOT_PRESENT_SUPERVISOR_PAGE;// 010

    PageAttributes(bool _rw, bool _umode) : rw (_rw), umode (_umode), valid(true) {};
    PageAttributes * mark_valid() {this->valid = true; return this;}
    PageAttributes * unmark_valid() {this->valid = false; return this;}
    PageAttributes * mark_rw() {this->rw = true; return this;}
    PageAttributes * unmark_rw() {this->rw = false; return this;}
    PageAttributes * mark_umode() {this->umode = true; return this;}
    PageAttributes * unmark_umode() {this->umode = false; return this;}
    bool is_valid() const {return this->valid;}
    bool is_rw() const {return this->rw;}
    bool is_umode() const {return this->umode;}

    unsigned int get_offset_value() const {
        unsigned int offset_value = 0x00;
        if(this->is_valid()) {
            offset_value = offset_value | 0x1;
        }
        if(this->is_rw()) {
            offset_value = offset_value | 0x2;
        }
        if(this->is_umode()) {
            offset_value = offset_value | 0x4;
        }
        return offset_value;
    }
};

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
    
    /*
    * Calculates the offset bits of the given size
    * For ex: page size 4096 returns 12 and and number of page 1024 returns 10
    */
    static unsigned short calculate_offset(unsigned int page_size) {
      unsigned short ret = 0;
      while ((page_size = page_size >> 1) > 0)
          ret ++;
      return ret;
    }

    // given a page entry check if it is valid or not
    bool is_valid_entry(unsigned long page_entry);

    // get a new frame pool, either from kernel or process pool memory by default kernel pool
    static unsigned long get_new_frame(bool kernel = true);

    // given a page table (can be a page directory page as well), and the location of the page, and the actual frame address and it's attributes, update the page entry
    void add_frame_to_entry(unsigned long * page_table, unsigned long entry_number, unsigned long frame_addr, const PageAttributes &attributes);

    /*
    * given a page table page (ps don't give the page directory page please, it will mess up)
    * it sets the entry number pointed by bits 21-12 of the logical address with the actual physical address with the given attributes.
    * as of this assignment, we are only assigning a page as read write, supervisor mode valid page
    * Note the differences with the add_frame_to_entry function.
    * This function internally calls the add_frame_to_entry function for the core functionality of adding the pages
    */
    void set_page_entry(unsigned long * page_table, unsigned long l_addr, unsigned long p_addr, const PageAttributes &attributes);

    // init page table entries with the given page attributes (generally will be blank page entries)
    void init_page_table_entries(unsigned long * page_table, const PageAttributes &attributes);

    /*
    * get the page directory entry for the given logical address
    * the way it works is it gets the upper 10 bits from the address and considers it as the entry number
    * then access the page directory entry. If the directory entry is invalid, it will fetch a new page from the
    * kernel pool and assign it as a page table page for the given entry, initialize the entry with all invalids, supervisor mode read write pages
    * and return the address of the page table page
    */
    unsigned long * get_pd_entry(unsigned long l_addr);

    /*
    * A utility function to direct map logical address space to the actual physical memory.
    * Should be called only before the paging is disabled.
    * In this context we are calling it only to direct map the first 4 MB of our memory
    */
    void direct_map_memory(unsigned long l_addr_start, unsigned long l_addr_end);

    static unsigned long get_pde_virtual_addr() {
        if(!paging_enabled) {
            return (unsigned long)current_page_table->page_directory;
        }
        return (unsigned long)(1023 << (ENTRIES_OFFSET + FRAME_OFFSET) &
                1023 << FRAME_OFFSET); // 1 way of masking the last 2 bits
    }

    static unsigned long get_pte_virtual_addr(unsigned long l_addr) {
        if(!paging_enabled) {
            return l_addr;
        }
        unsigned long entry_number = l_addr >> (FRAME_OFFSET + ENTRIES_OFFSET);                                 // page table entry number
        return (1023 << (ENTRIES_OFFSET + FRAME_OFFSET) &
                (entry_number << FRAME_OFFSET));
    }

    static void flush_tlb() {
        write_cr3((unsigned long)current_page_table->page_directory);
    }

public:
    static const unsigned int PAGE_SIZE        = Machine::PAGE_SIZE;
    static const unsigned short FRAME_OFFSET; // value 12 (length of bits used for page size)
    /* in bytes */
    static const unsigned int ENTRIES_PER_PAGE = Machine::PT_ENTRIES_PER_PAGE;
    static const unsigned short ENTRIES_OFFSET; // vaule 10 (length of bits used for page entires)
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
    
    // -- NEW IN MP4
    
    void register_pool(VMPool * _vm_pool);
    /* Register a virtual memory pool with the page table. */
    
    void free_page(unsigned long _page_no);
    /* If page is valid, release frame and mark page invalid. */
    
};

#endif

