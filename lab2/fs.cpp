#include <iostream>
#include <cstring>
#include <algorithm>
#include <string>
#include <sstream>
#include "fs.h"

FS::FS() {
    disk.read(FAT_BLOCK, (uint8_t *)fat); // Load the FAT table from the disk
    current_dir_block = ROOT_BLOCK;
    current_path = "/";

}

FS::~FS() {
    // Save the FAT back to the disk when the program exits
    disk.write(FAT_BLOCK, (uint8_t *)fat);
}

// Formats the disk
int FS::format() {
    // Initialize FAT
     fat[0] = FAT_EOF;  // Root directory block
    fat[1] = FAT_EOF;  // FAT block
    std::fill(fat + 2, fat + (BLOCK_SIZE / 2), FAT_FREE);

    // Initialize root directory block
    uint8_t root_block[BLOCK_SIZE] = {0};
    dir_entry* root_entries = (dir_entry*)root_block;

    // Set up ".." entry in root directory
    strcpy(root_entries[0].file_name, "..");
    root_entries[0].first_blk = ROOT_BLOCK;  // Points to itself
    root_entries[0].type = TYPE_DIR;
    root_entries[0].access_rights = READ | WRITE | EXECUTE;
    root_entries[0].parent_blk = ROOT_BLOCK;  // Root is its own parent
    root_entries[0].size = 0;

    // Write blocks
    disk.write(ROOT_BLOCK, root_block);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    // Set initial state
    current_dir_block = ROOT_BLOCK;
    current_path = "/";

    return 0;
}
// Creates a new file
int FS::create(std::string filepath) {
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_dir_block, dir_block);
    dir_entry* entries = (dir_entry*)dir_block;

    // Find free entry after ".."
    int free_entry = -1;
    for (int i = 1; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk == 0) {
            free_entry = i;
            break;
        }
        if (filepath == entries[i].file_name) return -1;
    }
    if (free_entry == -1) return -1;

    std::string content;
    std::string line;
    while (std::getline(std::cin, line) && !line.empty()) {
        content += line + '\n';
    }

    // Find free blocks
    int first_block = -1;
    int current_block = -1;
    int blocks_needed = (content.size() + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int blocks_found = 0;

    for (int i = 2; i < BLOCK_SIZE/2 && blocks_found < blocks_needed; i++) {
        if (fat[i] == FAT_FREE) {
            if (first_block == -1) first_block = i;
            else fat[current_block] = i;
            current_block = i;
            blocks_found++;
        }
    }
    if (blocks_found < blocks_needed) return -1;

    fat[current_block] = FAT_EOF;

    // Write content
    current_block = first_block;
    for (size_t pos = 0; pos < content.size(); pos += BLOCK_SIZE) {
        uint8_t block[BLOCK_SIZE] = {0};
        size_t chunk_size = std::min(static_cast<size_t>(BLOCK_SIZE), content.size() - pos);
        std::memcpy(block, content.c_str() + pos, chunk_size);
        disk.write(current_block, block);
        current_block = fat[current_block];
    }

    // Update directory preserving ".."
    strcpy(entries[free_entry].file_name, filepath.c_str());
    entries[free_entry].size = content.size();
    entries[free_entry].first_blk = first_block;
    entries[free_entry].type = TYPE_FILE;
    entries[free_entry].access_rights = READ | WRITE;
    entries[free_entry].parent_blk = current_dir_block;

    disk.write(current_dir_block, dir_block);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    return 0;
}
int FS::cat(std::string filepath) {
    // Load the current directory
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_dir_block, dir_block);
    dir_entry* entries = (dir_entry*)dir_block;

    // Check if filepath is a directory
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 &&
            filepath == entries[i].file_name &&
            entries[i].type == TYPE_DIR) {
            std::cerr << "Error: Cannot cat a directory\n";
            return -1;
        }
    }
    // Find the file
    int first_block = -1;
    uint32_t file_size = 0;
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 &&
            filepath == entries[i].file_name &&
            entries[i].type == TYPE_FILE) {
            first_block = entries[i].first_blk;
            file_size = entries[i].size;
            break;
        }
    }

    if (first_block == -1) {
        std::cerr << "Error: File not found\n";
        return -1;
    }

    // Read and print file contentsh
    int current_block = first_block;
    uint32_t bytes_read = 0;
    while (current_block != FAT_EOF && bytes_read < file_size) {
        uint8_t block[BLOCK_SIZE];
        disk.read(current_block, block);

        uint32_t bytes_to_print = std::min(static_cast<uint32_t>(BLOCK_SIZE), file_size - bytes_read);
        for (uint32_t i = 0; i < bytes_to_print; i++) {
            std::cout << block[i];
        }

        bytes_read += bytes_to_print;
        current_block = fat[current_block];
    }

    return 0;
}
// Lists the files in the current directory
// Update ls() to show file types
int FS::ls() {
    std::cout << "name\t type\t accessrights\t size\n";
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_dir_block, dir_block);
    dir_entry* entries = (dir_entry*)dir_block;

    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0) {
            std::string name = entries[i].file_name;
            std::string type = (entries[i].type == TYPE_DIR) ? "dir" : "file";
            std::string size = (entries[i].type == TYPE_DIR) ? "-" : std::to_string(entries[i].size);

            // Format access rights
            std::string rights = "";
            rights += (entries[i].access_rights & READ) ? 'r' : '-';
            rights += (entries[i].access_rights & WRITE) ? 'w' : '-';
            rights += (entries[i].access_rights & EXECUTE) ? 'x' : '-';

            printf("%-8s %-6s %-11s %s\n", name.c_str(), type.c_str(), rights.c_str(), size.c_str());
        }
    }
    return 0;
}// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int FS::mv(std::string sourcepath, std::string destpath) {
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_dir_block, dir_block);
    dir_entry* entries = (dir_entry*)dir_block;

    // Find source file

    // if the destination is starts with / go to the root and find the directory and copy the file to that directory
    std::string destpathfixed = cleanPath(destpath);
    dir_entry* src_entry = nullptr;
    int src_index = -1;
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && sourcepath == entries[i].file_name) {
            src_entry = &entries[i];
            src_index = i;
            break;
        }
    }
    if (!src_entry) return -1;

    // Handle parent directory (..)
    if (destpath == "..") {
        uint16_t parent_block = entries[0].parent_blk;
        dir_entry parent_dir;
        parent_dir.first_blk = parent_block;
        parent_dir.type = TYPE_DIR;
        int result = copyToDirectory(src_entry, &parent_dir);
        if (result == 0) {
            entries[src_index].first_blk = 0;
            disk.write(current_dir_block, dir_block);
        }
        return result;
    }
      dir_entry* dest_dir = nullptr;
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 &&
            destpathfixed == entries[i].file_name &&
            entries[i].type == TYPE_DIR) {
            dest_dir = &entries[i];
            break;
        }
    }
    if (dest_dir) {
        int result = copyToDirectory(src_entry, dest_dir);
        if (result == 0) {
            entries[src_index].first_blk = 0;
            disk.write(current_dir_block, dir_block);
        }
        return result;
    } else {
        // Simple rename/move
        for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
            if (entries[i].first_blk != 0 && destpath == entries[i].file_name) {
                return -1;  // Destination exists
            }
        }
        strcpy(src_entry->file_name, destpath.c_str());
        disk.write(current_dir_block, dir_block);
        return 0;
    }


    // Handle absolute/relative paths
    if (destpath.find('/') != std::string::npos) {
        std::vector<std::string> path_parts = splitPath(destpath);
        if (path_parts.empty()) return -1;

        uint16_t working_dir = isAbsolutePath(destpath) ? ROOT_BLOCK : current_dir_block;

        // Navigate through path
        for (const auto& part : path_parts) {
            if (part == "..") {
                uint8_t block[BLOCK_SIZE];
                disk.read(working_dir, block);
                dir_entry* dir_entries = (dir_entry*)block;
                working_dir = dir_entries[0].parent_blk;
            } else {
                dir_entry* next_dir = findEntryInBlock(part, working_dir);
                if (!next_dir || next_dir->type != TYPE_DIR) return -1;
                working_dir = next_dir->first_blk;
            }
        }

        dir_entry dest_dir;
        dest_dir.first_blk = working_dir;
        dest_dir.type = TYPE_DIR;
        int result = copyToDirectory(src_entry, &dest_dir);
        if (result == 0) {
            entries[src_index].first_blk = 0;
            disk.write(current_dir_block, dir_block);
        }
        return result;
    }

    // Check if destination is directory in current directory



}
std::string FS::cleanPath(const std::string& path) {
    if (path.empty()) return path;
    if (path[0] == '/') {
        return path.substr(1);  // Remove leading '/'
    }
    return path;
}

