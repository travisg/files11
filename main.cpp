// Copyright (c) 2024 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include <array>
#include <cstdio>
#include <string>

#include "disk.h"
#include "filesystem.h"
#include "ods2.h"
#include "utils.h"

// Test disk image in the root of the project
const std::string diskfile = "ods2.disk";

void dump_directory(auto dir) {
    if (!dir->is_dir())
        return;

    int err;
    ods2::DirEntryList list;
    std::tie(err, list) = dir->ReadDirEntries();

    printf("directory '%s'\n", dir->name().c_str());
    for (auto &e: list) {
        printf("\t");
        e.dump();
    }
}

int recurse_directory(std::shared_ptr<ods2::File> dir, std::string leading_path, size_t level) {

    int err;
    ods2::DirEntryList list;
    std::tie(err, list) = dir->ReadDirEntries();

    for (auto &e: list) {
    
        std::shared_ptr<ods2::File> f = dir->OpenFileInDir(e.name);
        if (!f) {
            fprintf(stderr, "error opening file '%s'\n", e.name.c_str());
            continue;
        }

        printf("%s:%s\n", leading_path.c_str(), e.name.c_str());

        if (f->is_dir()) {
            // skip recursion for the MFD
            if (leading_path == "000000.DIR" && e.name == "000000.DIR")
                continue;

            recurse_directory(f, leading_path + ":" + e.name, level + 1);
        }
    }

    return 0;
}

int main(int, char **) {
    ods2::Filesystem fs;

    if (fs.Mount(diskfile) < 0) {
        fprintf(stderr, "Failed to mount volume\n");
        return 1;
    }

    auto root_dir = fs.OpenRootDir();
    if (!root_dir) {
        fprintf(stderr, "error opening root directory");
    }

    recurse_directory(root_dir, "000000.DIR", 0);

    return 0;
}