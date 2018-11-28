/*
     File        : file.H

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Simple File class with sequential read/write operations.
 
*/

#ifndef _FILE_H_
#define _FILE_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* class  F i l e   */
#include "file_system.H"
//class FileSystem;
/*--------------------------------------------------------------------------*/

class File  {
    friend class FileSystem;
private:
    unsigned short startBlock;
    unsigned long offset;
    // we will not read the file again and again. Read it once and keep the buffer handy
    unsigned char blockBuf[512];
    unsigned short blockNum;
    bool readMode;
    bool writeMode;

    FileSystem * fileSystem;
    /* -- maybe it would be good to have a reference to the file system? */

    void generateNewEmptyBlock();
    void flushBlock();
    bool returnBlocks(bool keepFirst);
    unsigned short getNextBlock();
    void setNextBlock(unsigned short _blockNum);

    static void resetFileBits(unsigned char * buf);
    
public:

    File(unsigned short startBlock, unsigned char * startBuf, FileSystem * _fileSystem);
    /* Constructor for the file handle. Set the ’current
     position’ to be at the beginning of the file. */
    
    int Read(unsigned int _n, char * _buf);
    /* Read _n characters from the file starting at the current location and
     copy them in _buf.  Return the number of characters read. 
     Do not read beyond the end of the file. */
    
    void Write(unsigned int _n, const char * _buf);
    /* Write _n characters to the file starting at the current location, 
     if we run past the end of file, 
     we increase the size of the file as needed. */
    
    void Reset();
    /* Set the ’current position’ at the beginning of the file. */
    
    void Rewrite();
    /* Erase the content of the file. Return any freed blocks.
     Note: This function does not delete the file! It just erases its content. */
    
    bool EoF();
    /* Is the current location for the file at the end of the file? */

};

#endif
