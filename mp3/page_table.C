#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;
const unsigned short PageTable::FRAME_OFFSET = calculate_offset(PAGE_SIZE);
const unsigned short PageTable::ENTRIES_OFFSET = calculate_offset(PAGE_SIZE);
const unsigned long ZERO = 0;
const unsigned long PageTable::FRAME_MASK = ((!ZERO) << calculate_offset(PAGE_SIZE));

// just a wrapper function so that I don't write these two lines again and again
// I hate code duplications you know
void error_msg()
{
    Console::puts("Error, unexpected behaviour identified\n");
    assert(false);
}

void error_msg(const char * msg)
{
    Console::puts(msg);
    assert(false);
}

bool PageTable::is_valid_entry(unsigned long page_entry)
{
    return (page_entry & 1);
}

void PageTable::direct_map_memory(unsigned long l_addr_start, unsigned long l_addr_end) // both should be aligned with frame size
{
    l_addr_start = l_addr_start & FRAME_MASK;
    l_addr_end = l_addr_end & FRAME_MASK;

    unsigned long * curr_pd_entry = get_pd_entry(l_addr_start);
    unsigned long pd_entry = l_addr_start >> (FRAME_OFFSET + ENTRIES_OFFSET);
    for(unsigned long l_addr = l_addr_start; l_addr < l_addr_end; l_addr += PAGE_SIZE) {
        if((l_addr >> (FRAME_OFFSET + ENTRIES_OFFSET)) != pd_entry) {
            pd_entry = l_addr >> (FRAME_OFFSET + ENTRIES_OFFSET);
            curr_pd_entry = get_pd_entry(l_addr);
        }
        set_page_entry(curr_pd_entry, l_addr, l_addr);
    }
}

unsigned long * PageTable::get_pd_entry(unsigned long l_addr) 
{
    unsigned long entry_number = l_addr >> (FRAME_OFFSET + ENTRIES_OFFSET);
    if(!is_valid_entry(page_directory[entry_number])) {
        unsigned long page_addr = get_new_frame();
        init_page_table_entries(&page_addr);
        add_frame_to_entry(page_directory, entry_number, page_addr);
    }
    return (unsigned long *) (page_directory[entry_number] & FRAME_MASK);
}

unsigned long PageTable::get_page_entry(unsigned long * page_table, unsigned long l_addr) 
{
    unsigned long entry_number = (l_addr << ENTRIES_OFFSET) >> (FRAME_OFFSET + ENTRIES_OFFSET);
    if(!is_valid_entry(page_table[entry_number])) {
        unsigned long page_addr = get_new_frame();
        add_frame_to_entry(page_table, entry_number, page_addr);
    }
    return (page_table[entry_number] & FRAME_MASK);
}

void PageTable::set_page_entry(unsigned long * page_table, unsigned long l_addr, unsigned long p_addr) 
{
    unsigned long entry_number = (l_addr << ENTRIES_OFFSET) >> (FRAME_OFFSET + ENTRIES_OFFSET);
    if(!is_valid_entry(page_table[entry_number])) {
        add_frame_to_entry(page_table, entry_number, p_addr);
    } else { // I can release the frame back to the frame pool as well
             // , but where did the frame came from. I'll choose to panic as of now till it is not clear
        error_msg();
    }
}

void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = _shared_size;
    Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
    page_directory = (unsigned long *) get_new_frame();

    init_page_table_entries(page_directory);

    direct_map_memory(0, shared_size);

    Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
    current_page_table = this;
    write_cr3(*page_directory);
    Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
    write_cr0(paging_enabled);
    Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
    unsigned long faulty_l_addr = read_cr2();
    current_page_table->get_page_entry(
            current_page_table->get_pd_entry(faulty_l_addr),
            faulty_l_addr);
    Console::puts("handled page fault\n");
}

void PageTable::add_frame_to_entry(unsigned long *page_table, unsigned long entry_number, unsigned long frame_addr) {
    page_table[entry_number] = (frame_addr & FRAME_MASK) | 0x07;
}

void PageTable::init_page_table_entries(unsigned long * page_table) {
    // initialize all the page table pages to invalid
    for(int i = 0; i < ENTRIES_PER_PAGE; i++) {
        page_table[i] = 0x00; // main agenda is to set the last bit which is the invalid bit as 0.
                              // I don't really care about the other bits as of now
    }
}

unsigned long PageTable::get_new_frame() {
    unsigned long frame_no = kernel_mem_pool->get_frames(1);
    if(frame_no == 0) {
        error_msg("Kernel out of frames Not a good sign\n");
    }
    return frame_no * PAGE_SIZE;
}