int FS::cp(std::string sourcepath, std::string destpath) {
    // Find source file
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_dir_block, dir_block);
    dir_entry* entries = (dir_entry*)dir_block;

    dir_entry* src_entry = nullptr;
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && sourcepath == entries[i].file_name) {
            src_entry = &entries[i];
            break;
        }
    }
    if (!src_entry) return -1;

    // Handle absolute path
    if (isAbsolutePath(destpath)) {
        // Read root directory
        uint8_t root_block[BLOCK_SIZE];
        disk.read(ROOT_BLOCK, root_block);
        dir_entry* root_entries = (dir_entry*)root_block;

        // Find destination directory
        std::string target = cleanPath(destpath);
        dir_entry* dest_dir = nullptr;

        for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
            if (root_entries[i].first_blk != 0 &&
                target == root_entries[i].file_name &&
                root_entries[i].type == TYPE_DIR) {
                dest_dir = &root_entries[i];
                break;
            }
        }

        if (!dest_dir) return -1;

        // Copy to destination directory
        dir_entry dest_dir_entry;
        dest_dir_entry.first_blk = dest_dir->first_blk;
        dest_dir_entry.type = TYPE_DIR;

        return copyToDirectory(src_entry, &dest_dir_entry);
    }

    // Handle parent directory (..)
    if (destpath == "..") {
        dir_entry parent_dir;
        parent_dir.first_blk = entries[0].parent_blk;
        parent_dir.type = TYPE_DIR;
        return copyToDirectory(src_entry, &parent_dir);
    }

    // Handle local directory or rename
    dir_entry* dest_dir = findEntryInBlock(destpath, current_dir_block);
    if (dest_dir && dest_dir->type == TYPE_DIR) {
        return copyToDirectory(src_entry, dest_dir);
    }

    return copyWithNewName(src_entry, destpath);
}
int FS::copyToDirectory(dir_entry* src_entry, dir_entry* dest_dir) {
    uint8_t dest_block[BLOCK_SIZE];
    disk.read(dest_dir->first_blk, dest_block);
    dir_entry* dest_entries = (dir_entry*)dest_block;

   // Check if file already exists in destination
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (dest_entries[i].first_blk != 0 &&
            strcmp(dest_entries[i].file_name, src_entry->file_name) == 0) {
            return -1;  // File exists
        }
    }

    // Find free entry
    int free_entry = -1;
    for (int i = 1; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (dest_entries[i].first_blk == 0) {
            free_entry = i;
            break;
        }
    }
    if (free_entry == -1) return -1;

    // Copy blocks and update FAT
    int first_new_block = -1;
    int prev_new_block = -1;
    int src_block = src_entry->first_blk;

    while (src_block != FAT_EOF) {
        int new_block = -1;
        for (int i = 2; i < BLOCK_SIZE/2; i++) {
            if (fat[i] == FAT_FREE) {
                new_block = i;
                break;
            }
        }
        if (new_block == -1) return -1;

        uint8_t block_data[BLOCK_SIZE];
        disk.read(src_block, block_data);
        disk.write(new_block, block_data);

        if (first_new_block == -1) first_new_block = new_block;
        if (prev_new_block != -1) fat[prev_new_block] = new_block;

        fat[new_block] = FAT_EOF;
        prev_new_block = new_block;
        src_block = fat[src_block];
    }

    // Create directory entry
    dest_entries[free_entry] = *src_entry;
    dest_entries[free_entry].first_blk = first_new_block;
    dest_entries[free_entry].parent_blk = dest_dir->first_blk;

    disk.write(dest_dir->first_blk, dest_block);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    return 0;
}

