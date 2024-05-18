// Copyright (c) 2024 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ods2.h"
#include "disk.h"
#include "utils.h"

namespace ods2 {

class Filesystem;

struct DirEntry {
    std::string name;
    uint16_t version;
    file_id fid;

    void dump() const {
        printf("dir entry '%s;%u' fid %s\n", name.c_str(), version, fid.id_str().c_str());
    }
};

using DirEntryList = std::vector<DirEntry>;

class File {
public:
    explicit File(const Filesystem &fs);
    ~File();

    int Open(ods2::file_id id);

    // Used to bootstrap the INDEXF.SYS file
    int Open(ods2::file_id id, const Disk::Block &s);

    int ReadVbn(uint32_t vbn, Disk::Block *block) const;

    std::unique_ptr<File> OpenFileInDir(const std::string &name) const;

    ods2::file_id id() const { 
        assert(opened_);
        return *id_;
    }

    std::string name() const;
    bool is_dir() const {
        assert(opened_);
        return fhdr_->filechar & file_char_directory;
    }

    std::tuple<int, DirEntryList> ReadDirEntries() const;

private:
    struct extent {
        uint32_t vbn;
        uint32_t lbn;
        uint32_t block_count;
    };

    int ParseFileHeader(ods2::file_id id);

    const Filesystem &fs_;
    bool opened_ = false;

    Disk::Block file_rec_block_ {}; // a copy of the primary file record block

    // Pointers into the file record block above
    const ods2::file_header *fhdr_ {};
    const ods2::file_ident *fident_ {};
    const ods2::file_id *id_ {};

    // list of all the extents of the file
    std::vector<extent> extents_{};
};

} // ods2