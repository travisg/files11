// Copyright (c) 2024 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include "filesystem.h"

#include <array>
#include <cassert>
#include <vector>

#include "utils.h"
#include "file.h"

#define LOCAL_TRACE 0

namespace ods2 {

Filesystem::Filesystem() = default;
Filesystem::~Filesystem() = default;

int Filesystem::Mount(const std::string &diskfile) {
    if (mounted_) {
        return -1;
    }

    if (disk_.open(diskfile) < 0) {
        fprintf(stderr, "Failed to open file\n");
        return -1;
    }

    // read in the first home block from LBN 1
    // TODO: properly scan for it, it's not always on LBN 1
    if (disk_.read_block(1, &home_block_buf_) < 0) {
        fprintf(stderr, "failed to read home block\n");
        return -1;
    }

    hblock_ = (const ods2::home_block *)home_block_buf_.buf.data();

    if (LOCAL_TRACE) {
        LTRACEF("home block:\n");
        hblock_->dump();
    }

    // assuming the home block is correct:
    // index file bitmap is at block.ibmaplbn * 512
    // index file btiamp is block.ibmapsize * 512 bytes long
    // this means the proper start of the index file is at
    // (block.ibmaplbn + block.ibmapsize) * 512
    Disk::Block s;
    if (disk_.read_block(hblock_->ibmaplbn + hblock_->ibmapsize, &s) < 0) {
        fprintf(stderr, "failed to read index file record\n");
        return -1;
    }

    index_file_ = std::make_unique<File>(*this);
    if (index_file_->Open({reserved_files::INDEX, reserved_files::INDEX}, s) < 0) {
        fprintf(stderr, "error opening index file\n");
        return -1;
    }

    mounted_ = true;

    // try to read in the master file directory (4,4)
    mfd_file_ = std::make_unique<File>(*this);
    if (mfd_file_->Open({reserved_files::MFD, reserved_files::MFD}) < 0) {
        fprintf(stderr, "error opening master file directory\n");
        return -1;
    }
    if (!mfd_file_->is_dir()) {
        fprintf(stderr, "MFD file is not a directory\n");
        return -1;
    }
    return 0;

}

} // namespace ods2