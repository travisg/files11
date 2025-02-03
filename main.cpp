// Copyright (c) 2024 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include <cstdio>
#include <string>

#include "filesystem.h"

// Test disk image in the root of the project
const std::string diskfile = "ods2.disk";

int recurse_directory(std::shared_ptr<ods2::File> dir, std::string leading_path, size_t level) {
    auto [err, list] = dir->ReadDirEntries();

    for (auto &e : list) {
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