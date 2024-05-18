// Copyright (c) 2024 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// structures in the ODS2 filesystem
namespace ods2 {

struct home_block {
    uint32_t homelbn; // this LBN
    uint32_t alhomelbn; // alternate home LBN
    uint32_t altidxlbn; // backup index LBN
    uint16_t struclev; // structure level
    uint16_t cluster; // cluster factor
    uint16_t homevbn;
    uint16_t alhomevbn;
    uint16_t altidxvbn;
    uint16_t ibmapvbn; // VBM of the index file bitmap
    uint32_t ibmaplbn; // VBM of the index file bitmap
    uint32_t maxfiles;
    uint16_t ibmapsize;
    uint16_t resfiles;
    uint16_t devtype;
    uint16_t rvn;
    uint16_t setcount;
    uint16_t volchar;
    uint32_t volowner;
    uint16_t protect;
    uint16_t fileprot;
    uint16_t checksum1;
    uint64_t credate;
    uint8_t window;
    uint8_t lru_lim;
    uint16_t extend;
    uint64_t retainmin;
    uint64_t retainmax;
    uint64_t revdate;
    uint8_t min_class[20];
    uint8_t max_class[20];
    // padding
    uint8_t pad[512-184];

    uint32_t serialnum;
    uint8_t strucname[12];
    uint8_t volname[12];
    uint8_t ownername[12];
    uint8_t format[12];
    uint16_t checksum2;

    void dump() const {
        //hexdump8_ex(&block, sizeof(block));
        
        printf("Home block at LBN %#x (offset %#x)\n", homelbn, homelbn * 512);
        printf("\thome block VBN %#x\n", homevbn);
        printf("\talt home LBN %#x (offset %#x)\n", alhomelbn, alhomelbn * 512);
        printf("\talt home VBN %#x\n", alhomevbn);
        printf("\tstructure level %#x\n", struclev);
        printf("\tcluster factor %u (%#x bytes)\n", cluster, cluster * 512);
        printf("\tmaxfiles %u\n", maxfiles);
        printf("\tresfiles %u\n", resfiles);
        printf("\tindex file bitmap LBN %#x (offset %#x)\n", ibmaplbn, ibmaplbn * 512);
        printf("\tindex file bitmap VBN %#x\n", ibmapvbn);
        printf("\tindex file bitmap size %#x\n", ibmapsize);
        printf("\tbackup index file header LBN %#x (offset %#x)\n", altidxlbn, altidxlbn * 512);
        printf("\tbackup index file header vBN %#x\n", altidxvbn);
    }

} __attribute__((packed));

static_assert(sizeof(home_block) == 512);

enum class reserved_files {
    INDEX = 1,
    BITMAP = 2,
    BAD_BLOCK = 3,
    MFD = 4,
    CORE_IMAGE = 5,
    VOL_SET = 6,
    STANDARD_CONTINUATION = 7,
    BACKUP_JOURNAL = 8,
    PENDING_BAD_BLOCK = 9,
};

struct file_id {
    uint16_t low_num;
    uint16_t sequence_num;
    uint8_t  rv_num;
    uint8_t  high_num;

    file_id() = default;
    file_id(uint32_t num, uint16_t sequence, uint8_t rv) {
        low_num = num & 0xffff;
        high_num = (num >> 16) & 0xff;
        sequence_num = sequence;
        rv_num = rv;
    }
    file_id(reserved_files num, reserved_files sequence) :
        file_id((uint32_t)num, (uint32_t)sequence, 0) {}

    uint32_t file_num() const {
        return low_num | (high_num << 16);
    }

    std::string id_str() const {
        char buf[64];
        snprintf(buf, sizeof(buf), "%u,%u,%u", file_num(), sequence_num, rv_num);
        return buf;
    }
    void dump_id() const {
        printf("%u,%u,%u", file_num(), sequence_num, rv_num);
    }
    void dump() const {
        printf("file_id ");
        dump_id();
        puts("");
    }

    bool operator==(const file_id &id) {
        return low_num == id.low_num &&
            sequence_num == id.sequence_num &&
            rv_num == id.rv_num &&
            high_num == id.high_num;
    }
};

static_assert(sizeof(file_id) == 6);

struct file_record_attribute {
    uint8_t rtype;
    uint8_t rattrib;
    uint16_t rsize; // record size
    uint32_t _hiblk; // highest vbn allocated (pdp11 order)
    uint32_t _efblk; // vpn of the end of the file (pdp11 order)
    uint16_t ffbyte; // first free byte
    uint8_t  bktsize; // file bucket size
    uint8_t  vfcsize; // fixed control area size
    uint16_t maxrec; // maximum record size
    uint16_t defext; // default extend size
    uint16_t gbc; // global buffer count
    uint8_t pad[8];
    uint16_t versions; // directory default version limit

