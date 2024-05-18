// Copyright (c) 2024 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once
#include <cstdio>
#include <string>
#include <array>

class Disk {
public:
    Disk() = default;
    ~Disk() {
        if (fp) {
            fclose(fp);
        }
    }

    // Disk blocks are always in units of 512 bytes
    struct Block {
        std::array<uint8_t, 512> buf;
    };

    int open(const std::string &str) {
        fp = fopen(str.c_str(), "rb");
        if (!fp) {
            fprintf(stderr, "error opening file\n");
            return -1;
        }

        return 0;
    }

    int read(size_t offset, void *buf, size_t len) const {
        fseek(fp, offset, SEEK_SET);
        size_t err = fread(buf, 1, len, fp);
        if (err != len) {
            return -1;
        }
        return 0;
    }

    int read_block(size_t blocknum, Block *block) const {
        return read(blocknum * 512, block->buf.data(), block->buf.size());
    }

private:
    FILE *fp = nullptr;
};

