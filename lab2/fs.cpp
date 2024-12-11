#include <iostream>
#include <cstring>
#include <algorithm>
#include <sstream> // Add this for std::istringstream
#include <string>  // Add this for std::getline
#include "fs.h"

FS::FS() : current_directory(ROOT_BLOCK) {
    current_path.push_back("/"); // Start with the root directory
}

FS::~FS() {
    // Save the FAT back to the disk when the program exits
    disk.write(FAT_BLOCK, (uint8_t *)fat);
}


// Formats the disk

int FS::format() {
    // Initialize FAT table
    std::fill(std::begin(fat), std::end(fat), FAT_FREE);
    fat[ROOT_BLOCK] = FAT_EOF;
    fat[FAT_BLOCK] = FAT_EOF;

    // Write FAT to disk
    disk.write(FAT_BLOCK, (uint8_t *)fat);

    // Initialize root directory
    uint8_t root_block[BLOCK_SIZE] = {0};
    disk.write(ROOT_BLOCK, root_block);

    // Set current directory to root
    current_directory = ROOT_BLOCK;
    current_path.clear();
    current_path.push_back("/");

    return 0;
}


// Creates a new file

int FS::create(std::string filepath) {
    // Check if the filepath name is more than 56 characters
    if (filepath.length() > 56) {
        std::cerr << "Error: Filename exceeds the maximum length of 56 characters.\n";
        return -1;
    }

    // Backup current directory and path
    uint16_t current_directory_backup = current_directory;
    std::vector<std::string> path_backup = current_path;

    // Split the path into directories
    std::istringstream iss(filepath);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(iss, token, '/')) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    // Traverse to the target directory
    for (size_t i = 0; i < tokens.size() - 1; ++i) {
        token = tokens[i];

        // Load the current directory
        uint8_t dir_block[BLOCK_SIZE];
        disk.read(current_directory, dir_block);
        dir_entry *entries = (dir_entry *)dir_block;

        // Find the directory
        int dir_block_no = -1;
        for (int j = 0; j < BLOCK_SIZE / sizeof(dir_entry); j++) {
            if (entries[j].first_blk != 0 && token == entries[j].file_name && entries[j].type == TYPE_DIR) {
                current_directory = entries[j].first_blk;
                current_path.push_back(token);
                dir_block_no = entries[j].first_blk;
                break;
            }
        }
        if (dir_block_no == -1) {
            std::cerr << "Error: Directory " << token << " not found.\n";
            // Restore the original current directory and path
            current_directory = current_directory_backup;
            current_path = path_backup;
            return -1;
        }
    }

    // Create the new file in the target directory
    token = tokens.back();
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_directory, dir_block);
    dir_entry *entries = (dir_entry *)dir_block;

    // Check for duplicate file names
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && token == entries[i].file_name) {
            std::cerr << "Error: File already exists.\n";
            // Restore the original current directory and path
            current_directory = current_directory_backup;
            current_path = path_backup;
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
        // Restore the original current directory and path
        current_directory = current_directory_backup;
        current_path = path_backup;
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
    const char *data = content.c_str();
    size_t size = content.size();
    int first_block = -1, prev_block = -1;

    for (int i = 2; i < BLOCK_SIZE / 2 && size > 0; i++) {
        if (fat[i] == FAT_FREE) {
            if (first_block == -1) first_block = i; // First block of the file

            if (prev_block != -1) fat[prev_block] = i; // Link blocks
            prev_block = i;

            // Write data to the block
            uint8_t block[BLOCK_SIZE] = {0};
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
        // Restore the original current directory and path
        current_directory = current_directory_backup;
        current_path = path_backup;
        return -1;
    }

    // Update the directory entry
    std::strcpy(entries[free_entry].file_name, token.c_str());
    entries[free_entry].size = content.size();
    entries[free_entry].first_blk = first_block;
    entries[free_entry].type = TYPE_FILE;
    entries[free_entry].access_rights = READ | WRITE;

    // Write the updated directory block and FAT back to the disk
    disk.write(current_directory, dir_block);
    disk.write(FAT_BLOCK, (uint8_t *)fat);

    // Restore the original current directory and path
    current_directory = current_directory_backup;
    current_path = path_backup;

    return 0;
}

// Reads and displays a file's content


int FS::cat(std::string filepath) {
    // Split the path into directories
    std::istringstream iss(filepath);
    std::string token;
    uint16_t current_directory_backup = current_directory; // Backup current directory
    std::vector<std::string> path_backup = current_path; // Backup current path

    while (std::getline(iss, token, '/')) {
        if (token.empty()) continue;

        // Load the current directory
        uint8_t dir_block[BLOCK_SIZE];
        disk.read(current_directory, dir_block);
        dir_entry *entries = (dir_entry *)dir_block;

        // Find the directory or file
        int dir_block_no = -1;
        for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
            if (entries[i].first_blk != 0 && token == entries[i].file_name) {
                if (entries[i].type == TYPE_DIR) {
                    // If it's a directory, update current_directory
                    current_directory = entries[i].first_blk;
                    current_path.push_back(token);
                } else {
                    // If it's a file, read and print its content
                    uint8_t file_block[BLOCK_SIZE];
                    disk.read(entries[i].first_blk, file_block);
                    std::cout.write((char*)file_block, entries[i].size);
                    std::cout << std::endl;
                    return 0;
                }
                dir_block_no = entries[i].first_blk;
                break;
            }
        }
        if (dir_block_no == -1) {
            std::cerr << "Error: File or directory not found.\n";
            return -1;
        }
    }

    // If we reach here, it means the path is a directory
    std::cerr << "Error: " << filepath << " is a directory.\n";
    // Restore the original current directory and path
    current_directory = current_directory_backup;
    current_path = path_backup;
    return -1;
}



