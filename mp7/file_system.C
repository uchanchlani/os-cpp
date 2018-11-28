/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

typedef unsigned int size_t;
extern void * operator new (size_t size);
extern void * operator new[] (size_t size);
extern void operator delete (void * p);
extern void operator delete[] (void * p);

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

unsigned long FileSystem::FS_SIGNATURE = 282732341;

FileSystem::FileSystem() {
    disk = NULL;
    isValid = false;
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

/*
 * Read the meta info from the file system. A blocking call
 */
void FileSystem::readMeta(unsigned char *usableBuf) {
    this->disk->read(0, usableBuf);
    FileSystemMeta * _meta = (FileSystemMeta *) usableBuf;
    meta.freeListBlock = _meta->freeListBlock;
    meta.iNodeListBlock = _meta->iNodeListBlock;
    meta.signature = _meta->signature;
    meta.start_data_block = _meta->start_data_block;
    this->isValid = this->meta.signature == FS_SIGNATURE;
}

void FileSystem::updateMeta(unsigned char *usableBuf) {
    unsigned char * _buf = (unsigned char *) &meta;
    for(unsigned short i = 0; i < sizeof(FileSystemMeta); i++) {
        usableBuf[i] = _buf[i];
    }
    this->disk->write(0, usableBuf);
}

/*
 * Convert the pointer to chars into pointer to free blocks
 */
void FileSystem::updateFreeBlocksFromBuf(unsigned char * buf) {
    unsigned short * _free_blocks = (unsigned short *) buf;
    for(unsigned short i = 0; i < FREE_BLOCKS_COUNT; i++) {
        free_blocks[i] = _free_blocks[i];
    }
}

/*
 * Convert into pointer to chars from pointer to free blocks
 */
void FileSystem::castFreeBlockToBuf(unsigned char *buf) {
    unsigned char * _buf = (unsigned char *) free_blocks;
    for(unsigned short i = 0; i < FREE_BLOCKS_COUNT * sizeof(unsigned short); i++) {
        buf[i] = _buf[i];
    }
}

/*
 * Convert the pointer to chars into pointer to inodes
 */
void FileSystem::updateINodesFromBuf(unsigned char *buf) {
    INode * _inodes = (INode *) buf;
    for(unsigned short i = 0; i < INODES_COUNT; i++) {
        inodes[i].fileName = _inodes[i].fileName;
        inodes[i].startBlock = _inodes[i].startBlock;
    }
}

/*
 * Convert into pointer to chars from pointer to inodes
 */
void FileSystem::castINodesToBuf(unsigned char *buf) {
    unsigned char * _buf = (unsigned char *) inodes;
    for(unsigned short i = 0; i < INODES_COUNT * sizeof(INode); i++) {
        buf[i] = _buf[i];
    }
}

/*
 * Get the next available free block
 */
unsigned short FileSystem::getFreeBlock(unsigned char * usableBuf) {
    for(unsigned short i = FREE_BLOCKS_COUNT - 1; i > 0; i--) {
        if(free_blocks[i] != 0) {
            unsigned short block = free_blocks[i];
            free_blocks[i] = 0;
            updateFreeBlocksToDisk(usableBuf);
            return block;
        }
    }
    unsigned short candidate = free_blocks[0];
    if(candidate != 0) {
        this->disk->read(candidate, usableBuf);
        updateFreeBlocksFromBuf(usableBuf);
        updateFreeBlocksToDisk(usableBuf);
        return candidate;
    } else {
        return 0;
    }
}

/*
 * return the free block
 */
bool FileSystem::returnFreeBlock(unsigned char * usableBuf, unsigned short * block_nums, unsigned long size) {
    while (size > 0) {
        for (unsigned short i = 1; i < FREE_BLOCKS_COUNT && size > 0; i++) {
            if (free_blocks[i] == 0) {
                free_blocks[i] = *block_nums;
                size--;
                block_nums++;
            }
        }
        if(size > 0) {
            unsigned short candidate = *block_nums;
            size--;
            block_nums++;
            updateFreeBlocksToDisk(usableBuf);
            refreshFreeBlocks();
            free_blocks[0] = candidate;
        }
    }
    updateFreeBlocksToDisk(usableBuf);
    return true;
}

/*
 * Pure blocking call to update the free block datastructure to disk
 */
void FileSystem::updateFreeBlocksToDisk(unsigned char * usableBuf) {
    castFreeBlockToBuf(usableBuf);
    this->disk->write(this->meta.freeListBlock, usableBuf);
}

/*
 * Pure blocking call to update the inodess block datastructure to disk
 */
void FileSystem::updateINodesToDisk(unsigned char * usableBuf) {
    castINodesToBuf(usableBuf);
    this->disk->write(this->meta.iNodeListBlock, usableBuf);
}

/*
 * reset functionality
 */
void FileSystem::refreshFreeBlocks() {
    for(unsigned short i = 0; i < FREE_BLOCKS_COUNT; i++) {
        free_blocks[i] = 0;
    }
}

/*
 * reset functionality
 */
void FileSystem::clearINodes() {
    for(unsigned short i = 0; i < INODES_COUNT; i++) {
        inodes[i].fileName = 0;
    }
}

// Public functions

/*
 * We read the file system from disk and check if it valid or not. we return if the file system is successfully mounted
 */
bool FileSystem::Mount(SimpleDisk * _disk) {
    disk = _disk;
    unsigned char buf[512];
    readMeta(buf);
    if(!this->isValid) {
        return false;
    }

    this->disk->read(this->meta.iNodeListBlock, buf);
    updateINodesFromBuf(buf);
    this->disk->read(this->meta.freeListBlock, buf);
    updateFreeBlocksFromBuf(buf);
    return true;
}

/*
 * Load my file system into the disk. and make it valid.
 */
bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) {
    Console::puts("formatting disk\n");
    unsigned char buf[512];
    FileSystem tempFileSystem;
    tempFileSystem.disk = _disk;
    tempFileSystem.isValid = false;
    tempFileSystem.meta.size_in_blocks = (unsigned short)(_size / 512);
    tempFileSystem.meta.signature = FS_SIGNATURE;
    tempFileSystem.meta.freeListBlock = 1;
    tempFileSystem.meta.iNodeListBlock = 2;
    tempFileSystem.meta.start_data_block = 3;

    tempFileSystem.clearINodes();
    tempFileSystem.updateINodesToDisk(buf);

    tempFileSystem.refreshFreeBlocks();
    unsigned short * allBlocks = (unsigned short *) new unsigned short[tempFileSystem.meta.size_in_blocks];
    for (unsigned short i = 0; i < tempFileSystem.meta.size_in_blocks; i++) {
        allBlocks[i] = i;
    }
    tempFileSystem.returnFreeBlock(buf, allBlocks + tempFileSystem.meta.start_data_block,
            tempFileSystem.meta.size_in_blocks - tempFileSystem.meta.start_data_block);

    tempFileSystem.updateMeta(buf);
    delete[] allBlocks;
    return true;
}

