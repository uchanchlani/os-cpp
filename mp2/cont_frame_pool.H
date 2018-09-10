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

class PoolManager;

class ContFramePool {
    
private:
    /* -- DEFINE YOUR CONT FRAME POOL DATA STRUCTURE(s) HERE. */
    unsigned long base_frame_no;
    unsigned long end_frame_no;
    unsigned long info_frame_no;
    unsigned long n_info_frames;
    unsigned long free_frames;
    unsigned char * bitmap;
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
    static PoolManager * pool_manager;

    void release_pool_frames(unsigned long _first_frame_no);
    unsigned char get_state_for_frame(unsigned long frame_no);
    bool is_free_frame(unsigned char frame_state);
    bool is_head_frame(unsigned char frame_state);
    bool is_follow_frame(unsigned char frame_state);
    unsigned long check_continous_free_frames(unsigned long start, unsigned long cutoff);
    unsigned short get_first_free_frame(unsigned char bit_block);
    unsigned short get_first_occupied_frame(unsigned char bit_block);
    void block_frames(unsigned long start, unsigned long size);

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

class PoolManager {
private:
    unsigned long base_frame;
    unsigned long n_frames;
    ContFramePool * curr_pool
    PoolManager * next;

public:
    PoolManager(unsigned long _base_frame,
                unsigned long _n_frames,
                ContFramePool * _curr_pool);


 
    ContFramePool * get_pool_for_frame(unsigned long _curr_frame);
    void add_new_pool(PoolManager * pool_manager);
}

#endif
