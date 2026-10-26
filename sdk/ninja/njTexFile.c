/* texture file reading through a sector buffer */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "ninja/njtexmem.h"

Sint32 njTexFread2(void *buf, Uint32 size);

/* the buffer the reads go through, and its size in bytes and in sectors */
Sint8 *_nj_file_buffer_st = _nj_file_buffer;
Sint32 _nj_file_buffer_max_size = GDD_FS_SCTSIZE;
Sint32 _nj_buffer_sect_size_ = 1;
/* the open file's state. _nj_file_sect_ptr_ is the first sector of the last
 * buffer's worth read from the file, or 0 after njTexFread2 reads to the
 * file's end. a read through the buffer leaves the buffer holding those
 * sectors. the zero initializers put the state in initialized data after
 * the buffer's globals, not in bss */
Sint32 _nj_file_sect_ptr_ = 0;
Sint32 _nj_sect_max_size_ = 0; /* the file's size in sectors */
Sint32 _nj_file_max_size_ = 0; /* the file's size in bytes */
Sint32 _nj_file_byte_ptr_ = 0; /* the offset of the next byte to read */
Sint32 _nj_buffer_byte_ptr_ = 0; /* that byte's offset in the buffer */
Sint32 _nj_buffer_r_byte_size_ = 0; /* the buffer's bytes from that one on */
Sint32 _nj_buffer_read_flag_ = 0;

GDFS file_ptr; /* the open file */
Sint32 sect_size;
Sint32 load_size;

/* record a failed read in _nj_texerr_: GDFS's status and error for the file,
 * and NJD_TEXERR_FILELOAD */
#pragma inline(read_failed)
static void read_failed(void)
{
    _nj_texerr_.gdstat = gdFsGetStat(file_ptr);
    _nj_texerr_.gderr = gdFsGetErrStat(file_ptr);
    _nj_texerr_.texerr = NJD_TEXERR_FILELOAD;
    /* @hack redundant: a label nothing jumps to, which keeps each expansion's
     * last stores in its own arm (MOV.L R0,@(16,R14) at 0x8c66ac18); without
     * it, the expansions branch to one shared tail, and 403 of njTexFread's and
     * njTexFread2's 538 words differ */
done:; /* NOLINT(clang-diagnostic-unused-label) */
}

Sint32 njTexFopen(const char *filename)
{
    if ((file_ptr = gdFsOpen(filename, NULL)) == NULL) {
        _nj_texerr_.texerr = NJD_TEXERR_FILEOPEN;
        return -2;
    }
    if (!gdFsGetFileSize(file_ptr, &_nj_file_max_size_)) {
        _nj_texerr_.texerr = NJD_TEXERR_FILEOPEN;
        return -2;
    }
    _nj_sect_max_size_ =
        (_nj_file_max_size_ + GDD_FS_SCTSIZE - 1) >> SECTOR_SHIFT;
    _nj_file_sect_ptr_ = 0;
    _nj_file_byte_ptr_ = 0;
    _nj_buffer_byte_ptr_ = 0;
    _nj_buffer_r_byte_size_ = 0;
    _nj_buffer_read_flag_ = 0;
    return 0;
}

void njTexFclose(void)
{
    gdFsClose(file_ptr);
}