/*
 * looks up the inodes for the file. If found we allocate the file pointer to it else return null
 */
File * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file\n");
    if (!isValid) return NULL;
    for(unsigned short i = 0; i < INODES_COUNT; i++) {
        if(inodes[i].fileName == _file_id) {
            unsigned char tempBuf[512];
            disk->read(inodes[i].startBlock, tempBuf);
            return new File(inodes[i].startBlock, tempBuf, this);
        }
    }
    return NULL;
}

/*
 * looks up the inodes for the file. If found we return null
 * else we create the file
 */
bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file\n");
    unsigned char tempBuf[512];
    if (!isValid) return false;
    File *pFile = LookupFile(_file_id);
    if(pFile != NULL) {
        Console::puts("File Already exists.\n");
        delete pFile;
        return false;
    }
    unsigned short _blockNum = this->getFreeBlock(tempBuf);
    File::resetFileBits(tempBuf);
    disk->write(_blockNum, tempBuf);
    for(unsigned short i = 0; i < INODES_COUNT; i++) {
        if(inodes[i].fileName == 0) {
            inodes[i].fileName = (unsigned char)_file_id;
            inodes[i].startBlock = _blockNum;
            updateINodesToDisk(tempBuf);
            return true;
        }
    }
    return false;
}

/*
 * looks up the inodes for the file. If not found we return null
 * else we return all file blocks and update the inodes to delete the file
 */
bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file\n");
    unsigned char tempBuf[512];
    if (!isValid) return false;
    File *pFile = LookupFile(_file_id);
    if (pFile == NULL) {
        Console::puts("File does not exists.\n");
        return false;
    }
    pFile->returnBlocks(false);
    for (unsigned short i = 0; i < INODES_COUNT; i++) {
        if (inodes[i].fileName == _file_id) {
            inodes[i].fileName = 0;
            updateINodesToDisk(tempBuf);
            break;
        }
    }
    delete pFile;
    return true;
}