    // swizzle the PDP11 order (16 bit values reversed)
    uint32_t hiblk() const {
        return ((_hiblk >> 16) & 0xffff) | ((_hiblk << 16) & 0xffff0000);
    }
    uint32_t efblk() const {
        return ((_efblk >> 16) & 0xffff) | ((_efblk << 16) & 0xffff0000);
    }

    void dump() const {
        printf("\trtype %#x\n", rtype);
        printf("\trattrib %#x\n", rattrib);
        printf("\trsize %#x\n", rsize);
        printf("\thiblk %#x\n", hiblk());
        printf("\tefblk %#x\n", efblk());
        printf("\tffbyte %#x\n", ffbyte);
        printf("\tbktsize %#x\n", bktsize);
        printf("\tvfcsize %#x\n", vfcsize);
        printf("\tmaxrec %#x\n", maxrec);
        printf("\tdefext %#x\n", defext);
        printf("\tgbc %#x\n", gbc);
        printf("\tversions %#x\n", versions);
    }
};

static_assert(sizeof(file_record_attribute) == 32);

struct file_header {
    uint8_t id_offset;
    uint8_t map_area_offset;
    uint8_t acl_offset;
    uint8_t rsvd_offset;
    uint16_t seg_num;
    uint16_t struclev;
    file_id fid;
    file_id ext_fid;
    file_record_attribute file_rec_attributes;
    uint32_t filechar;
    uint16_t recprot;
    uint8_t map_inuse;
    uint8_t acc_mode;
    uint32_t fileowner;
    uint16_t fileprot;
    file_id backlink;
    uint16_t journal;
    uint16_t pad;
    uint32_t highwater;

    void dump() const {
        printf("\tfile map offset %#x\n", map_area_offset);
        printf("\tmap words in use %#x\n", map_inuse);
        printf("\tid offset %#x\n", id_offset);
        printf("\tfile id "); fid.dump_id(); puts("");
        printf("\text file id "); ext_fid.dump_id(); puts("");
        printf("\tbacklink "); backlink.dump_id(); puts("");
        printf("\tfile flags %#x\n", filechar);
        printf("\thighwater vbn %#x\n", highwater);
        file_rec_attributes.dump();
    }
};

static_assert(sizeof(file_header) == 80);

// flags for the above file characteristics field
const uint32_t file_char_nobackup = (1<<1);
const uint32_t file_char_writeback = (1<<2);
const uint32_t file_char_readcheck = (1<<3);
const uint32_t file_char_writecheck = (1<<4);
const uint32_t file_char_contigb = (1<<5);
const uint32_t file_char_locked = (1<<6);
const uint32_t file_char_contig = (1<<7);
const uint32_t file_char_badacl = (1<<11);
const uint32_t file_char_spool = (1<<12);
const uint32_t file_char_directory = (1<<13);
const uint32_t file_char_badblock = (1<<14);
const uint32_t file_char_markdel = (1<<15);
const uint32_t file_char_nocharge = (1<<16);
const uint32_t file_char_erase = (1<<17);

struct file_ident {
    uint8_t filename[20];
    uint16_t revision;
    uint64_t credate;
    uint64_t revdate;
    uint64_t expdate;
    uint64_t bakdate;
    uint8_t filenamext[66];

    std::string name() const {
        char str[20+66+1];

        memcpy(str, filename, sizeof(filename));
        memcpy(str + 20, filenamext, sizeof(filenamext));
        str[sizeof(str) - 1] = '\0';

        // trim from the right
        for (int i = strlen(str) - 1; i >= 0; i--) {
            if (isspace(str[i])) {
                str[i] = '\0';
            } else {
                break;
            }
        }

        return std::string(str);
    }

    void dump() const {
        printf("\tfilename '%s'\n", name().c_str());
        printf("\trevision %u\n", revision);
    }
} __attribute__((packed));

static_assert(sizeof(file_ident) == 120);

struct dir_header {
    uint16_t record_byte_count;
    uint16_t version_limit;
    uint8_t flags;
    uint8_t name_byte_count;
};

static_assert(sizeof(dir_header) == 6);

struct dir_version_fid {
    uint16_t version;
    file_id id;
};

static_assert(sizeof(dir_version_fid) == 8);

} // ods2