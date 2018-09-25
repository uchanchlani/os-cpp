/*
 File: ContFramePool.C
 
 Author: Utkarsh Chanchlani
 Date  : 09/09/2018
 
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

// initialize the pool manager with 0 frame pools
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
 * PS I'd like to establish that whatever data private functions will take
 * will be offsetted from frame pool start point
 * (unless explicitly mentioned)
 */

// just a wrapper function so that I don't write these two lines again and again
// I hate code duplications you know
void error_msg_for_frame_pool()
{
    Console::puts("Error, unexpected behaviour identified\n");
    assert(false);
}

// helper function to set all the bits to free initially.
void set_bit_map(unsigned char * bitmap,
                 unsigned long _n_frames) {
    _n_frames = (_n_frames / 4) + (_n_frames % 4 == 0 ? 0 : 1);
    for(unsigned long i = 0; i < _n_frames; i++) {
        bitmap[i] = 0xff;
    }
}

unsigned char ContFramePool::get_first_free_frame(unsigned char bit_block,
                                                  unsigned char start_at)
{
    if(start_at > 3) {
        return 4;
    }

    /*
     * This is a bit mask to fake that the start_at number of bits
     * are not what the function requires
     */
    unsigned char bit_mask = 0xff;
    bit_mask = bit_mask >> 2 * start_at;

    // we fake that i frames are occupied even if they are not
    bit_block = bit_block & bit_mask;

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

unsigned char ContFramePool::get_first_non_follow_frame(unsigned char bit_block,
                                                        unsigned char start_at)
{
    if(start_at > 3) {
        return 4;
    }

    /*
     * This is a bit mask to fake that the start_at number of bits
     * are not what the function requires
     */
    unsigned char bit_mask = 0xff;
    bit_mask = bit_mask >> (2*start_at);

    bit_block = bit_block & bit_mask;
    // we fake that i frames are follow even if they are not

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

unsigned char ContFramePool::get_first_occupied_frame(unsigned char bit_block,
                                                      unsigned char start_at)
{
    if(start_at > 3) {
        return 4;
    }

    /*
     * This is a bit mask to fake that the start_at number of bits
     * are not what the function requires
     */
    unsigned char bit_mask = 0xff;
    bit_mask = bit_mask >> (2*start_at);

    bit_block = bit_block | ~bit_mask;
    // we fake that i frames are free even if they are not (note the not operator in occupied)

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

unsigned char ContFramePool::release_frames_in_block(unsigned char block,
                                                     unsigned char start_at /* inclusive */,
                                                     unsigned char end_at /* exclusive */)
{
    unsigned char left_free_mask = 0xff;
    unsigned char right_free_mask = 0xff;
    for(unsigned char i = 0; i < start_at; i++) {
        left_free_mask = left_free_mask >> 2;
    }
    for(unsigned char i = 4; i > end_at; i--) {
        right_free_mask = right_free_mask << 2;
    }
    unsigned char free_mask = left_free_mask & right_free_mask;

    return block | free_mask;
}

unsigned char ContFramePool::assign_frames_in_block(unsigned char block,
                                                    unsigned char start_at /* inclusive */,
                                                    unsigned char end_at /* exclusive */,
                                                    bool want_head)
{
    unsigned char left_free_mask = 0xff;
    unsigned char right_free_mask = 0xff;
    for(unsigned char i = 0; i < start_at; i++) {
        left_free_mask = left_free_mask >> 2;
    }
    for(unsigned char i = 4; i > end_at; i--) {
        right_free_mask = right_free_mask << 2;
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

unsigned long ContFramePool::check_continous_free_frames(unsigned long start_frame,
                                                         unsigned long cutoff)
{
    unsigned long end_frame_relative = end_frame_no - base_frame_no;
    unsigned long return_size = 0;

    // here we try to figure our the size of frames which are free.
    // here we require that get first occupied frame function
    // we check 4 frames at a time and stop if we find a desired frame
    // we do it optimally using bit operations
    while (return_size < cutoff && start_frame < end_frame_relative) {
        unsigned char first_occupied = get_first_occupied_frame(bitmap[start_frame / 4], start_frame % 4);
        return_size += ((unsigned long) (first_occupied - (start_frame % 4)));
        if(first_occupied < 4) {
            break;
        }
        start_frame += ((unsigned long) (4 - (start_frame % 4)));
    }

    return return_size;
}

void ContFramePool::release_pool_frames(unsigned long start_frame)
{
    unsigned long end_frame_relative = end_frame_no - base_frame_no;
    unsigned long start_frame_copy = start_frame;
    unsigned long size = 1;
    start_frame++; // we know the first frame is head, we note it in size and proceed

    // here we try to figure out the size of the frames assigned at that time
    // here we require that get first non follow frame function.
    // the way do it is we check 4 frames at a time using bit operators (for optimizations)
    // the while loop checks 4 frames at a time
    // we break if we reach the end of the frame pool or we get the first free frame
    while(true) {
        if(start_frame >= end_frame_relative) {
            break;
        }

        unsigned char curr_size = get_first_non_follow_frame(bitmap[start_frame / 4], start_frame % 4);
        size += ((unsigned long) (curr_size - (start_frame % 4)));
        start_frame += ((unsigned long) (curr_size - (start_frame % 4)));
        if(curr_size < 4) {
            break;
        }
    }

    start_frame = start_frame_copy;

    // now we free the frames 4 at a time according to the size
    // here we use the release frames in block method to do the bit operattons for freeing the frames
    while (size > 0) {
        bitmap[start_frame / 4] = release_frames_in_block(bitmap[start_frame / 4], start_frame % 4, (size < 4 ? size : 4));
        free_frames += (size < 4 ? size : 4);
        size -= (size < 4 ? size : 4);
        start_frame += ((unsigned long) (4 - (start_frame % 4)));
    }
}

void ContFramePool::assign_frames(unsigned long start_frame,
                                  unsigned long size)
{
    // to assign frames first frame must be marked head frame that's what we are trying to do by keeping want_head = 1
    // PS want_head becomes 0 after the first block allotment
    // here we use the assign frames in block function for the bit operations
    bool want_head = true;
    while (size > 0) {
        unsigned char curr_frame_count = 4 - (start_frame % 4);
        bitmap[start_frame / 4] = assign_frames_in_block(bitmap[start_frame / 4], start_frame % 4, (start_frame %4) + (size < curr_frame_count ? size : curr_frame_count), want_head);
        want_head = false;
        free_frames -= (size < curr_frame_count ? size : curr_frame_count);
        size -= (size < curr_frame_count ? size : curr_frame_count);
        start_frame += curr_frame_count;
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
    base_frame_no = _base_frame_no;
    end_frame_no = _base_frame_no + _n_frames;
    free_frames = _n_frames;

    // if the info frame no is 0 we have to allocate some of the current pool's frame to the info frames
    if(_info_frame_no == 0) {
        info_frame_no = base_frame_no;

        // initialize the bitmap pointer to the start of the frames and set the required no of bits to free
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
        set_bit_map(bitmap, _n_frames);

        // now mark the number of frames used in bitmap to allocated
        assign_frames(0, needed_info_frames(_n_frames));
    } else {
        // if we are given some frames for bitmap storage and it is less than our demand we will panic
        if(needed_info_frames(_n_frames) > _n_info_frames) {
            error_msg_for_frame_pool();
            return;
        }

        info_frame_no = _info_frame_no;

        // initialize the bitmap pointer to the start of the allocated frames and set the required no of bits to free
        bitmap = (unsigned char *) (_info_frame_no * FRAME_SIZE);
        set_bit_map(bitmap, _n_frames);
    }

    // add the current pool to our pool manager linked list

    add_new_pool(this);
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    if(_n_frames > free_frames) {
        return 0;
    }
    unsigned long rem_free_frames = free_frames; // stores currently available free frames.
                                                 // we break if the at any time this number falls below _n_frames
    unsigned long allocated_frame = 0;           // we start with trying to allocate first frame we increment this number as our sliding window increases
    unsigned long total_size = end_frame_no - base_frame_no; // end frame number relative to the start frame number

    // this function simply skips the allocated blocks and checks the size of the free block
    // if the size of the free block is greater than _n_frames we allocate it else we proceed further
    for(; allocated_frame < total_size; /* we'll increment the frame no inside as needed */) {

        unsigned char first_free = get_first_free_frame(bitmap[allocated_frame / 4], allocated_frame % 4);
        allocated_frame += ((unsigned long) first_free);

        // if we get the first free frame we check for the continuos block of free frames
        if(first_free < 4) {
            unsigned long curr_free_size = check_continous_free_frames(allocated_frame, _n_frames);

            // if got well and good, break
            if(curr_free_size >= _n_frames) {
                break;
            } else { // else proceed
                allocated_frame += curr_free_size;
                rem_free_frames -= curr_free_size;
            }
        }

        // if not enough frames available break
        if(rem_free_frames < _n_frames) {
            allocated_frame = end_frame_no;
        }
    }

    // if allocated frames not within the range return 0
    if(allocated_frame >= end_frame_no) {
        return 0;
    }

    // assign frame and return
    assign_frames(allocated_frame, _n_frames);
    return base_frame_no + allocated_frame;
}

/*
 * mark inaccessible basically assigns the frames
 */
void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    assign_frames(_base_frame_no - base_frame_no, _n_frames);
}

/*
 * checks in the linked list pointed by static pointer to pool manager
 * tries to find out which frame pool manages the frame and calls it to release
 * the frame.
 */
void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    if(head_fp == NULL) {
        error_msg_for_frame_pool();
        return;
    }
    ContFramePool * curr_pool = head_fp->get_pool_for_frame(_first_frame_no);
    if (curr_pool == NULL) {
        error_msg_for_frame_pool();
        return;
    }
    curr_pool->release_pool_frames(_first_frame_no - curr_pool->base_frame_no);
}

/*
 * divide by 4 because 1 byte manages 4 bits
 * then divide by frame size because we have total frame_size bytes in a frame
 */
unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    _n_frames = (_n_frames / 4) + (_n_frames % 4 == 0 ? 0 : 1);
    _n_frames = (_n_frames / FRAME_SIZE) + (_n_frames % FRAME_SIZE == 0 ? 0 : 1);
    return _n_frames;
}

ContFramePool * ContFramePool::get_pool_for_frame(unsigned long _curr_frame)
{
    if (_curr_frame >= base_frame_no && _curr_frame < end_frame_no) {
        return this;
    } else if (next != NULL) {
        return next->get_pool_for_frame(_curr_frame);
    } else {
        return NULL;
    }
}

void ContFramePool::add_new_pool(ContFramePool * pool)
{
    if(head_fp == NULL) {
        head_fp = pool;
    }
    ContFramePool * last_pool = head_fp;
    while(last_pool->next != NULL) {
        last_pool = last_pool->next;
    }
    last_pool->next = pool;
}

