/*
 File: cont_frame_pool.H
 
 Author: R. Bettati
 Department of Computer Science
 Texas A&M University
 Date  : 17/02/04 
 
 Description: Management of the CONTIGUOUS Free-Frame Pool.
 
 As opposed to a non-contiguous free-frame pool, here we can allocate
 a sequence of CONTIGUOUS frames.
 
 */

#ifndef _CONT_FRAME_POOL_H_                   // include file only once
#define _CONT_FRAME_POOL_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/*
 * Idea of how I've used my 2 bits (4 states to represent free, head and follow frame
 */
#define FREE_FRAME 0x03   // bits representation 11
#define HEAD_FRAME 0x01   // bits representation 01
#define FOLLOW_FRAME 0x00 // bits representation 00

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* C o n t F r a m e   P o o l  */
/*--------------------------------------------------------------------------*/

// class pointer required in pool manager
class ContFramePool;

/*
 * PoolManager
 * Class for managing frame pools
 * stored as a linked list
 * each node of linked list contains:
 * the frame pool's start point and size, pointer to the frame (for direct access)
 * and one next pointer (for the linked list requirement
 */
class PoolManager {
private:
    unsigned long base_frame;
    unsigned long n_frames;
    ContFramePool * curr_pool;
    PoolManager * next;

public:
    /*
     * default constructor for pool manager so that I will initialize the
     * object later
     */
    PoolManager();

    /*
     * initialize the pool manager object. we store the base frame (and size) for
     * the pool we store. then we store the pointer to the pool to access
     * it from the manager
     */
    void init_pool_manager(unsigned long _base_frame,
                unsigned long _n_frames,
                ContFramePool * _curr_pool);

    /*
     * given a frame no., get which Frame Pool manages it
     * returns pointer to ContFramePool if found
     * else NULL
     */
    ContFramePool * get_pool_for_frame(unsigned long _curr_frame);

    /*
     * called from constructor of ContFramePool. Append the newly constructed frame pool
     * at the end of the linked list of the PoolManager
     */
    void add_new_pool(PoolManager * pool_manager);
};

class ContFramePool {
    
private:
    /* -- DEFINE YOUR CONT FRAME POOL DATA STRUCTURE(s) HERE. */
    unsigned long base_frame_no; // to store base frame no that it manager
    unsigned long end_frame_no;  // end frame no
    unsigned long info_frame_no; // info frame no stored. can be external from the pool or start of the pool
    unsigned long free_frames;   // current no of frames remaining in the pool. helpful in little optimizations

    /*
     * my bitmap uses 2 bits per frame. I had the choice of either using
     * 2 bit array objects (one for if it's free or not and other for
     * if it's a head frame or not. But this gives rise to complications
     * like how to store the 2 bitmaps in the memory.
     * I can assume that the no. of frames is fixed and the bitmap can be
     * stored in exactly x no of bits but I don't like the design somehow
     * as the bitmap won't be runtime extensible
     *
     * Second approach, I use 2 consecutive bits for my frame, say ab
     * if a bit is on it means the frame is free, else not
     * if a is off: if b off then head, else follow
     * any frame x (0 is the first frame belonging to the pool (not the first
     * frame of the memory) is represented by 2*x and 2*x+1 bits in bitmap
     */
    unsigned char * bitmap;

    /*
     * we use this object because we want to utilize caller's memory space
     * for any persistent storage. Hence this is a complete object and
     * not a reference to this object.
     *
     * PS: one can easily notice that the ContFramePool stores the actual
     * PoolManager object, bot the PoolManager only stores the "pointer"
     * to the actual ContFramePool class.
     */
    PoolManager curr_pool_manager; // to bring this in stack memory

    /*
     * Used as the head of the pool manager linked list and hence static
     */
    static PoolManager * pool_manager;

    // private functions block

    // these 5 functions are all performed on a single block of 4 frames and are optimized heavily

    /*
     * In the next three functions we try to find a frame from a block of 4 frames such that
     * it satisfies some condition that the function wants.
     *
     * Given a block of memory report the first desired frame in the block
     * it takes a bit block of 8 bits (4 frames) and
     * a start_at parameter which tells which frame to start from (shold be between 0 and 3 both inclusive)
     * return value is between 0 to 4. Where 0-3 represents the desired frame no.,
     * if no desired frame is found in block it returns 4 representing out of frame.
     * PS: notice that the return value will be always greater than or equal to start_at
     */

    // here we desire the first free frame
    unsigned char get_first_free_frame(unsigned char bit_block,
                                       unsigned char start_at);

    // here we desire the first frame that is not a follow frame i.e. a head or free frame
    // used to check the length of the allocated frame
    unsigned char get_first_non_follow_frame(unsigned char bit_block,
                                             unsigned char start_at);

