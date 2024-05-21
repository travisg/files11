// Copyright (c) 2024 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include "disk.h"

#include <cstdio>

Disk::Disk() = default;

Disk::~Disk() {
    if (fp) {
        fclose(fp);
    }
}

int Disk::open(const std::string &str) {
    fp = fopen(str.c_str(), "rb");
    if (!fp) {
        fprintf(stderr, "error opening file\n");
        return -1;
    }

    return 0;
}

int Disk::read(size_t offset, void *buf, size_t len) const {
    fseek(fp, offset, SEEK_SET);
    size_t err = fread(buf, 1, len, fp);
    if (err != len) {
        return -1;
    }
    return 0;
}
