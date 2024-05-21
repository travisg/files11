// Copyright (c) 2024 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include "file.h"

#include <cassert>

#include "disk.h"
#include "filesystem.h"
#include "utils.h"

#define LOCAL_TRACE 0

namespace ods2 {

File::File(const Filesystem &fs) : fs_(fs) {}
File::~File() = default;

// Open a file based on the id
int File::Open(ods2::file_id id) {
    assert(!opened_);

    // read the file record out of the index file
    LTRACEF("Opening file %s from index file\n", id.id_str().c_str());

    const auto index_file = fs_.index_file();
    assert(index_file);

    if (index_file->ReadVbn(id.file_num() + fs_.index_file_starting_vbn(), &file_rec_block_) < 0) {
        fprintf(stderr, "error reading file record\n");
        return -1;
    }

    if (ParseFileHeader(id) < 0) {
        fprintf(stderr, "error parsing file header\n");
        return -1;
    }

    opened_ = true;

    return 0;
}

// Used to bootstrap the INDEXF.SYS file
int File::Open(ods2::file_id id, const Disk::Block &s) {
    assert(!opened_);

    LTRACEF("Opening file %s from raw disk sector\n", id.id_str().c_str());

    // make a copy of the disk sector
    file_rec_block_ = s;

    if (ParseFileHeader(id) < 0) {
        fprintf(stderr, "error parsing file header\n");
        return -1;
    }

    opened_ = true;

    return 0;
}

int File::ParseFileHeader(ods2::file_id id) {

    // TODO: validate file header
    fhdr_ = (const ods2::file_header *)file_rec_block_.buf.data();
    if (LOCAL_TRACE) {
        fhdr_->dump();
    }

    // Make sure the file number and sequence num match
    if (fhdr_->fid != id) {
        return -1;
    }

    fident_ = (const ods2::file_ident *)(file_rec_block_.buf.data() + fhdr_->id_offset * 2);
    if (LOCAL_TRACE) {
        fident_->dump();
    }

    // Read in the map
    assert(extents_.size() == 0);
    const uint16_t *map_area = (uint16_t *)(file_rec_block_.buf.data() + fhdr_->map_area_offset * 2);

    // walk the map list
    LTRACEF("parsing extent list:\n");
    uint32_t vbn = 1;
    const uint16_t *map_area_stop = map_area + fhdr_->map_inuse;
    while (map_area != map_area_stop) {
        LTRACEF_LEVEL(2, "%#x %#x %#x %#x\n", map_area[0], map_area[1], map_area[2], map_area[3]);
        uint32_t lbn;
        uint32_t block_count;
        const uint8_t format = (map_area[0] >> 14) & 0x3;
        switch (format) {
            default:
                // not actually possible, since it's a 2 bit field
                assert(false);
                continue;
            case 0:
                // TODO: handle
                continue;
            case 1:
                block_count = map_area[0] & 0xff;
                lbn = ((map_area[0] << 8) & 0x3f0000) | map_area[1];
                break;
            case 2:
                block_count = map_area[0] & 0x3fff;
                lbn = (map_area[2] << 16) | map_area[1];
                break;
            case 3:
                block_count = (map_area[0] & 0x3fff) << 16;
                block_count |= map_area[1];
                lbn = (map_area[3] << 16) | map_area[2];
                break;
        }
        // previously read block count n + 1
        block_count++;

        LTRACEF_NOFILE("\tformat %u: cluster %#x vbn %#x lbn %#x (offset %#x), count %#x\n",
                format, vbn / fs_.cluster_factor(), vbn, lbn, lbn * 512, block_count);
        extents_.push_back({ vbn, lbn, block_count });

        assert((block_count % fs_.cluster_factor()) == 0);

        // accumulate vbn
        vbn += block_count;
        map_area += format + 1;
    }

    LTRACEF_NOFILE("\ttotal vbns %#x\n", vbn - 1);

    return 0;
}

std::string File::name() const {
    assert(opened_);
    return fident_->name();
}

int File::ReadVbn(const uint32_t vbn, Disk::Block *block) const {
    assert(vbn > 0);

    LTRACEF("vbn %#x\n", vbn);

    // translate vbn to lbn
    uint32_t lbn = UINT32_MAX;
    for (const auto &extent: extents_) {
        if (vbn >= extent.vbn && vbn < extent.vbn + extent.block_count) {
            // found it here
            lbn = extent.lbn + vbn - extent.vbn;
            break;
        }
    }
    if (lbn == UINT32_MAX) {
        fprintf(stderr, "failed looking up lbn from vbn\n");
        return -1;
    }

    LTRACEF("translated vbn %#x to lbn %#x (offset %#lx)\n", vbn, lbn, (unsigned long)lbn * 512);

    return fs_.disk().read_block(lbn, block);
}

std::tuple<int, DirEntryList> File::ReadDirEntries() const {
    assert(is_dir());

    DirEntryList entries;

    // find the max number of blocks we'll want to read in
    uint32_t endvbn = fhdr_->file_rec_attributes.efblk();
    for (uint32_t vbn = 1; vbn < endvbn; vbn++) {
        Disk::Block block;
        if (ReadVbn(vbn, &block) < 0) {
            fprintf(stderr, "readdir: failed to read vbn\n");
            return { -1, {} };
        }

        uintptr_t dir_pointer = (uintptr_t)block.buf.data();
        auto *dh = (const ods2::dir_header *)dir_pointer;

        while (dh->record_byte_count != 0xffff) {
            LTRACEF_NOFILE("record_byte_count %u, ", dh->record_byte_count);
            LTRACEF_NOFILE("name_byte_count %u, ", dh->name_byte_count);
            LTRACEF_NOFILE("verlimit %u, ", dh->version_limit);
            LTRACEF_NOFILE("flags %#x, ", dh->flags);

            // copy the name out
            std::array<uint8_t, 256> namebuf;
            auto *name_ptr = (const uint8_t *)(dir_pointer + sizeof(dir_header));
            memcpy(namebuf.data(), name_ptr, dh->name_byte_count);
            namebuf[dh->name_byte_count] = '\0';
            LTRACEF_NOFILE("name '%s'\n", namebuf.data());

            // deal with the variable part
            const auto num_dvfids = (dh->record_byte_count - sizeof(dir_header) - dh->name_byte_count + 2) / sizeof(ods2::dir_version_fid);
            auto *dvfid = (const ods2::dir_version_fid *)(dir_pointer + sizeof(dir_header) + ROUNDUP(dh->name_byte_count, 2));
            for (uint32_t i = 0; i < num_dvfids; i++) {
                LTRACEF_NOFILE("\tmax version %u, fid %s\n", dvfid->version, dvfid->id.id_str().c_str());
                
                entries.push_back({ std::string((const char *)namebuf.data()), dvfid->version, dvfid->id });

                dvfid++;
            }

            dir_pointer += dh->record_byte_count + 2;
            dh = (const ods2::dir_header *)dir_pointer;
        }
    }

    return { 0, std::move(entries) };
}

namespace {
DirEntry *FindInDirEntryList(DirEntryList &list, const std::string &name) {
    for (auto &e: list) {
        // First match should be the highest version since they're sorted
        // highest version first in the directory.
        //printf("comparing '%s' to '%s'\n", name.c_str(), e.name.c_str());
        if (e.name == name) {
            return &e;
        }
    }

    return nullptr;
}
}

std::unique_ptr<File> File::OpenFileInDir(const std::string &name) const {
    (void)name;

    // get a list of our entries
    auto [err, entries] = ReadDirEntries();
    if (err < 0) {
        fprintf(stderr, "error reading MFD");
        return nullptr;
    }

    // Dump the current directory
    if (LOCAL_TRACE) {
        for (auto &e: entries) {
            printf("\t");
            e.dump();
        }
    }

    // walk the directory name
    auto entry = FindInDirEntryList(entries, name);
    if (!entry) {
        fprintf(stderr, "failed to find file '%s' in directory\n", name.c_str());
        return nullptr;
    }

    if (LOCAL_TRACE) {
        printf("found entry for name '%s': ", name.c_str());
        entry->dump();
    }

    // Create a file for it
    auto file = std::make_unique<File>(fs_);
    if (file->Open(entry->fid) < 0) {
        fprintf(stderr, "error creating file\n");
        return nullptr;
    }

    return file;
}


} // ods2
