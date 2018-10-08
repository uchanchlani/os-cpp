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
const unsigned short PageTable::ENTRIES_OFFSET = calculate_offset(ENTRIES_PER_PAGE);
const unsigned long ZERO = 0;
const unsigned long PageTable::FRAME_MASK = 0xfffff000;
const PageAttributes PageAttributes::DEFAULT_USER_PAGE(true, true);
const PageAttributes PageAttributes::DEFAULT_SUPERVISOR_PAGE(true, false);
const PageAttributes PageAttributes::NOT_PRESENT_USER_PAGE = *PageAttributes(true, true).unmark_valid();
const PageAttributes PageAttributes::NOT_PRESENT_SUPERVISOR_PAGE = *PageAttributes(true, false).unmark_valid();

// just a wrapper function so that I don't write these two lines again and again
// I hate code duplications you know
void error_msg(const char * msg = "Error, unexpected behaviour identified\n")
{
    Console::puts(msg);
    assert(false);
}

bool PageTable::is_valid_entry(unsigned long page_entry)
{
    return (page_entry & 1); // checks only the last bit
}

unsigned long PageTable::get_new_frame(bool kernel)
{
    ContFramePool * currFramePool = kernel_mem_pool;                // by default get the frames from kernel
    if(!kernel) {
        currFramePool = process_mem_pool;                           // if explicitly asked get it from user memory
    }
    unsigned long frame_no = currFramePool->get_frames(1);
    if(frame_no == 0) {                                             // if the frame is invalid let's panic
        error_msg("Curr frame pool out of frames Not a good sign\n");
    }
    return frame_no * PAGE_SIZE;
}

void PageTable::add_frame_to_entry(unsigned long *page_table, unsigned long entry_number, unsigned long frame_addr, const PageAttributes &attributes)
{
    page_table[entry_number] = (frame_addr & FRAME_MASK) | attributes.get_offset_value();
}

void PageTable::set_page_entry(unsigned long * page_table, unsigned long l_addr, unsigned long p_addr, const PageAttributes &attributes)
{
    unsigned long entry_number = (l_addr << ENTRIES_OFFSET) >> (FRAME_OFFSET + ENTRIES_OFFSET);
    if(!is_valid_entry(page_table[entry_number])) {
        add_frame_to_entry(page_table, entry_number, p_addr, attributes);
    } else { // I can release the frame back to the frame pool as well
             // , but where did the frame came from is the question to ask. I guess I'll get the answer in mp4
             // I'll choose to panic as of now till it is not clear
        error_msg();
    }
}

void PageTable::init_page_table_entries(unsigned long * page_table, const PageAttributes &attributes)
{
    // initialize all the page table pages with the attributes asked for (make sure to send valid bit as 0)
    for(int i = 0; i < ENTRIES_PER_PAGE; i++) {
        page_table[i] = attributes.get_offset_value();
    }
}

unsigned long * PageTable::get_pd_entry(unsigned long l_addr)
{
    unsigned long entry_number = l_addr >> (FRAME_OFFSET + ENTRIES_OFFSET);                                 // page table entry number
    if(!is_valid_entry(page_directory[entry_number])) {                                                     // if not valid entry, we get a page from process pool and assign it to page table page
        unsigned long page_addr = get_new_frame(false);
        add_frame_to_entry(page_directory, entry_number, page_addr, PageAttributes::DEFAULT_SUPERVISOR_PAGE);// add the page table page entry in the page directory
        init_page_table_entries((unsigned long *)get_pte_virtual_addr(l_addr),
                PageAttributes::NOT_PRESENT_SUPERVISOR_PAGE);   // init all entries to SUPERVISOR READ/WRITE and INVALID
    }
    return (unsigned long *) (page_directory[entry_number] & FRAME_MASK);                                   // after optionally assigning the page table page and setting it to the page directory, we return it by adding frame mask
}

void PageTable::direct_map_memory(unsigned long l_addr_start, unsigned long l_addr_end) // both should be aligned with frame size
{
    l_addr_start = l_addr_start & FRAME_MASK;
    l_addr_end = l_addr_end & FRAME_MASK;

    unsigned long * curr_pd_entry = get_pd_entry(l_addr_start);
    unsigned long pd_entry = l_addr_start >> (FRAME_OFFSET + ENTRIES_OFFSET);
    for(unsigned long l_addr = l_addr_start; l_addr < l_addr_end; l_addr += PAGE_SIZE) {
        if((l_addr >> (FRAME_OFFSET + ENTRIES_OFFSET)) != pd_entry) { // just an optimization, Now that I think, it is useless, not optimizing anything, but I'll choose to keep it here because I don't want to retest it
            pd_entry = l_addr >> (FRAME_OFFSET + ENTRIES_OFFSET);
            curr_pd_entry = get_pd_entry(l_addr);
        }
        set_page_entry(curr_pd_entry, l_addr, l_addr, PageAttributes::DEFAULT_SUPERVISOR_PAGE);
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
    // Now we move the page_directory to the process pool as well
    page_directory = (unsigned long *) get_new_frame(false);                                     // get new frame for page directory

    init_page_table_entries(page_directory, PageAttributes::NOT_PRESENT_SUPERVISOR_PAGE);   // init all entries to invalid

    direct_map_memory(0, shared_size);                                                      // do the direct mapping of the shared space

    Console::puts("Constructed Page Table object\n");                                       // we are all set. Cheers
}


void PageTable::load()
{
    current_page_table = this;
    write_cr3((unsigned long)page_directory);
    Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
    paging_enabled=1;
    write_cr0(read_cr0() | 0x80000000);
    Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
    unsigned long faulty_l_addr = read_cr2();               // assuming the faults are as of now only the page faults, not reading the page fault err_no and simply assuming page fault and moving on.
    // Ideally I should have checked it and if not should have panicked but I think we'll anyway panic for this case in mp4
#ifdef DEBUG_MODE
    Console::puts("Page fault for address ");
    Console::puti(faulty_l_addr);
    Console::puts("\n");
#endif
    unsigned long *page_table = current_page_table->get_pd_entry(faulty_l_addr);
    unsigned long frame = get_new_frame(false);
    current_page_table->set_page_entry(
            (unsigned long *)get_pte_virtual_addr(faulty_l_addr),
            faulty_l_addr,
            frame,
            PageAttributes::DEFAULT_SUPERVISOR_PAGE);
#ifdef DEBUG_MODE
    Console::puts("Alloted frame ");
    Console::puti(frame);
    Console::puts("\n");
    Console::puts("handled page fault\n");
#endif
}

void PageTable::register_pool(VMPool * _vm_pool)
{
    assert(false);
    Console::puts("registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no) {
    assert(false);
    Console::puts("freed page\n");
}
