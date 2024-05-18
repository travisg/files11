// Copyright (c) 2024 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <cassert>
#include <cstdio>
#include <memory>
#include <string>

#include "ods2.h"
#include "disk.h"
#include "file.h"

namespace ods2 {

class Filesystem {
public:
    Filesystem();
    ~Filesystem();

    int Mount(const std::string &diskfile);

    std::shared_ptr<File> OpenRootDir() const { return mfd_file(); }

    // Most internal routines for File classes
    const std::shared_ptr<File> index_file() const {
        assert(mounted_);
        return index_file_;
    }
    const std::shared_ptr<File> mfd_file() const {
        assert(mounted_);
        return mfd_file_;
    }

    const Disk &disk() const {
        return disk_;
    }

    uint8_t cluster_factor() const { return hblock_->cluster; }
    uint32_t index_file_starting_vbn() const {
        return hblock_->ibmapvbn - 1 + hblock_->ibmapsize;
    }

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(Filesystem);

    bool mounted_ = false;
    Disk::Block home_block_buf_;
    const ods2::home_block *hblock_;
    std::shared_ptr<File> index_file_;
    std::shared_ptr<File> mfd_file_;
    Disk disk_;
};

} // ods2