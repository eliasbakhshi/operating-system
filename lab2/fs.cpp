#include <iostream>
#include <cstring>
#include <algorithm>
#include "fs.h"

FS::FS() {
    disk.read(FAT_BLOCK, (uint8_t*)fat); // Load the FAT table from the disk
}

FS::~FS() {
    // Save the FAT back to the disk when the program exits
    disk.write(FAT_BLOCK, (uint8_t*)fat);
}

// Formats the disk
int FS::format() {


    // Initialize FAT: Reserve block 0 for the root directory and block 1 for the FAT
    fat[0] = FAT_EOF; // Root directory block
    fat[1] = FAT_EOF; // FAT block
    std::fill(fat + 2, fat + (BLOCK_SIZE / 2), FAT_FREE); // Mark all other blocks as free

    // Clear the root directory
    uint8_t block[BLOCK_SIZE] = { 0 };
    disk.write(ROOT_BLOCK, block);

    // Write the FAT back to disk
    disk.write(FAT_BLOCK, (uint8_t*)fat);
    return 0;
}

// Creates a new file
int FS::create(std::string filepath) {
    // it the filepath name is more than 56 characters it should give should give en error and say the name should be 56 characters or leses
    if (filepath.length() > 56) {
        std::cerr << "Error: Filename exceeds the maximum length of 56 characters.\n";
        return -1;
    }

    // Load the root directory
    uint8_t root_block[BLOCK_SIZE];
    disk.read(ROOT_BLOCK, root_block);
    dir_entry* entries = (dir_entry*)root_block;


    // Check for duplicate file names
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && filepath == entries[i].file_name) {
            std::cerr << "Error: File already exists.\n";
            return -1;
        }
    }


    // Find a free directory entry
    int free_entry = -1;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk == 0) { // Unused entry
            free_entry = i;
            break;
        }
    }
    if (free_entry == -1) {
        std::cerr << "Error: No free directory entries available.\n";
        return -1;
    }

    // Read file content from user
    std::cout << "Enter file content (end with an empty line):\n";
    std::string content, line;
    while (true) {
        std::getline(std::cin, line);
        if (line.empty()) break;
        content += line + '\n';
    }

    // Allocate blocks for the file content
    const char* data = content.c_str();
    size_t size = content.size();
    int first_block = -1, prev_block = -1;

    for (int i = 2; i < BLOCK_SIZE / 2 && size > 0; i++) {
        if (fat[i] == FAT_FREE) {
            if (first_block == -1) first_block = i; // First block of the file

            if (prev_block != -1) fat[prev_block] = i; // Link blocks
            prev_block = i;

            // Write data to the block
            uint8_t block[BLOCK_SIZE] = { 0 };
            size_t chunk_size = std::min(size, (size_t)BLOCK_SIZE);
            std::memcpy(block, data, chunk_size);
            disk.write(i, block);

            data += chunk_size;
            size -= chunk_size;

            if (size == 0) fat[i] = FAT_EOF; // End of file
        }
    }

    if (size > 0) {
        std::cerr << "Error: Not enough space on disk.\n";
        return -1;
    }

    // Update the directory entry
    std::strcpy(entries[free_entry].file_name, filepath.c_str());
    entries[free_entry].size = content.size();
    entries[free_entry].first_blk = first_block;
    entries[free_entry].type = TYPE_FILE;
    entries[free_entry].access_rights = READ | WRITE;

    // Write the updated root directory and FAT back to the disk
    disk.write(ROOT_BLOCK, root_block);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    return 0;
}

// Reads and displays a file's content
int FS::cat(std::string filepath) {


    // Load the root directory
    uint8_t root_block[BLOCK_SIZE];
    disk.read(ROOT_BLOCK, root_block);
    dir_entry* entries = (dir_entry*)root_block;

    // Find the file
    int first_block = -1;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (filepath == entries[i].file_name) {
            first_block = entries[i].first_blk;
            break;
        }
    }
    if (first_block == -1) {
        std::cerr << "Error: File not found.\n";
        return -1;
    }

    // Read the file content block-by-block
    int block = first_block;
    uint8_t data[BLOCK_SIZE];
    while (block != FAT_EOF) {
        disk.read(block, data);
        std::cout << std::string((char*)data, BLOCK_SIZE);
        block = fat[block];
    }


    return 0;
}

// Lists the files in the current directory
int FS::ls() {
    std::cout << "name     size\n";

    // Load the root directory
    uint8_t root_block[BLOCK_SIZE];
    disk.read(ROOT_BLOCK, root_block);
    dir_entry* entries = (dir_entry*)root_block;

    // Display each file's name and size
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0) { // Valid file
            std::cout << entries[i].file_name << "       " << entries[i].size << "\n";
        }
    }

    return 0;
}



// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int FS::cp(std::string sourcepath, std::string destpath) {
    // Load the root directory
    uint8_t root_block[BLOCK_SIZE];
    disk.read(ROOT_BLOCK, root_block);
    dir_entry* entries = (dir_entry*)root_block;

    // Find the source file
    dir_entry* src_entry = nullptr;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && sourcepath == entries[i].file_name) {
            src_entry = &entries[i];
            break;
        }
    }
    if (!src_entry) {
        std::cerr << "Error: Source file not found.\n";
        return -1;
    }

    // Check if destination file already exists
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && destpath == entries[i].file_name) {
            std::cerr << "Error: Destination file already exists.\n";
            return -1;
        }
    }

    // Find a free directory entry for the new file
    dir_entry* dest_entry = nullptr;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk == 0) {
            dest_entry = &entries[i];
            break;
        }
    }
    if (!dest_entry) {
        std::cerr << "Error: No free directory entries available.\n";
        return -1;
    }

    // Copy file content
    int src_block = src_entry->first_blk;
    int dest_first_block = -1, prev_block = -1;
    while (src_block != FAT_EOF) {
        // Allocate a free block for the destination
        int dest_block = -1;
        for (int i = 2; i < BLOCK_SIZE / 2; i++) {
            if (fat[i] == FAT_FREE) {
                dest_block = i;
                break;
            }
        }
        if (dest_block == -1) {
            std::cerr << "Error: Not enough space on disk.\n";
            return -1;
        }

        // Copy data from source to destination block
        uint8_t data[BLOCK_SIZE];
        disk.read(src_block, data);
        disk.write(dest_block, data);

        // Update FAT
        if (prev_block != -1) fat[prev_block] = dest_block;
        prev_block = dest_block;
        if (dest_first_block == -1) dest_first_block = dest_block;

        src_block = fat[src_block];
    }
    fat[prev_block] = FAT_EOF;

    // Update destination directory entry
    std::strcpy(dest_entry->file_name, destpath.c_str());
    dest_entry->size = src_entry->size;
    dest_entry->first_blk = dest_first_block;
    dest_entry->type = TYPE_FILE;
    dest_entry->access_rights = src_entry->access_rights;

    // Write updates to disk
    disk.write(ROOT_BLOCK, root_block);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    return 0;
}


// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int FS::mv(std::string sourcepath, std::string destpath) {
    // Load the root directory
    uint8_t root_block[BLOCK_SIZE];
    disk.read(ROOT_BLOCK, root_block);
    dir_entry* entries = (dir_entry*)root_block;

    // Find the source file
    dir_entry* src_entry = nullptr;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && sourcepath == entries[i].file_name) {
            src_entry = &entries[i];
            break;
        }
    }
    if (!src_entry) {
        std::cerr << "Error: Source file not found.\n";
        return -1;
    }

    // Check if destination file already exists
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && destpath == entries[i].file_name) {
            std::cerr << "Error: Destination file already exists.\n";
            return -1;
        }
    }

    // Rename the file
    std::strcpy(src_entry->file_name, destpath.c_str());

    // Write updates to disk
    disk.write(ROOT_BLOCK, root_block);

    return 0;
}


// rm <filepath> removes / deletes the file <filepath>
int FS::rm(std::string filepath) {
    // Load the root directory
    uint8_t root_block[BLOCK_SIZE];
    disk.read(ROOT_BLOCK, root_block);
    dir_entry* entries = (dir_entry*)root_block;

    // Find the file
    dir_entry* entry = nullptr;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && filepath == entries[i].file_name) {
            entry = &entries[i];
            break;
        }
    }
    if (!entry) {
        std::cerr << "Error: File not found.\n";
        return -1;
    }

    // Free the FAT chain
    int block = entry->first_blk;
    while (block != FAT_EOF) {
        int next_block = fat[block];
        fat[block] = FAT_FREE;
        block = next_block;
    }

    // Clear the directory entry
    entry->first_blk = 0;

    // Write updates to disk
    disk.write(ROOT_BLOCK, root_block);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    return 0;
}


// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int FS::append(std::string filepath1, std::string filepath2) {
    // Load the root directory
    uint8_t root_block[BLOCK_SIZE];
    disk.read(ROOT_BLOCK, root_block);
    dir_entry* entries = (dir_entry*)root_block;

    // Find both files
    dir_entry* entry1 = nullptr, * entry2 = nullptr;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && filepath1 == entries[i].file_name) {
            entry1 = &entries[i];
        }
        if (entries[i].first_blk != 0 && filepath2 == entries[i].file_name) {
            entry2 = &entries[i];
        }
    }
    if (!entry1 || !entry2) {
        std::cerr << "Error: One or both files not found.\n";
        return -1;
    }

    // Traverse to the end of file 2
    int last_block2 = entry2->first_blk;
    while (fat[last_block2] != FAT_EOF) {
        last_block2 = fat[last_block2];
    }

    // Append content from file 1
    int block1 = entry1->first_blk;
    while (block1 != FAT_EOF) {
        // Allocate a new block for file 2
        int new_block = -1;
        for (int i = 2; i < BLOCK_SIZE / 2; i++) {
            if (fat[i] == FAT_FREE) {
                new_block = i;
                break;
            }
        }
        if (new_block == -1) {
            std::cerr << "Error: Not enough space on disk.\n";
            return -1;
        }

        // Copy data from file 1
        uint8_t data[BLOCK_SIZE];
        disk.read(block1, data);
        disk.write(new_block, data);

        // Update FAT
        fat[last_block2] = new_block;
        last_block2 = new_block;

        block1 = fat[block1];
    }
    fat[last_block2] = FAT_EOF;

    // Update file 2's size
    entry2->size += entry1->size;

    // Write updates to disk
    disk.write(ROOT_BLOCK, root_block);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int
FS::mkdir(std::string dirpath) {
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int
FS::cd(std::string dirpath) {
    std::cout << "FS::cd(" << dirpath << ")\n";
    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int
FS::pwd() {
    std::cout << "/\n";
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int
FS::chmod(std::string accessrights, std::string filepath) {
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}