// Lists the files in the current directory

int FS::ls() {
    std::cout << "name     size     type\n";

    // Load the current directory
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_directory, dir_block);
    dir_entry *entries = (dir_entry *)dir_block;

    // Display each file's name, size, and type
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0) { // Valid file or directory
            std::string type = (entries[i].type == TYPE_DIR) ? "DIR" : "FILE";
            std::cout << entries[i].file_name << "       " << (entries[i].size == 0 ? "-" : std::to_string(entries[i].size)) << "       " << type << "\n";
        }
    }

    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>


int FS::cp(std::string sourcepath, std::string destpath) {
    // Backup current directory and path
    uint16_t current_directory_backup = current_directory;
    std::vector<std::string> path_backup = current_path;

    // Split the source path into directories
    std::istringstream iss_source(sourcepath);
    std::string token;
    std::vector<std::string> source_tokens;
    while (std::getline(iss_source, token, '/')) {
        if (!token.empty()) {
            source_tokens.push_back(token);
        }
    }

    // Traverse to the source file
    dir_entry *source_entry = nullptr;
    for (size_t i = 0; i < source_tokens.size(); ++i) {
        token = source_tokens[i];

        // Load the current directory
        uint8_t dir_block[BLOCK_SIZE];
        disk.read(current_directory, dir_block);
        dir_entry *entries = (dir_entry *)dir_block;

        // Find the directory or file
        int dir_block_no = -1;
        for (int j = 0; j < BLOCK_SIZE / sizeof(dir_entry); j++) {
            if (entries[j].first_blk != 0 && token == entries[j].file_name) {
                if (entries[j].type == TYPE_DIR) {
                    // If it's a directory, update current_directory
                    current_directory = entries[j].first_blk;
                    current_path.push_back(token);
                } else {
                    // If it's a file, save the file entry
                    source_entry = &entries[j];
                    dir_block_no = entries[j].first_blk;
                    break;
                }
            }
        }
        if (dir_block_no == -1) {
            std::cerr << "Error: Source file not found.\n";
            // Restore the original current directory and path
            current_directory = current_directory_backup;
            current_path = path_backup;
            return -1;
        }
    }

    if (source_entry == nullptr || source_entry->type != TYPE_FILE) {
        std::cerr << "Error: Source is not a file.\n";
        // Restore the original current directory and path
        current_directory = current_directory_backup;
        current_path = path_backup;
        return -1;
    }

    // Backup the source file content
    std::vector<uint8_t> file_content(source_entry->size);
    uint8_t *data_ptr = file_content.data();
    int block_no = source_entry->first_blk;
    while (block_no != FAT_EOF) {
        disk.read(block_no, data_ptr);
        data_ptr += BLOCK_SIZE;
        block_no = fat[block_no];
    }

    // Restore the original current directory and path
    current_directory = current_directory_backup;
    current_path = path_backup;

    // Split the destination path into directories
    std::istringstream iss_dest(destpath);
    std::vector<std::string> dest_tokens;
    while (std::getline(iss_dest, token, '/')) {
        if (!token.empty()) {
            dest_tokens.push_back(token);
        }
    }

    // Traverse to the destination directory
    for (size_t i = 0; i < dest_tokens.size(); ++i) {
        token = dest_tokens[i];

        // Load the current directory
        uint8_t dir_block[BLOCK_SIZE];
        disk.read(current_directory, dir_block);
        dir_entry *entries = (dir_entry *)dir_block;

        // Find the directory
        int dir_block_no = -1;
        for (int j = 0; j < BLOCK_SIZE / sizeof(dir_entry); j++) {
            if (entries[j].first_blk != 0 && token == entries[j].file_name && entries[j].type == TYPE_DIR) {
                current_directory = entries[j].first_blk;
                current_path.push_back(token);
                dir_block_no = entries[j].first_blk;
                break;
            }
        }
        if (dir_block_no == -1) {
            std::cerr << "Error: Destination directory not found.\n";
            // Restore the original current directory and path
            current_directory = current_directory_backup;
            current_path = path_backup;
            return -1;
        }
    }

    // Create the new file entry in the destination directory
    token = source_tokens.back();
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_directory, dir_block);
    dir_entry *entries = (dir_entry *)dir_block;

    // Check for duplicate file names
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && token == entries[i].file_name) {
            std::cerr << "Error: File already exists in the destination directory.\n";
            // Restore the original current directory and path
            current_directory = current_directory_backup;
            current_path = path_backup;
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
        std::cerr << "Error: No free directory entries available in the destination directory.\n";
        // Restore the original current directory and path
        current_directory = current_directory_backup;
        current_path = path_backup;
        return -1;
    }

    // Allocate blocks for the new file content
    const uint8_t *data = file_content.data();
    size_t size = file_content.size();
    int first_block = -1, prev_block = -1;

    for (int i = 2; i < BLOCK_SIZE / 2 && size > 0; i++) {
        if (fat[i] == FAT_FREE) {
            if (first_block == -1) first_block = i; // First block of the file

            if (prev_block != -1) fat[prev_block] = i; // Link blocks
            prev_block = i;

            // Write data to the block
            uint8_t block[BLOCK_SIZE] = {0};
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
        // Restore the original current directory and path
        current_directory = current_directory_backup;
        current_path = path_backup;
        return -1;
    }

    // Update the directory entry
    std::strcpy(entries[free_entry].file_name, token.c_str());
    entries[free_entry].size = file_content.size();
    entries[free_entry].first_blk = first_block;
    entries[free_entry].type = TYPE_FILE;
    entries[free_entry].access_rights = READ | WRITE;

    // Write the updated directory block and FAT back to the disk
    disk.write(current_directory, dir_block);
    disk.write(FAT_BLOCK, (uint8_t *)fat);

    // Restore the original current directory and path
    current_directory = current_directory_backup;
    current_path = path_backup;

    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)