int FS::copyWithNewName(dir_entry* src_entry, std::string newname) {
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_dir_block, dir_block);
    dir_entry* entries = (dir_entry*)dir_block;
// Check if file already exists
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && strcmp(entries[i].file_name, newname.c_str()) == 0) {
            return -1;  // File exists
        }
    }

    // Find free entry
    int free_entry = -1;
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk == 0) {
            free_entry = i;
            break;
        }
    }
    if (free_entry == -1) return -1;
    // Copy blocks and update FAT
    int first_new_block = -1;
    int prev_new_block = -1;
    int src_block = src_entry->first_blk;

    while (src_block != FAT_EOF) {
        int new_block = -1;
        for (int i = 2; i < BLOCK_SIZE/2; i++) {
            if (fat[i] == FAT_FREE) {
                new_block = i;
                break;
            }
        }
        if (new_block == -1) return -1;

        uint8_t block_data[BLOCK_SIZE];
        disk.read(src_block, block_data);
        disk.write(new_block, block_data);

        if (first_new_block == -1) first_new_block = new_block;
        if (prev_new_block != -1) fat[prev_new_block] = new_block;

        fat[new_block] = FAT_EOF;
        prev_new_block = new_block;
        src_block = fat[src_block];
    }

    // Create directory entry
    entries[free_entry] = *src_entry;
    entries[free_entry].first_blk = first_new_block;
    strcpy(entries[free_entry].file_name, newname.c_str());

    disk.write(current_dir_block, dir_block);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    return 0;
}
int FS::rm(std::string filepath) {
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_dir_block, dir_block);
    dir_entry* entries = (dir_entry*)dir_block;

    // Find entry
    dir_entry* entry = nullptr;
    int entry_index = -1;
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && filepath == entries[i].file_name) {
            entry = &entries[i];
            entry_index = i;
            break;
        }
    }

    if (!entry) {
        std::cerr << "Error: File/directory not found\n";
        return -1;
    }

    // Handle directory removal
    if (entry->type == TYPE_DIR) {
        uint8_t dir_content[BLOCK_SIZE];
        disk.read(entry->first_blk, dir_content);
        dir_entry* dir_entries = (dir_entry*)dir_content;

        // Check if empty (only ".." entry)
        for (int i = 1; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
            if (dir_entries[i].first_blk != 0) {
                std::cerr << "Error: Directory not empty\n";
                return -1;
            }
        }

        // Free directory block
        fat[entry->first_blk] = FAT_FREE;
    } else {
        // Free file blocks
        int current_block = entry->first_blk;
        while (current_block != FAT_EOF) {
            int next_block = fat[current_block];
            fat[current_block] = FAT_FREE;
            current_block = next_block;
        }
    }

    // Clear directory entry
    entry->first_blk = 0;

    // Write updates
    disk.write(current_dir_block, dir_block);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    return 0;
}
// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int FS::append(std::string filepath1, std::string filepath2) {
    // Load current directory
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_dir_block, dir_block);
    dir_entry* entries = (dir_entry*)dir_block;

    // Find both files
    dir_entry *entry1 = nullptr, *entry2 = nullptr;
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && filepath1 == entries[i].file_name) {
            entry1 = &entries[i];
        }
        if (entries[i].first_blk != 0 && filepath2 == entries[i].file_name) {
            entry2 = &entries[i];
        }
    }

    if (!entry1 || !entry2) {
        std::cerr << "Error: File not found\n";
        return -1;
    }

    // Find last block of destination file
    int last_block2 = entry2->first_blk;
    uint32_t last_block_offset = entry2->size % BLOCK_SIZE;
    while (fat[last_block2] != FAT_EOF) {
        last_block2 = fat[last_block2];
    }

    // Read last block of destination
    uint8_t last_block_data[BLOCK_SIZE];
    disk.read(last_block2, last_block_data);

    // Copy source file blocks
    int src_block = entry1->first_blk;
    uint32_t remaining_size = entry1->size;
    uint32_t dest_offset = last_block_offset;

    while (src_block != FAT_EOF && remaining_size > 0) {
        uint8_t src_data[BLOCK_SIZE];
        disk.read(src_block, src_data);

        uint32_t copy_size = std::min(static_cast<uint32_t>(BLOCK_SIZE - dest_offset),
                                    remaining_size);

        // If current block is full, allocate new block
        if (dest_offset + copy_size > BLOCK_SIZE) {
            // Find new block
            int new_block = -1;
            for (int i = 2; i < BLOCK_SIZE/2; i++) {
                if (fat[i] == FAT_FREE) {
                    new_block = i;
                    break;
                }
            }
            if (new_block == -1) return -1;

            // Link blocks
            fat[last_block2] = new_block;
            fat[new_block] = FAT_EOF;
            last_block2 = new_block;
            dest_offset = 0;
        }

        // Copy data
        std::memcpy(last_block_data + dest_offset, src_data, copy_size);
        disk.write(last_block2, last_block_data);

        remaining_size -= copy_size;
        dest_offset += copy_size;
        src_block = fat[src_block];
    }

    // Update destination file size
    entry2->size += entry1->size;

    // Write changes
    disk.write(current_dir_block, dir_block);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    return 0;
}// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory

