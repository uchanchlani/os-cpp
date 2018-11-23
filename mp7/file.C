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

File::File(unsigned short _startBlock, unsigned char * _blockBuf, FileSystem * _fileSystem) {
    /* We will need some arguments for the constructor, maybe pointer to disk
     block with file management and allocation data. */
    Console::puts("In file constructor.\n");
    fileSystem = _fileSystem;
    startBlock = _startBlock;
    offset = 0;
    blockNum = _startBlock;
    for(unsigned short i = 0; i < 512; i++) {
        blockBuf[i] = _blockBuf[i];
    }
    readMode = false;
    writeMode = false;
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

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

void File::Reset() {
    Console::puts("reset current position in file\n");
    offset = 0;
    if (blockNum != startBlock) {
        blockNum = startBlock;
        fileSystem->disk->read(blockNum, blockBuf);
    }
}

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


bool File::EoF() {
    Console::puts("testing end-of-file condition\n");
    return blockBuf[offset % 510] == 0;
}

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

void File::resetFileBits(unsigned char *buf) {
    for(unsigned short i = 0; i < 512; i++)
        buf[i] = 0;
}

void File::flushBlock() {
    fileSystem->disk->write((unsigned long)blockNum, blockBuf);
}

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

unsigned short File::getNextBlock() {
    return *((unsigned short *)(blockBuf + 510));
}

void File::setNextBlock(unsigned short _blockNum) {
    *((unsigned short *)(blockBuf+510)) = _blockNum;
}

