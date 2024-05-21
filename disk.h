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
    Disk();
    ~Disk();

    // Disk blocks are always in units of 512 bytes
    struct Block {
        std::array<uint8_t, 512> buf;
    };

    int open(const std::string &str);

    int read(size_t offset, void *buf, size_t len) const;

    int read_block(size_t blocknum, Block *block) const {
        return read(blocknum * 512, block->buf.data(), block->buf.size());
    }

private:
    FILE *fp = nullptr;
};

