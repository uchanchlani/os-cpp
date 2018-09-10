/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

PoolManager * ContFramePool::pool_manager = NULL;

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

/*
 * Private methods
 */
void error_msg_for_release_frames()
{
    Console::puts("Error, unexpected behaviour identified\n");
    assert(false);
}

void ContFramePool::release_pool_frames(unsigned long start_frame)
{
    unsigned long start_frame_copy = start_frame;
    start_frame = start_frame - base_frame_no;
    if(!is_head_frame(get_state_for_frame(start_frame))) {
        error_msg_for_release_frames();
        return;
    }
    unsigned long size = 1;
    start_frame++;
    while(true) {
        unsigned short curr_size = get_first_non_follow_frame(bitmap[start_frame / 4], start_frame % 4);
        size += ((unsigned long) (curr_size - (start_frame % 4)));
        start_frame += ((unsigned long) (curr_size - (start_frame % 4)));
        if(curr_size < 4) {
            break;
        }
    }
    release_pool_frames(start_frame_copy, size);
}

void ContFramePool::release_pool_frames(unsigned long start_frame, unsigned long size)
{
    start_frame = start_frame - base_frame_no;
    while (size > 0) {
        bitmap[start_frame / 4] = release_frames_in_block(bitmap[start_frame / 4], start_frame % 4, (size < 4 ? size : 4));
        size -= 4;
        start_frame += ((unsigned long) (4 - (start_frame % 4)));
    }
}

unsigned char ContFramePool::get_state_for_frame(unsigned long offset_frame_no)
{
    unsigned long item_no = offset_frame_no / 4; // 4 because one char will store info for 4 frames
    unsigned short int displacement_in_byte = 3 - (offset_frame_no % 4); // one char represents these 4 frames (00112233)
    unsigned char the_byte = bitmap[item_no];
    unsigned char value_of_frame = the_byte >> (displacement_in_byte*2);
    value_of_frame == value_of_frame & 0x03; // shutdown bits other than the first two bits (for comparator)
    return value_of_frame;
}

bool ContFramePool::is_free_frame(unsigned char frame_state)
{
    return frame_state == FREE_FRAME;
}

bool ContFramePool::is_head_frame(unsigned char frame_state)
{
    return frame_state == HEAD_FRAME;
}

bool ContFramePool::is_follow_frame(unsigned char frame_state)
{
    return frame_state == FOLLOW_FRAME;
}

unsigned long check_continous_free_frames(unsigned long start_frame, unsigned long cutoff)
{
    start_frame = start_frame - base_frame_no;
    unsigned long return_size = 0;
    while (return_size < cutoff) {
        unsigned short first_occupied = get_first_occupied_frame(bitmap[start_frame / 4], start_frame % 4);
        return_size += ((unsigned long) (first_occupied - (start_frame % 4)));
        if(return_size < 4) {
            break;
        }
        start_frame += ((unsigned long) (4 - (start_frame % 4)));
    }
    return return_size;
}

unsigned short get_first_free_frame(unsigned char bit_block, unsigned short start_at)
{
    if(start_at > 3) {
        return 4;
        // I can do start at = 4, ussentially building the
        // same functionality, but this
        // is a bit more optimized (thinking for machine)
    }
    unsigned char occupied_mask = 0xff; // start with no occupied mask
    for(unsigned short i = 0; i < start_at; i++) {
        occupied_mask >> 2; // add occupied to ith frame
    }
    bit_block = bit_block & occupied_mask; // we fake that i frames are occupied even if they are not

    // now try getting the first free frame
    bit_block = bit_block & 0xaa; // unsetting bits which doesn't represent free bits 10101010
    if(bit_block == 0x00) { // no free frame as all frames are occupied, we return 4
        return 4;
    }
    // now we check all bits sequentially to see which is free
    if((bit_block & 0xc0) != 0x00) { // 11,00,00,00 unset 2,3,4 (PS: can do 10,00,00,00 as well, but this is more readable)
                                     // compare with 10,00,00,00 (representing first frame free)
        return 0;
    }
    if((bit_block & 0x30) != 0x00) { // 00,11,00,00 unset 1,3,4
                                     // compare with 00,10,00,00 (representing second frame free)
        return 1;
    }
    if((bit_block & 0x0c) != 0x00) { // 00,00,11,00 unset 1,2,4
                                     // compare with 00,00,10,00 (representing third frame free)
        return 2;
    }
    if((bit_block & 0x03) != 0x00) { // 00,00,00,11 unset 1,2,3
                                     // compare with 00,00,00,10 (representing forth frame free)
        return 3;
    }
    return 4; // should never come here ideally.
}

unsigned short get_first_non_follow_frame(unsigned char bit_block, unsigned short start_at)
{
    if(start_at > 3) {
        return 4;
        // I can do start at = 4, ussentially building the
        // same functionality, but this
        // is a bit more optimized (thinking for machine)
    }
    unsigned char occupied_mask = 0xff; // start with no occupied mask
    for(unsigned short i = 0; i < start_at; i++) {
        occupied_mask >> 2; // add occupied to ith frame
    }
    bit_block = bit_block & occupied_mask; // we fake that i frames are follow even if they are not

    // now try getting the first non follow frame
    if(bit_block == 0x00) { // no free frame as all frames are occupied, we return 4
        return 4;
    }
    // now we check all bits sequentially to see which is non follow
    if((bit_block & 0xc0) != 0x00) { // 11,00,00,00 unset 2,3,4
        return 0;
    }
    if((bit_block & 0x30) != 0x00) { // 00,11,00,00 unset 1,3,4
        return 1;
    }
    if((bit_block & 0x0c) != 0x00) { // 00,00,11,00 unset 1,2,4
        return 2;
    }
    if((bit_block & 0x03) != 0x00) { // 00,00,00,11 unset 1,2,3
        return 3;
    }
    return 4; // should never come here ideally.
}

