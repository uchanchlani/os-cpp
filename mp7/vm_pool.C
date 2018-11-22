/*
 File: vm_pool.C
 
 Author: Utkarsh Chanchlani
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

// just a wrapper function so that I don't write these two lines again and again
// I hate code duplications you know
void _error_msg(const char * msg = "Error, unexpected behaviour identified\n")
{
    Console::puts(msg);
    assert(false);
}

// The idea is to use a hashing function but as we don't have standard libraries to use,
// I wrote a short and sweet hash number generator. It does the job as of now
// But obviously this is very biased, will produce repeated values, is not guaranteed to generate
// values with equal probabilities and what not
// ........BUT.......
// this is perfect for the demonstration purposes
unsigned long VMPool::calculate_hash(unsigned long _size)
{
    // with only first arg, we may get only odds or evens
    _hash_seed = _hash_seed * _hash_seed + (_hash_seed >> 1);
    return _hash_seed % _size;
}

template <class T>
void _swap(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

// For 2 ranges check if they overlap
bool is_overlap(unsigned long start1, unsigned long end1, unsigned long start2, unsigned long end2)
{
    if(start2 < start1) {
        _swap(start1, start2);
        _swap(end1, end2);
    }
    return end1 > start2;
}

bool VMPool::check_feasible_assgn(unsigned long _start_page, unsigned long _num_pages)
{
    unsigned long assgns_left = total_assgns;
    unsigned int size = PageTable::PAGE_SIZE / sizeof(unsigned long) / 2;
    for(unsigned int i = 0; i < size; ++i) { // for every allotments check if it is overlapping with the current candidate allotment
        if(assgns_left <= 0) {
            return true;
        }
        if(assigned_frames[2*i] == FREE_SPACE) {
            continue;
        }
        unsigned long start = assigned_frames[2*1], end = assigned_frames[2*i + 1];
        if(is_overlap(start, end, _start_page, _start_page + _num_pages)) { // if there is an overlap we return false because this candidate is invalid now
            return false;
        }
        assgns_left--;
    }
    return true;
}

// Simply add the entry in our assigned_frames array
void VMPool::assign_pages(unsigned long _start_page, unsigned long _num_pages)
{
    unsigned int size = PageTable::PAGE_SIZE / sizeof(unsigned long) / 2;
    for(unsigned int i = 0; i < size; ++i) {
        if(assigned_frames[2*i] == FREE_SPACE) {
            assigned_frames[2*i] = _start_page;
            assigned_frames[2*i + 1] = _start_page + _num_pages;
            total_assgns++;
            return;
        }
    }
    _error_msg("No space left in the vm pool manager table. Will panic as of now\n");
}

// Initialize with all 0s
void VMPool::init_pool(unsigned long *bitmap, unsigned int size)
{
    for(unsigned int i = 0; i < size; ++i) {
        bitmap[2*i] = FREE_SPACE;
        bitmap[2*i+1] = FREE_SPACE;
    }
}

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    frame_pool = _frame_pool;
    start_page = _base_address >> PageTable::FRAME_OFFSET;
    num_pages = _size >> PageTable::FRAME_OFFSET;
    page_table = _page_table;
    page_table->register_pool(this);
    total_assgns = 0;

    _hash_seed = 2147483647; // some prime number
    assigned_frames = (unsigned long *)(start_page << PageTable::FRAME_OFFSET);
    init_pool(assigned_frames, PageTable::PAGE_SIZE / sizeof(unsigned long) / 2);
    assigned_frames[0] = start_page;
    assigned_frames[1] = start_page + 1;
    total_assgns++;
     
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    unsigned short leftover = (_size % PageTable::PAGE_SIZE > 0) ? 1 : 0;
    _size = (_size >> PageTable::FRAME_OFFSET) + leftover;
    unsigned long start = 0;
    for(unsigned long i = 0; i < 5; ++i) { // let's calculate the hash only for 5 times then panic. We'll figure out if my algo is shit
        start = start_page + calculate_hash(num_pages - _size);
        if(check_feasible_assgn(start, _size)) // If feasible, we proceed to allocate the candidate in our allocation table
            break;
        start = 0;
        Console::puts("WARN -> Iter (");
        Console::puti((unsigned int)i);
        Console::puts("): Your hash function returned same value\n");
    }
    if(start == 0) { // if the hashing function fails for 5 times continuously, I panic for now and maybe change the algorithm.
        _error_msg("Hashing function is not so good dude\n");
        return 0;
    }
    assign_pages(start, _size);
    return start << PageTable::FRAME_OFFSET;
}

// For every page in the allotment, release all the pages using the page table
void VMPool::release(unsigned long _start_address) {
    unsigned int size = PageTable::PAGE_SIZE / sizeof(unsigned long) / 2;
    for(unsigned int i = 0; i < size; ++i) {
        if(assigned_frames[2*i] == _start_address >> PageTable::FRAME_OFFSET) {
            for(unsigned long page = assigned_frames[2*i]; page < assigned_frames[2*i+1]; ++page) {
                page_table->free_page(page);
            }
            assigned_frames[2*i] = FREE_SPACE;
            assigned_frames[2*i+1] = FREE_SPACE;
            return;
        }
    }
    _error_msg("Panic as the release request is not legitimate. The code block should never reach here\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    if((_address >> PageTable::FRAME_OFFSET) == start_page) return true; // special case of first page of vm
    return !check_feasible_assgn(_address >> PageTable::FRAME_OFFSET, 1);
}