int FS::mv(std::string sourcepath, std::string destpath) {
    // Backup current state
    uint16_t current_directory_backup = current_directory;
    std::vector<std::string> path_backup = current_path;

    // Parse source path
    std::vector<std::string> source_tokens;
    std::istringstream iss_source(sourcepath);
    std::string token;
    bool source_absolute = sourcepath[0] == '/';

    while (std::getline(iss_source, token, '/')) {
        if (!token.empty()) {
            source_tokens.push_back(token);
        }
    }

    // Parse destination path
    std::vector<std::string> dest_tokens;
    std::istringstream iss_dest(destpath);
    bool dest_absolute = destpath[0] == '/';

    while (std::getline(iss_dest, token, '/')) {
        if (!token.empty()) {
            dest_tokens.push_back(token);
        }
    }

    // Navigate to source directory
    if (source_absolute) {
        current_directory = ROOT_BLOCK;
        current_path.clear();
        current_path.push_back("/");
    }

    for (size_t i = 0; i < source_tokens.size() - 1; i++) {
        token = source_tokens[i];
        if (token == "..") {
            if (current_path.size() > 1) {
                current_path.pop_back();
                // Navigate to parent
                current_directory = ROOT_BLOCK;
                for (size_t j = 1; j < current_path.size(); j++) {
                    uint8_t dir_block[BLOCK_SIZE];
                    disk.read(current_directory, dir_block);
                    dir_entry *entries = (dir_entry *)dir_block;
                    for (int k = 0; k < BLOCK_SIZE / sizeof(dir_entry); k++) {
                        if (entries[k].first_blk != 0 &&
                            current_path[j] == entries[k].file_name &&
                            entries[k].type == TYPE_DIR) {
                            current_directory = entries[k].first_blk;
                            break;
                        }
                    }
                }
            }
            continue;
        }

        uint8_t dir_block[BLOCK_SIZE];
        disk.read(current_directory, dir_block);
        dir_entry *entries = (dir_entry *)dir_block;
        bool found = false;

        for (int j = 0; j < BLOCK_SIZE / sizeof(dir_entry); j++) {
            if (entries[j].first_blk != 0 &&
                token == entries[j].file_name &&
                entries[j].type == TYPE_DIR) {
                current_directory = entries[j].first_blk;
                current_path.push_back(token);
                found = true;
                break;
            }
        }

        if (!found) {
            std::cerr << "Error: Source directory not found.\n";
            current_directory = current_directory_backup;
            current_path = path_backup;
            return -1;
        }
    }

    // Find and save source file
    uint8_t source_block[BLOCK_SIZE];
    disk.read(current_directory, source_block);
    dir_entry *source_entries = (dir_entry *)source_block;
    dir_entry *source_entry = nullptr;
    std::string source_name = source_tokens.back();

    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (source_entries[i].first_blk != 0 &&
            source_name == source_entries[i].file_name) {
            source_entry = &source_entries[i];
            break;
        }
    }

    if (!source_entry) {
        std::cerr << "Error: Source file not found.\n";
        current_directory = current_directory_backup;
        current_path = path_backup;
        return -1;
    }

    dir_entry saved_entry = *source_entry;
    std::memset(source_entry, 0, sizeof(dir_entry));
    disk.write(current_directory, source_block);

    // Navigate to destination
    current_directory = dest_absolute ? ROOT_BLOCK : current_directory_backup;
    current_path = dest_absolute ? std::vector<std::string>{("/")} : path_backup;

    for (const auto& dest_dir : dest_tokens) {
        if (dest_dir == "..") {
            if (current_path.size() > 1) {
                current_path.pop_back();
                current_directory = ROOT_BLOCK;
                for (size_t j = 1; j < current_path.size(); j++) {
                    uint8_t dir_block[BLOCK_SIZE];
                    disk.read(current_directory, dir_block);
                    dir_entry *entries = (dir_entry *)dir_block;
                    for (int k = 0; k < BLOCK_SIZE / sizeof(dir_entry); k++) {
                        if (entries[k].first_blk != 0 &&
                            current_path[j] == entries[k].file_name &&
                            entries[k].type == TYPE_DIR) {
                            current_directory = entries[k].first_blk;
                            break;
                        }
                    }
                }
            }
            continue;
        }

        uint8_t dir_block[BLOCK_SIZE];
        disk.read(current_directory, dir_block);
        dir_entry *entries = (dir_entry *)dir_block;
        bool found = false;

        for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
            if (entries[i].first_blk != 0 &&
                dest_dir == entries[i].file_name &&
                entries[i].type == TYPE_DIR) {
                current_directory = entries[i].first_blk;
                current_path.push_back(dest_dir);
                found = true;
                break;
            }
        }

        if (!found) {
            std::cerr << "Error: Destination directory not found.\n";
            current_directory = current_directory_backup;
            current_path = path_backup;
            return -1;
        }
    }

    // Create entry in destination
    uint8_t dest_block[BLOCK_SIZE];
    disk.read(current_directory, dest_block);
    dir_entry *dest_entries = (dir_entry *)dest_block;

    int free_entry = -1;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (dest_entries[i].first_blk == 0) {
            free_entry = i;
            break;
        }
    }

    if (free_entry == -1) {
        std::cerr << "Error: No free entry in destination directory.\n";
        return -1;
    }

    // Copy saved entry
    dest_entries[free_entry] = saved_entry;
    disk.write(current_directory, dest_block);

    // Restore original state
    current_directory = current_directory_backup;
    current_path = path_backup;

    return 0;
}

