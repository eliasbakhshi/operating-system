#include <iostream>
#include <cstdint>
#include <string>
#include "disk.h"
#include <vector>


#ifndef __FS_H__
#define __FS_H__

#define ROOT_BLOCK 0
#define FAT_BLOCK 1
#define FAT_FREE 0
#define FAT_EOF -1

#define TYPE_FILE 0
#define TYPE_DIR 1
#define READ 0x04
#define WRITE 0x02
#define EXECUTE 0x01

// #define DIR_SIZE BLOCK_SIZE/sizeof(dir_entry)
// #define FAT_ENTRIES BLOCK_SIZE/2

struct dir_entry {
    char file_name[56]; // name of the file / sub-directory
    uint32_t size; // size of the file in bytes
    uint16_t first_blk; // index in the FAT for the first block of the file
    uint8_t type; // directory (1) or file (0)
    uint8_t access_rights; // read (0x04), write (0x02), execute (0x01)
    uint16_t parent_blk; // index in the FAT for the parent directory

};

class FS {
private:
    int moveToDirectory(dir_entry* src_entry, int src_index, dir_entry* dest_dir);

    int copyToDirectory(dir_entry* src_entry, dir_entry* dest_dir);
    int copyWithNewName(dir_entry* src_entry, std::string newname);
    std::vector<std::string> splitPath(const std::string& path);
    dir_entry* findEntryInBlock(const std::string& name, uint16_t block);
    int navigateToPath(const std::string& path, bool excludeLast = false);
    bool isAbsolutePath(const std::string& path);
    std::string cleanPath(const std::string& path);
    Disk disk;
    // size of a FAT entry is 2 bytes
    std::string current_path;  // Add this member

    uint16_t current_dir_block; // Tracks current directory block number
    int16_t fat[BLOCK_SIZE/2];

public:
    FS();
    ~FS();
    // formats the disk, i.e., creates an empty file system
    int format();
    // create <filepath> creates a new file on the disk, the data content is
    // written on the following rows (ended with an empty row)
    int create(std::string filepath);
    // cat <filepath> reads the content of a file and prints it on the screen
    int cat(std::string filepath);
    // ls lists the content in the current directory (files and sub-directories)
    int ls();

    // cp <sourcepath> <destpath> makes an exact copy of the file
    // <sourcepath> to a new file <destpath>
    int cp(std::string sourcepath, std::string destpath);
    // mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
    // or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
    int mv(std::string sourcepath, std::string destpath);
    // rm <filepath> removes / deletes the file <filepath>
    int rm(std::string filepath);
    // append <filepath1> <filepath2> appends the contents of file <filepath1> to
    // the end of file <filepath2>. The file <filepath1> is unchanged.
    int append(std::string filepath1, std::string filepath2);

    // mkdir <dirpath> creates a new sub-directory with the name <dirpath>
    // in the current directory
    int mkdir(std::string dirpath);
    // cd <dirpath> changes the current (working) directory to the directory named <dirpath>
    int cd(std::string dirpath);
    // pwd prints the full path, i.e., from the root directory, to the current
    // directory, including the current directory name
    int pwd();

    // chmod <accessrights> <filepath> changes the access rights for the
    // file <filepath> to <accessrights>.
    int chmod(std::string accessrights, std::string filepath);
};

#endif // __FS_H__
