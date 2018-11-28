/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/
/*
 * Constructor. We keep the pointer to the file system as the file needs it.
 * The file system provides the initial block number to the file system along with the data in that block for the file to use it
 */
File::File(unsigned short _startBlock, unsigned char * _blockBuf, FileSystem * _fileSystem) {
    Console::puts("In file constructor.\n");
    fileSystem = _fileSystem;
    startBlock = _startBlock;
    offset = 0;
    blockNum = _startBlock;
    for(unsigned short i = 0; i < 512; i++) {
        blockBuf[i] = _blockBuf[i];
    }
    writeMode = false;
    readMode = (blockBuf[0] != 0);
    // if there is data in the file we cannot simply write it. We need to reset it first to write it. Hence we cannot go in the write mode
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

// All the private functions

/*
 * Blocking call which gets the new block from the system, appends it to the end of current block
 * and flushes the current data block into the disk
 * it also resets the bits for the current data block to all 0s because that is what is expected.
 * called only in the write mode
 */
void File::generateNewEmptyBlock() {
    if(offset % 510 != 0) {
        // it should never reach here
        assert(false);
    }
    unsigned char tempBuf[512];
    unsigned short _blockNum = fileSystem->getFreeBlock(tempBuf);
    setNextBlock(_blockNum);
    flushBlock();
    resetFileBits(blockBuf);
    blockNum = _blockNum;
//    flushBlock();
}

/*
 * Flushes the block into the disk. Pure blocking call
 */
void File::flushBlock() {
    fileSystem->disk->write((unsigned long)blockNum, blockBuf);
}

/*
 * Utility function to sequentially return all the blocks back to the system.
 * PS: If we are rewriting the file we may want to keep 1 block and return the rest. This function takes care of that.
 */
bool File::returnBlocks(bool keepFirst) {
    unsigned char tempBuf[512];
    bool isStale = false;
    unsigned short retBlocks[100];
    unsigned long sizeRetBlocks = 0;
    Reset();
    if(keepFirst) {
        blockNum = getNextBlock();
        isStale = blockNum > 0;
    }
    while(blockNum != 0) {
        if(isStale) {
            fileSystem->disk->read(blockNum, blockBuf);
        }
        retBlocks[sizeRetBlocks++] = blockNum;
        blockNum = getNextBlock();
        isStale = true;
        if(sizeRetBlocks == 100) {
            fileSystem->returnFreeBlock(tempBuf, retBlocks, sizeRetBlocks);
        }
    }
    if(sizeRetBlocks > 0) {
        fileSystem->returnFreeBlock(tempBuf, retBlocks, sizeRetBlocks);
    }
    return isStale;
}

/*
 * For the buffer currently in the system, it gets the next block number from the buffer.
 * It only gets the block number and is a non blocking call
 */
unsigned short File::getNextBlock() {
    return *((unsigned short *)(blockBuf + 510));
}

/*
 * Counter function to set the next block
 */
void File::setNextBlock(unsigned short _blockNum) {
    *((unsigned short *)(blockBuf+510)) = _blockNum;
}

/*
 * Non blocking function, to reset the bits of that buffer.
 */
void File::resetFileBits(unsigned char *buf) {
    for(unsigned short i = 0; i < 512; i++)
        buf[i] = 0;
}

// The asked public functions

/*
 * We go to the read mode. Then we start reading from the disk like this.
 * We read char by char from the buf. If we encounter a 0 we stop
 * If the buf ends and we have the next block stored we get the next block from the file system and continue reading
 * char by char as before.
 */
int File::Read(unsigned int _n, char * _buf) {
    Console::puts("reading from file\n");
    if(!(readMode ^ writeMode)) {
        readMode = true;
        writeMode = false;
    } else if (!readMode) {
        Console::puts("Please don't write and read on the file simultaneously. It is not good you know.\n");
        return 0;
    }
    if(_n == 0 || blockBuf[offset % 510] == 0)
        return 0;
    unsigned int charRead = 0;
    while(_n > 0) {
        _buf[charRead] = blockBuf[offset % 510];
        if(_buf[charRead] == 0) {
            return charRead;
        }
        charRead++;
        offset++;
        _n--;
        if(offset % 510 == 0) {
            unsigned short nextBlock = getNextBlock();
            if (nextBlock != 0) {
                fileSystem->disk->read(nextBlock, blockBuf);
            } else {
                blockBuf[0] = 0; // eof, so that next time if someone tries to read it they get this eof
            }
        }
    }
    return charRead;
}

/*
 * Very similar to read mode with some subtle differences
 * We go to the write mode. Then we start writing from the disk like this.
 * We write char by char to the buf. If we encounter end of characters we stop
 * If the buf ends it is flushed into the system, and a new block is brought in from the file system and we continue writing
 * char by char as before.
 */
void File::Write(unsigned int _n, const char * _buf) {
    Console::puts("writing to file\n");
    if(!(readMode ^ writeMode)) {
        readMode = false;
        writeMode = true;
    } else if (!writeMode) {
        Console::puts("Please don't write and read on the file simultaneously. It is not good you know.\n");
    }
    if(_n == 0)
        return;
    unsigned int charRead = 0;
    while(_n > 0) {
        blockBuf[offset % 510] = _buf[charRead];
        if(_buf[charRead] == 0) {
            break;
        }
        charRead++;
        offset++;
        _n--;
        if(offset % 510 == 0 && _n > 0) {
            generateNewEmptyBlock();
        }
    }
    flushBlock();
}

/*
 * Reset the offset to the start of the file
 */
void File::Reset() {
    Console::puts("reset current position in file\n");
    offset = 0;
    if (blockNum != startBlock) {
        blockNum = startBlock;
        fileSystem->disk->read(blockNum, blockBuf);
    }
    writeMode = false;
    readMode = (blockBuf[0] != 0);
    // if there is data in the file we cannot simply write it. We need to reset it first to write it. Hence we cannot go in the write mode
}

/*
 * This also resets the offset but it also overwrite the file to 0 bytes and returns any excess blocks back to the file system.
 */
void File::Rewrite() {
    Console::puts("erase content of file\n");
    if(!(readMode ^ writeMode)) {
        readMode = false;
        writeMode = true;
    } else if (!writeMode) {
        Console::puts("Please don't write and read on the file simultaneously. It is not good you know.\n");
    }
    bool isStale = returnBlocks(true);
    blockNum = startBlock;
    if(isStale)
        fileSystem->disk->read(blockNum, blockBuf);
    blockBuf[0] = 0;
//    if(getNextBlock() != 0) {
        setNextBlock(0);
        flushBlock();
//    }
}

/*
 * Check if it is eof by simply checking the character pointed by the buf at this moment.
 */
bool File::EoF() {
    Console::puts("testing end-of-file condition\n");
    return blockBuf[offset % 510] == 0;
}