// rm <filepath> removes / deletes the file <filepath>


int FS::rm(std::string filepath) {
    // Backup current directory and path
    uint16_t current_directory_backup = current_directory;
    std::vector<std::string> path_backup = current_path;

    // Split the filepath into directories
    std::istringstream iss(filepath);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(iss, token, '/')) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    // Traverse to the file's directory
    for (size_t i = 0; i < tokens.size() - 1; ++i) {
        token = tokens[i];

        // Load the current directory
        uint8_t dir_block[BLOCK_SIZE];
        disk.read(current_directory, dir_block);
        dir_entry *entries = (dir_entry *)dir_block;

        // Find the directory
        bool found = false;
        for (int j = 0; j < BLOCK_SIZE / sizeof(dir_entry); j++) {
            if (entries[j].first_blk != 0 && token == entries[j].file_name && entries[j].type == TYPE_DIR) {
                current_directory = entries[j].first_blk;
                current_path.push_back(token);
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "Error: Directory not found.\n";
            current_directory = current_directory_backup;
            current_path = path_backup;
            return -1;
        }
    }

    // Find the file in current directory
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_directory, dir_block);
    dir_entry *entries = (dir_entry *)dir_block;

    // Get filename from last token
    std::string filename = tokens.back();

    dir_entry *entry = nullptr;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && filename == entries[i].file_name && entries[i].type == TYPE_FILE) {
            entry = &entries[i];
            break;
        }
    }

    if (!entry) {
        std::cerr << "Error: File not found.\n";
        current_directory = current_directory_backup;
        current_path = path_backup;
        return -1;
    }

      // Free the FAT chain
    int block = entry->first_blk;
    int next_block;

    while (block != FAT_EOF && block > 0 && block < BLOCK_SIZE/2) {
        next_block = fat[block];
        // std::cout << "Freeing block " << block << ", next block is " << next_block << std::endl;
        fat[block] = FAT_FREE;
        if (next_block == block) {
            std::cerr << "Error: Circular reference detected in FAT chain\n";
            break;
        }
        block = next_block;
    }

    // Clear the directory entry
    entry->first_blk = 0;
    entry->size = 0;

    // Write updates to disk
    disk.write(current_directory, dir_block);
    disk.write(FAT_BLOCK, (uint8_t *)fat);

    // Restore original directory and path
    current_directory = current_directory_backup;
    current_path = path_backup;

    return 0;
}



// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.

int FS::append(std::string filepath1, std::string filepath2) {
    // Load the root directory
    uint8_t root_block[BLOCK_SIZE];
    disk.read(ROOT_BLOCK, root_block);
    dir_entry *entries = (dir_entry *)root_block;

    // Find both files
    dir_entry *entry1 = nullptr, *entry2 = nullptr;
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
    disk.write(FAT_BLOCK, (uint8_t *)fat);

    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory


int FS::mkdir(std::string dirpath) {
    // Check if the directory name is too long
    if (dirpath.length() > 56) {
        std::cerr << "Error: Directory name exceeds the maximum length of 56 characters.\n";
        return -1;
    }

    // Load the current directory
    uint8_t dir_block[BLOCK_SIZE];
    disk.read(current_directory, dir_block);
    dir_entry *entries = (dir_entry *)dir_block;

    // Check for duplicate directory names
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
        if (entries[i].first_blk != 0 && dirpath == entries[i].file_name) {
            std::cerr << "Error: Directory already exists.\n";
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

    // Find a free block for the new directory
    int free_block = -1;
    for (int i = 2; i < BLOCK_SIZE / 2; i++) {
        if (fat[i] == FAT_FREE) {
            free_block = i;
            break;
        }
    }
    if (free_block == -1) {
        std::cerr << "Error: Not enough space on disk.\n";
        return -1;
    }

    // Mark the block as end of file in FAT
    fat[free_block] = FAT_EOF;

    // Clear the new directory block
    uint8_t block[BLOCK_SIZE] = {0};
    disk.write(free_block, block);

    // Update the directory entry
    std::strcpy(entries[free_entry].file_name, dirpath.c_str());
    entries[free_entry].size = 0;
    entries[free_entry].first_blk = free_block;
    entries[free_entry].type = TYPE_DIR;
    entries[free_entry].access_rights = READ | WRITE | EXECUTE;

    // Write the updated current directory and FAT back to the disk
    disk.write(current_directory, dir_block);
    disk.write(FAT_BLOCK, (uint8_t *)fat);

    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>

int FS::cd(std::string dirpath) {
    // Handle special cases
    if (dirpath == ".") {
        // Stay in the current directory
        return 0;
    } else if (dirpath == "..") {
        // Go to the parent directory if it exists
        if (current_path.size() > 1) {
            current_path.pop_back();
            // Reload the parent directory
            uint8_t dir_block[BLOCK_SIZE];
            disk.read(ROOT_BLOCK, dir_block);
            dir_entry *entries = (dir_entry *)dir_block;
            current_directory = ROOT_BLOCK;
            for (const auto& dir : current_path) {
                for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
                    if (entries[i].first_blk != 0 && dir == entries[i].file_name && entries[i].type == TYPE_DIR) {
                        current_directory = entries[i].first_blk;
                        disk.read(current_directory, dir_block);
                        entries = (dir_entry *)dir_block;
                        break;
                    }
                }
            }
        }
        return 0;
    } else if (dirpath[0] == '/') {
        // Change directory from the root
        current_directory = ROOT_BLOCK;
        current_path.clear();
        current_path.push_back("/");
        dirpath = dirpath.substr(1); // Remove the leading '/'
    }

    // Split the path into directories
    std::istringstream iss(dirpath);
    std::string token;
    while (std::getline(iss, token, '/')) {
        if (token.empty()) continue;

        // Load the current directory
        uint8_t dir_block[BLOCK_SIZE];
        disk.read(current_directory, dir_block);
        dir_entry *entries = (dir_entry *)dir_block;

        // Find the directory
        int dir_block_no = -1;
        for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); i++) {
            if (entries[i].first_blk != 0 && token == entries[i].file_name && entries[i].type == TYPE_DIR) {
                current_directory = entries[i].first_blk;
                current_path.push_back(token);
                dir_block_no = entries[i].first_blk;
                break;
            }
        }
        if (dir_block_no == -1) {
            std::cerr << "Error: Directory not found.\n";
            return -1;
        }
    }

    return 0;
}
// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the current directory name
int FS::pwd() {
    // Print the full path
    for (size_t i = 0; i < current_path.size(); ++i) {
        std::cout << current_path[i];
        if (i != 0 && i != current_path.size() - 1) {
            std::cout << "/";
        }
    }
    std::cout << "\n";
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int
FS::chmod(std::string accessrights, std::string filepath)
{
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}