Sint32 njTexFread(void *buf, Uint32 size)
{
    Sint32 sector;
    Sint32 offset;
    Sint32 held;
    Sint32 count;
    Uint32 done;

    if (size == 0) {
        _nj_texerr_.texerr = NJD_TEXERR_FILELOAD;
        return -1;
    }
    if ((Uint32)_nj_file_max_size_ < _nj_file_byte_ptr_ + size) {
        _nj_texerr_.texerr = NJD_TEXERR_FILELOAD;
        return -1;
    }
    sector = _nj_file_byte_ptr_ / _nj_file_buffer_max_size;
    offset = _nj_file_byte_ptr_ - (sector * _nj_file_buffer_max_size);
    held = _nj_file_sect_ptr_ / _nj_buffer_sect_size_;
    if (sector != held || _nj_buffer_r_byte_size_ == 0) {
        sector *= _nj_buffer_sect_size_;
        if (gdFsSeek(file_ptr, sector, GDD_SEEK_SET)) {
            _nj_texerr_.texerr = NJD_TEXERR_FILELOAD;
            return -1;
        }
        if (gdFsRead(file_ptr, _nj_buffer_sect_size_, _nj_file_buffer_st)) {
            read_failed();
            return -1;
        }
        _nj_file_sect_ptr_ = sector;
    }
    if (offset != _nj_buffer_byte_ptr_) {
        _nj_buffer_byte_ptr_ = offset;
    }
    _nj_buffer_r_byte_size_ = _nj_file_buffer_max_size - _nj_buffer_byte_ptr_;
    /* @hack type: the buffer and buf addresses as integer sums, here and
     * below, which keeps the frame at 16 bytes (ADD #-16,R15 at 0x8c66ab42);
     * as pointer sums, SHC computes them before they are needed, and 167 of
     * 253 words differ */
    if ((Uint32)_nj_buffer_r_byte_size_ >= size) {
        njMemCopy(buf,
                  (void *)((Uint32)_nj_file_buffer_st + _nj_buffer_byte_ptr_),
                  size);
        _nj_buffer_byte_ptr_ += size;
        _nj_file_byte_ptr_ += size;
        _nj_buffer_r_byte_size_ =
            _nj_file_buffer_max_size - _nj_buffer_byte_ptr_;
    } else {
        done = 0;
        if (_nj_buffer_r_byte_size_ != 0) {
            njMemCopy(
                buf,
                (void *)((Uint32)_nj_file_buffer_st + _nj_buffer_byte_ptr_),
                _nj_buffer_r_byte_size_);
            done = _nj_buffer_r_byte_size_;
            _nj_file_byte_ptr_ += _nj_buffer_r_byte_size_;
        }
        /* @hack spelling: a test, then a do-while, which keeps
         * _nj_file_sect_ptr_'s address in R8 and offset in R9 (0x8c66ab6c,
         * 0x8c66ab72); as a while loop, 144 of 253 words differ */
        if (size > done) {
            do {
                if (size - done > (Uint32)_nj_file_buffer_max_size) {
                    count = (size - done) / _nj_file_buffer_max_size;
                    if (gdFsRead(file_ptr, count * _nj_buffer_sect_size_,
                                 (void *)((Uint32)buf + done))) {
                        read_failed();
                        return -1;
                    }
                    done += count * _nj_file_buffer_max_size;
                    _nj_file_byte_ptr_ += count * _nj_file_buffer_max_size;
                    _nj_file_sect_ptr_ += count * _nj_buffer_sect_size_;
                    _nj_buffer_byte_ptr_ = 0;
                } else {
                    count = size - done;
                    if (gdFsRead(file_ptr, _nj_buffer_sect_size_,
                                 _nj_file_buffer_st)) {
                        read_failed();
                        return -1;
                    }
                    njMemCopy((void *)((Uint32)buf + done), _nj_file_buffer_st,
                              count);
                    _nj_file_byte_ptr_ += count;
                    _nj_file_sect_ptr_ += _nj_buffer_sect_size_;
                    _nj_buffer_byte_ptr_ = count;
                    _nj_buffer_r_byte_size_ =
                        _nj_file_buffer_max_size - _nj_buffer_byte_ptr_;
                    done += count;
                }
            } while (size > done);
        }
    }
    return 1;
}

/* read as njTexFread does, except where the read runs to the end of the file.
 * there the bytes the buffer doesn't hold go directly into buf in whole
 * sectors, and the read's result goes unchecked. buf must then have room up to
 * the end of the file's last sector, where the next byte to read moves */