unsigned short get_first_occupied_frame(unsigned char bit_block, unsigned short start_at)
{
    if(start_at > 3) {
        return 4;
        // I can do start at = 4, ussentially building the
        // same functionality, but this
        // is a bit more optimized (thinking for machine)
    }
    unsigned char occupied_mask = 0xff; // start with no occupied mask
    for(unsigned short i = 0; i < start_at; i++) {
        occupied_mask >> 2; // add occupied to ith frame
    }
    bit_block = bit_block | ~occupied_mask; // we fake that i frames are free even if they are not (note the not operator in occupied)

    bit_block = bit_block & 0xaa; // unsetting bits which doesn't represent free bits 10101010
    if(bit_block == 0xaa) { // all frames free
        return 4;
    }
    // now we check all bits sequentially to see which is first occupied
    if((bit_block & 0xc0) == 0x00) { // 11,00,00,00 unset 2,3,4
        return 0;
    }
    if((bit_block & 0x30) == 0x00) { // 00,11,00,00 unset 1,3,4
        return 1;
    }
    if((bit_block & 0x0c) == 0x00) { // 00,00,11,00 unset 1,2,4
        return 2;
    }
    if((bit_block & 0x03) == 0x0) { // 00,00,00,11 unset 1,2,3
        return 3;
    }
    return 4; // should never come here ideally
}

unsigned char release_frames_in_block(unsigned char block, unsigned short start_at /* inclusive */, unsigned short end_at /* exclusive */)
{
    unsigned char left_free_mask = 0xff;
    unsigned char right_free_mask = 0xff;
    for(unsigned short i = 0; i < start_at; i++) {
        left_free_mask >> 2;
    }
    for(unsigned short i = 4; i > end_at; i--) {
        right_free_mask << 2;
    }
    unsigned char free_mask = left_free_mask & right_free_mask;

    return block | free_mask;
}

unsigned char assign_frames_in_block(unsigned char block, unsigned short start_at /* inclusive */, unsigned short end_at /* exclusive */, bool want_head)
{
    unsigned char left_free_mask = 0xff;
    unsigned char right_free_mask = 0xff;
    for(unsigned short i = 0; i < start_at; i++) {
        left_free_mask >> 2;
    }
    for(unsigned short i = 4; i > end_at; i--) {
        right_free_mask << 2;
    }
    unsigned char free_mask = left_free_mask & right_free_mask;

    block = block & ~free_mask; // fill required frames

    if(want_head) {
        switch (start_at) {
        case 0:
            block = block | 0x40;
            break;
        case 1:
            block = block | 0x10;
            break;
        case 2:
            block = block | 0x04;
            break;
        case 3:
            block = block | 0x01;
            break;
        default:
            break;
        }
    }
    return block;
}

void assign_frames(unsigned long start_frame, unsigned long size)
{
    start_frame = start_frame - base_frame_no;
    bool want_head = true;
    while (size > 0) {
        bitmap[start_frame / 4] = assign_frames_in_block(bitmap[start_frame / 4], start_frame % 4, (size < 4 ? size : 4), want_head);
        want_head = false;
        size -= 4;
        start_frame += ((unsigned long) (4 - (start_frame % 4)));
    }
}

/*
 * Public methods
 */

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    //assert(false);
    
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    if(_n_frames > free_frames) {
        return 0;
    }
    unsigned long rem_free_frames = free_frames;
    for(unsigned long allocated_frame = base_frame_no; allocated_frame < end_frame_no; /* we'll increment the frame no inside as needed */) {

        if(is_free_frame(get_state_for_frame(allocated_frame))) {
            unsigned long available_frames = check_continous_free_frames(allocated_frame, _n_frames);
            if(available_continous_frames == _n_frames) {
                block_frames(allocated_frame, _n_frames);
                return allocated_frame;
            } else {
                allocated_frame += available_continous_frames;
                continue;
            }
        } else {
            allocated_frame++;
            continue;
        }
    }
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    block_frames(_base_frame_no, _n_frames);
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    if(pool_manager == NULL) {
        error_msg_for_release_frames();
        return;
    }
    ContFramePool * curr_pool = pool_manager->get_pool_for_frames(_first_frame_no);
    if (curr_pool == NULL) {
        error_msg_for_release_frames();
        return;
    }
    curr_pool->release_pool_frames(_first_frame_no);
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    assert(false);
}

PoolManager::PoolManager(unsigned long _base_frame,
                         unsigned long _n_frames,
                         ContFramePool * _curr_pool)
{
    base_frame = _base_frame;
    n_frame = _n_frame;
    curr_pool = _curr_pool;
    next = NULL;
}

ContFramePool * PoolManager::get_pool_for_frame(unsigned long _curr_frame)
{
    if (_curr_frame >= base_frame && _curr_frame < (base_frame + n_frames)) {
        return curr_pool;
    } else if (next != NULL) {
        return next->get_pool_for_frame(_curr_frame);
    } else {
        return NULL;
    }
}

void PoolManager::add_new_pool(PoolManager * pool_manager)
{
    PoolManager * last_pool_manager = this;
    while(last_pool_manager->next != NULL) {
        last_pool_manager = last_pool_manager->next;
    }
    last_pool_manager->next = pool_manager;
}