    // get the first frame that is occupied.
    // a return value 4 means that all the checked frames are free frames
    // we use this function to jump 4 frames while finding the size of the free block
    unsigned char get_first_occupied_frame(unsigned char bit_block,
                                           unsigned char start_at);

    /*
     * The next 2 functions are the ones that do the bit manipulations for changing
     * state of the frames. i.e. from assigned to free and vice-versa
     * it takes 3 parameters:
     * the bit block we need to operate on (8 bits or 4 frames)
     * start frame position (0-3)
     * end frame position (1-4 because I've kept it exclusive as it was implementationally easier)
     * returns the new bit block which can be updated at the original address
     */

    // as the name suggests it sets the frames to free
    unsigned char release_frames_in_block(unsigned char block,
                                          unsigned char start_at /* inclusive */,
                                          unsigned char end_at /* exclusive */);

    // it assigns the frames in the block
    // note that we can have 2 conditions in a bit block
    // 1 we are starting to assign the frames from this block onwards
    // 2 this is a continuation block of the frame assignment
    // we solve these 2 cases by taking a boolean value want_head
    // as the name suggests, if you are just starting to assign the frames, you want the first frame to be a head frame
    // if not you just want to assign all frames to be follow frames
    unsigned char assign_frames_in_block(unsigned char block,
                                         unsigned char start_at /* inclusive */,
                                         unsigned char end_at /* exclusive */,
                                         bool want_head /* the block can get request to mark all frames and follow frames */);

    /*
     * Given a start frame it tells the size of free frames block available
     * starting at the start frame specified
     */
    unsigned long check_continous_free_frames(unsigned long start_frame,
                                              unsigned long cutoff);

    /*
     * Given a start frame release all the frames assigned at that time
     * with that start frame (inclusive). The start frame must be a head frame
     */
    void release_pool_frames(unsigned long start_frame);

    /*
     * Utility function which actually does the operation of
     * assigning frames and modifying the data structure
     */
    void assign_frames(unsigned long start_frame,
                       unsigned long size);

public:

    // The frame size is the same as the page size, duh...    
    static const unsigned int FRAME_SIZE = Machine::PAGE_SIZE; 

    ContFramePool(unsigned long _base_frame_no,
                  unsigned long _n_frames,
                  unsigned long _info_frame_no,
                  unsigned long _n_info_frames);
    /*
     Initializes the data structures needed for the management of this
     frame pool.
     _base_frame_no: Number of first frame managed by this frame pool.
     _n_frames: Size, in frames, of this frame pool.
     EXAMPLE: If _base_frame_no is 16 and _n_frames is 4, this frame pool manages
     physical frames numbered 16, 17, 18 and 19.
     _info_frame_no: Number of the first frame that should be used to store the
     management information for the frame pool.
     NOTE: If _info_frame_no is 0, the frame pool is free to
     choose any frames from the pool to store management information.
     _n_info_frames: If _info_frame_no is 0, this argument specifies the
     number of consecutive frames needed to store the management information
     for the frame pool.
     EXAMPLE: If _info_frame_no is 699 and _n_info_frames is 3,
     then Frames 699, 700, and 701 are used to store the management information
     for the frame pool.
     NOTE: This function must be called before the paging system
     is initialized.
     */
    
    unsigned long get_frames(unsigned int _n_frames);
    /*
     Allocates a number of contiguous frames from the frame pool.
     _n_frames: Size of contiguous physical memory to allocate,
     in number of frames.
     If successful, returns the frame number of the first frame.
     If fails, returns 0.
     */
    
    void mark_inaccessible(unsigned long _base_frame_no,
                           unsigned long _n_frames);
    /*
     Marks a contiguous area of physical memory, i.e., a contiguous
     sequence of frames, as inaccessible.
     _base_frame_no: Number of first frame to mark as inaccessible.
     _n_frames: Number of contiguous frames to mark as inaccessible.
     */
    
    static void release_frames(unsigned long _first_frame_no);
    /*
     Releases a previously allocated contiguous sequence of frames
     back to its frame pool.
     The frame sequence is identified by the number of the first frame.
     NOTE: This function is static because there may be more than one frame pool
     defined in the system, and it is unclear which one this frame belongs to.
     This function must first identify the correct frame pool and then call the frame
     pool's release_frame function.
     */
    
    static unsigned long needed_info_frames(unsigned long _n_frames);
    /*
     Returns the number of frames needed to manage a frame pool of size _n_frames.
     The number returned here depends on the implementation of the frame pool and 
     on the frame size.
     EXAMPLE: For FRAME_SIZE = 4096 and a bitmap with a single bit per frame 
     (not appropriate for contiguous allocation) one would need one frame to manage a 
     frame pool with up to 8 * 4096 = 32k frames = 128MB of memory!
     This function would therefore return the following value:
       _n_frames / 32k + (_n_frames % 32k > 0 ? 1 : 0) (always round up!)
     Other implementations need a different number of info frames.
     The exact number is computed in this function..
     */
};

#endif
