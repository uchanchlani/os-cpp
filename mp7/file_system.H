/* 
    File: file_system.H

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 10/04/05

    Description: Simple File System.
    

*/

#ifndef _FILE_SYSTEM_H_                   // include file only once
#define _FILE_SYSTEM_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define FREE_BLOCKS_COUNT 256
#define INODES_COUNT 127
/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

//#include "file.H"
#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARD DECLARATIONS */ 
/*--------------------------------------------------------------------------*/

class File;

/*--------------------------------------------------------------------------*/
/* F i l e S y s t e m  */
/*--------------------------------------------------------------------------*/

struct INode {
    unsigned char fileName;
    unsigned short startBlock;
};

struct FileSystemMeta {
    unsigned short freeListBlock;
    unsigned short iNodeListBlock;
    /*
     * size in blocks is the total number of blocks that we have in the system disk
     */
    unsigned short size_in_blocks;
    /*
     * Validated this signature with the signature present in the data structure.
     * Chances of these 32 bits being exactly the same as the signature is very rare
     */
    unsigned long signature;
    unsigned long start_data_block;
};

class FileSystem {

friend class File; /* -- not sure if we need this; feel free to delete */

private:
     /* -- DEFINE YOUR FILE SYSTEM DATA STRUCTURES HERE. */

     /*
      * I have made the file system data structure bulky by adding every buffer as a non dynamic data structure.
      * This is okay because we will have very limited file systems in our running system.
      *
      * Also I have restricted the number of blocks that we can have in our disk to 2^16 (2 bytes storable).
      * this means we can support a disk of upto 2^25 bytes which is 32 MB.
      */

     SimpleDisk * disk;

     /*
      * free blocks stores the list of free blocks in the system. 1-255 entries stores a list of some free blocks.
      * The 0th entry stores the pointer to one more free block which is used as a block to store other free blocks
      * And the cycle goes on.
      * We store the base free blocks list in block 0
      */
     unsigned short free_blocks[FREE_BLOCKS_COUNT];
     /*
      * inodes stores things a bit smartly
      * It uses 3 bytes to store the name of the file in 1 byte
      * and start address of the file in the next 2 bytes.
      * Thus we allow a total of 255 different file names for the file system (0 is a reserved name and cannot be used)
      * And we allow at max 170 files to be present in our vanilla version of the file system
      * We store this free blocks in block 1
      *
      * Now when we read the disk we need to know that weather it has a valid file system or not.
      * To do that we store some signature value in the last 2 bytes of the block 1 of the disk.
      * If we read that signature value we load the file system else we mark this as invalid file system.
      */
     INode inodes[INODES_COUNT];

     FileSystemMeta meta;
     bool isValid;

     static unsigned long FS_SIGNATURE;

     /*
      * You will encounter this usableBuf in the entire file.
      * This is basically the memory location of the start of a 512 bytes buffer which can be used in whatever the function wants to
      * It is generally used to interact with the disk
      */

     void readMeta(unsigned char * usableBuf);
     void updateMeta(unsigned char * usableBuf);

     unsigned short getFreeBlock(unsigned char * usableBuf);
     bool returnFreeBlock(unsigned char * usableBuf, unsigned short * block_nums, unsigned long size);
     void refreshFreeBlocks();
     void updateFreeBlocksToDisk(unsigned char * usableBuf);
     void updateFreeBlocksFromBuf(unsigned char * buf);
     void castFreeBlockToBuf(unsigned char * buf);

     void clearINodes();
     void updateINodesToDisk(unsigned char * usableBuf);
     void updateINodesFromBuf(unsigned char * buf);
     void castINodesToBuf(unsigned char * buf);

public:

    FileSystem();
    /* Just initializes local data structures. Does not connect to disk yet. */
    
    bool Mount(SimpleDisk * _disk);
    /* Associates this file system with a disk. Limit to at most one file system per disk.
     Returns true if operation successful (i.e. there is indeed a file system on the disk.) */
    
    static bool Format(SimpleDisk * _disk, unsigned int _size);
    /* Wipes any file system from the disk and installs an empty file system of given size. */
    
    File * LookupFile(int _file_id);
    /* Find file with given id in file system. If found, return the initialized
     file object. Otherwise, return null. */
    
    bool CreateFile(int _file_id);
    /* Create file with given id in the file system. If file exists already,
     abort and return false. Otherwise, return true. */
    
    bool DeleteFile(int _file_id);
    /* Delete file with given id in the file system; free any disk block occupied by the file. */
   
};
#endif