Sint32 njTexFread2(void *buf, Uint32 size)
{
    Sint32 sector;
    Sint32 offset;
    Sint32 held;
    Sint32 count;
    Sint32 bytes;
    Uint32 done;

    if (size == 0) {
        _nj_texerr_.texerr = NJD_TEXERR_FILELOAD;
        return -1;
    }
    if ((Uint32)_nj_file_max_size_ < _nj_file_byte_ptr_ + size) {
        _nj_texerr_.texerr = NJD_TEXERR_FILELOAD;
        return -1;
    }
    sector = _nj_file_byte_ptr_ / _nj_file_buffer_max_size;
    offset = _nj_file_byte_ptr_ - (sector * _nj_file_buffer_max_size);
    held = _nj_file_sect_ptr_ / _nj_buffer_sect_size_;
    if (sector != held || _nj_buffer_r_byte_size_ == 0) {
        sector *= _nj_buffer_sect_size_;
        if (gdFsSeek(file_ptr, sector, GDD_SEEK_SET)) {
            _nj_texerr_.texerr = NJD_TEXERR_FILELOAD;
            return -1;
        }
        if (gdFsRead(file_ptr, _nj_buffer_sect_size_, _nj_file_buffer_st)) {
            read_failed();
            return -1;
        }
        _nj_file_sect_ptr_ = sector;
    }
    if (offset != _nj_buffer_byte_ptr_) {
        _nj_buffer_byte_ptr_ = offset;
    }
    _nj_buffer_r_byte_size_ = _nj_file_buffer_max_size - _nj_buffer_byte_ptr_;
    /* @hack type: the buffer and buf addresses as integer sums, here and
     * below, which keeps the frame at 16 bytes (ADD #-16,R15 at 0x8c66ad86);
     * as pointer sums, 180 of 285 words differ */
    if ((Uint32)_nj_buffer_r_byte_size_ >= size) {
        njMemCopy(buf,
                  (void *)((Uint32)_nj_file_buffer_st + _nj_buffer_byte_ptr_),
                  size);
        _nj_buffer_byte_ptr_ += size;
        _nj_file_byte_ptr_ += size;
        _nj_buffer_r_byte_size_ =
            _nj_file_buffer_max_size - _nj_buffer_byte_ptr_;
    } else {
        done = 0;
        if (_nj_buffer_r_byte_size_ != 0) {
            njMemCopy(
                buf,
                (void *)((Uint32)_nj_file_buffer_st + _nj_buffer_byte_ptr_),
                _nj_buffer_r_byte_size_);
            done = _nj_buffer_r_byte_size_;
            _nj_file_byte_ptr_ += _nj_buffer_r_byte_size_;
        }
        if (size - done >= (Uint32)(_nj_file_max_size_ - _nj_file_byte_ptr_)) {
            count = (size - done + GDD_FS_SCTSIZE - 1) >> SECTOR_SHIFT;
            gdFsRead(file_ptr, count, (void *)((Uint32)buf + done));
            /* @hack spelling: the byte count through bytes, which shifts a
             * copy of count in R4 (MOV R14,R4 at 0x8c66aeee); shifted in
             * place, 70 of 285 words differ */
            bytes = count << SECTOR_SHIFT;
            _nj_file_byte_ptr_ += bytes;
            _nj_file_sect_ptr_ = 0;
            _nj_buffer_byte_ptr_ = 0;
        } else if (size > done) {
            /* @hack spelling: a do-while under a test of its condition, which
             * reaches that test with one BF (0x8c66aed6); as a while loop in
             * the else, 76 of 285 words differ */
            do {
                if (size - done > (Uint32)_nj_file_buffer_max_size) {
                    count = (size - done) / _nj_file_buffer_max_size;
                    if (gdFsRead(file_ptr, count * _nj_buffer_sect_size_,
                                 (void *)((Uint32)buf + done))) {
                        read_failed();
                        return -1;
                    }
                    done += count * _nj_file_buffer_max_size;
                    _nj_file_byte_ptr_ += count * _nj_file_buffer_max_size;
                    _nj_file_sect_ptr_ += count * _nj_buffer_sect_size_;
                    _nj_buffer_byte_ptr_ = 0;
                } else {
                    count = size - done;
                    if (gdFsRead(file_ptr, _nj_buffer_sect_size_,
                                 _nj_file_buffer_st)) {
                        read_failed();
                        return -1;
                    }
                    njMemCopy((void *)((Uint32)buf + done), _nj_file_buffer_st,
                              count);
                    _nj_file_byte_ptr_ += count;
                    _nj_file_sect_ptr_ += _nj_buffer_sect_size_;
                    _nj_buffer_byte_ptr_ = count;
                    _nj_buffer_r_byte_size_ =
                        _nj_file_buffer_max_size - _nj_buffer_byte_ptr_;
                    done += count;
                }
            } while (size > done);
        }
    }
    return 1;
}

void njTexFseek(Sint32 offset, Sint32 type)
{
    switch (type) {
    case GDD_SEEK_CUR:
        _nj_file_byte_ptr_ += offset;
        break;
    case GDD_SEEK_SET:
        _nj_file_byte_ptr_ = offset;
        break;
    case GDD_SEEK_END:
        _nj_file_byte_ptr_ = _nj_file_max_size_ - offset;
        break;
    }
}
