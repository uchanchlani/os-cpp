/*
 File: vm_pool.C
 
 Author:
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
void error_msg(const char * msg = "Error, unexpected behaviour identified\n")
{
    Console::puts(msg);
    assert(false);
}

unsigned long VMPool::calculate_hash(unsigned long _size)
{
    _hash_seed = _hash_seed * _hash_seed + (_hash_seed >> 1); // with only first arg, we may get only negs or positives
    return _hash_seed % _size;
}

bool is_overlap(unsigned long start1, unsigned long end1, unsigned long start2, unsigned long end2)
{
    // todo complete this function and good night
}

bool VMPool::check_feasible_assgn(unsigned long _start_page, unsigned long _num_pages)
{
    unsigned long assgns_left = total_assgns;
    unsigned int size = PageTable::PAGE_SIZE / sizeof(unsigned long) / 2;
    for(unsigned int i = 0; i < size; ++i) {
        if(assgns_left <= 0) {
            return true;
        }
        if(assigned_frames[2*i] == 0) {
            continue;
        }
        unsigned long start = assigned_frames[2*1], end = assigned_frames[2*i + 1];
        // todo: check this code line
        if((start >= _start_page && start < (_start_page + _num_pages)) ||
           (end > _start_page && end <= (_start_page + _num_pages))) {
            return false;
        }
        assgns_left--;
    }
    return true;
}

void VMPool::assign_pages(unsigned long _start_page, unsigned long _num_pages)
{
    unsigned int size = PageTable::PAGE_SIZE / sizeof(unsigned long) / 2;
    for(unsigned int i = 0; i < size; ++i) {
        if(assigned_frames[2*i] == 0) {
            assigned_frames[2*i] = _start_page;
            assigned_frames[2*i + 1] = _start_page + _num_pages;
            total_assgns++;
        }
    }
    error_msg("No space left in the vm pool manager table. Will panic as of now");
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
    assigned_frames[0] = start_page;
    assigned_frames[1] = 1;
    total_assgns++;
     
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    unsigned short leftover = (_size % PageTable::PAGE_SIZE > 0) ? 1 : 0;
    _size = (_size >> PageTable::FRAME_OFFSET) + leftover;
    unsigned long start = 0;
    for(unsigned long i = 0; i < 5; ++i) { // let's calculate the hash only for 5 times then panic. We'll figure out if my algo is shit
        start = start_page + calculate_hash(num_pages - _size);
        if(check_feasible_assgn(start, _size))
            break;
        start = 0;
    }
    if(start == 0) {
        error_msg("Hashing function is not so good dude");
        return 0;
    }
    assign_pages(start, _size);
    return start << PageTable::FRAME_OFFSET;
}

void VMPool::release(unsigned long _start_address) {
    unsigned int size = PageTable::PAGE_SIZE / sizeof(unsigned long);
    for(unsigned int i = 0; i < size; i += 2) {
        if(assigned_frames[i] == _start_address >> PageTable::FRAME_OFFSET) {
            for(unsigned long page = assigned_frames[i]; page < assigned_frames[i+1]; ++page) {
                page_table->free_page(page);
            }
            return;
        }
    }
    error_msg("Panic as the release request is not legitimate. The code block should never reach here");
}

bool VMPool::is_legitimate(unsigned long _address) {
    return !check_feasible_assgn(_address >> PageTable::FRAME_OFFSET, 1);
}