// In fs.cpp
std::vector<std::string> FS::splitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;

    while (std::getline(ss, part, '/')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

bool FS::isAbsolutePath(const std::string& path) {
    return !path.empty() && path[0] == '/';
}

dir_entry* FS::findEntryInBlock(const std::string& name, uint16_t block) {
    uint8_t block_data[BLOCK_SIZE];
    disk.read(block, block_data);
    dir_entry* entries = (dir_entry*)block_data;

    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && name == entries[i].file_name) {
            return &entries[i];
        }
    }
    return nullptr;
}
int FS::mkdir(std::string dirpath) {
    uint16_t original_dir = current_dir_block;
    uint16_t working_dir = current_dir_block;

    std::vector<std::string> path_parts = splitPath(dirpath);
    if (path_parts.empty()) return -1;

    std::string target_name = path_parts.back();
    path_parts.pop_back();

    // Navigate through each path component
    for (const auto& part : path_parts) {
        if (part == "..") {
            // Move back to parent directory
            uint8_t block[BLOCK_SIZE];
            disk.read(working_dir, block);
            dir_entry* entries = (dir_entry*)block;
            working_dir = entries[0].parent_blk;
        } else {
            // Find and enter directory
            dir_entry* entry = findEntryInBlock(part, working_dir);
            if (!entry || entry->type != TYPE_DIR) {
                current_dir_block = original_dir;
                return -1;
            }
            working_dir = entry->first_blk;
        }
    }

    // Now working_dir is where we want to create the new directory
    uint8_t block[BLOCK_SIZE];
    disk.read(working_dir, block);
    dir_entry* entries = (dir_entry*)block;

    // Find free entry
    int free_entry = -1;
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk == 0) {
            free_entry = i;
            break;
        }
        if (strcmp(entries[i].file_name, target_name.c_str()) == 0) {
            current_dir_block = original_dir;
            return -1;
        }
    }
    if (free_entry == -1) return -1;

    // Find free block
    int new_block = -1;
    for (int i = 2; i < BLOCK_SIZE/2; i++) {
        if (fat[i] == FAT_FREE) {
            new_block = i;
            break;
        }
    }
    if (new_block == -1) return -1;

    // Create directory
    uint8_t new_dir[BLOCK_SIZE] = {0};
    dir_entry* new_entries = (dir_entry*)new_dir;

    strcpy(new_entries[0].file_name, "..");
    new_entries[0].first_blk = new_block;
    new_entries[0].type = TYPE_DIR;
    new_entries[0].access_rights = READ | WRITE | EXECUTE;
    new_entries[0].size = 0;
    new_entries[0].parent_blk = working_dir;

    strcpy(entries[free_entry].file_name, target_name.c_str());
    entries[free_entry].first_blk = new_block;
    entries[free_entry].type = TYPE_DIR;
    entries[free_entry].access_rights = READ | WRITE | EXECUTE;
    entries[free_entry].size = 0;
    entries[free_entry].parent_blk = working_dir;

    fat[new_block] = FAT_EOF;
    disk.write(working_dir, block);
    disk.write(new_block, new_dir);
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    current_dir_block = original_dir;
    return 0;
}
int FS::cd(std::string dirpath) {
       uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_dir_block, dir_block);
    dir_entry* entries = (dir_entry*)dir_block;

    if (dirpath == "..") {
        // Don't move if already at root
        if (current_dir_block == ROOT_BLOCK) {
            return 0;
        }

        // Get parent block from first entry ("..")
        current_dir_block = entries[0].parent_blk;

        // Update current path
        if (current_path != "/") {
            size_t last_slash = current_path.find_last_of('/');
            if (last_slash == 0) {
                current_path = "/";
            } else {
                current_path = current_path.substr(0, last_slash);
            }
        }
        return 0;
    }

    // Navigate to subdirectory
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 &&
            dirpath == entries[i].file_name &&
            entries[i].type == TYPE_DIR) {
            current_dir_block = entries[i].first_blk;
            current_path = (current_path == "/") ?
                          current_path + dirpath :
                          current_path + "/" + dirpath;
            return 0;
        }
    }
    return -1;
}

int FS::pwd() {
    std::cout << current_path << std::endl;
    return 0;
}
// Update ls() to show file types


// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int FS::chmod(std::string accessrights, std::string filepath) {
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_dir_block, dir_block);
    dir_entry* entries = (dir_entry*)dir_block;

    // Find file/directory
    dir_entry* entry = nullptr;
    for (int i = 0; i < BLOCK_SIZE/sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && filepath == entries[i].file_name) {
            entry = &entries[i];
            break;
        }
    }
    if (!entry) {
        std::cerr << "Error: File not found\n";
        return -1;
    }

    // Convert accessrights string to int
    int rights = std::stoi(accessrights);
    if (rights < 0 || rights > 7) {
        std::cerr << "Error: Invalid access rights\n";
        return -1;
    }

    // Update access rights
    entry->access_rights = rights;
    disk.write(current_dir_block, dir_block);
    return 0;
}
