//
// ff.h
//
// mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
// Copyright (C) 2026 pim-32 contributors
//
// This file is part of mt32-pi.
//
// mt32-pi is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// mt32-pi is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// mt32-pi. If not, see <http://www.gnu.org/licenses/>.
//

// Fake FatFs on the host file system (host test harness)
//
// Drive prefixes map to folders under a root the test sets:
// "SD:soundfonts/a.sf2" and "SD:/soundfonts/a.sf2" both become
// <root>/SD/soundfonts/a.sf2. Like Circle with NO_BUSY_WAIT, every f_read()
// yields to the scheduler.
#ifndef _fatfs_ff_h
#define _fatfs_ff_h

#include <cstdio>
#include <cstdint>

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;
typedef char TCHAR;
typedef QWORD FSIZE_t;
typedef QWORD LBA_t;

typedef enum
{
	FR_OK = 0,
	FR_DISK_ERR,
	FR_INT_ERR,
	FR_NOT_READY,
	FR_NO_FILE,
	FR_NO_PATH,
	FR_INVALID_NAME,
	FR_DENIED,
	FR_EXIST,
	FR_INVALID_OBJECT,
	FR_WRITE_PROTECTED,
	FR_INVALID_DRIVE,
	FR_NOT_ENABLED,
	FR_NO_FILESYSTEM,
	FR_MKFS_ABORTED,
	FR_TIMEOUT,
	FR_LOCKED,
	FR_NOT_ENOUGH_CORE,
	FR_TOO_MANY_OPEN_FILES,
	FR_INVALID_PARAMETER
} FRESULT;

#define FA_READ 0x01
#define FA_WRITE 0x02
#define FA_OPEN_EXISTING 0x00
#define FA_CREATE_NEW 0x04
#define FA_CREATE_ALWAYS 0x08
#define FA_OPEN_ALWAYS 0x10
#define FA_OPEN_APPEND 0x30

#define AM_RDO 0x01
#define AM_HID 0x02
#define AM_SYS 0x04
#define AM_DIR 0x10
#define AM_ARC 0x20

#define FF_MAX_LFN 255

struct FATFS
{
	int nDummy;
};

struct FIL
{
	FILE* pFile;
	FSIZE_t fptr;
	FSIZE_t fsize;
};

struct DIR
{
	void* pEntries;
	size_t nIndex;
	char Pattern[64];
};

struct FILINFO
{
	FSIZE_t fsize;
	WORD fdate;
	WORD ftime;
	BYTE fattrib;
	TCHAR altname[13];
	TCHAR fname[FF_MAX_LFN + 1];
};

#define f_size(fp) ((fp)->fsize)
#define f_tell(fp) ((fp)->fptr)
#define f_eof(fp) ((int)((fp)->fptr == (fp)->fsize))
#define f_unmount(path) f_mount(0, path, 0)

FRESULT f_open(FIL* fp, const TCHAR* path, BYTE mode);
FRESULT f_close(FIL* fp);
FRESULT f_read(FIL* fp, void* buff, UINT btr, UINT* br);
FRESULT f_write(FIL* fp, const void* buff, UINT btw, UINT* bw);
FRESULT f_lseek(FIL* fp, FSIZE_t ofs);
FRESULT f_opendir(DIR* dp, const TCHAR* path);
FRESULT f_closedir(DIR* dp);
FRESULT f_readdir(DIR* dp, FILINFO* fno);
FRESULT f_findfirst(DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern);
FRESULT f_findnext(DIR* dp, FILINFO* fno);
FRESULT f_stat(const TCHAR* path, FILINFO* fno);
FRESULT f_mount(FATFS* fs, const TCHAR* path, BYTE opt);

namespace FakeFatFs
{
	// Folder that holds one subfolder per drive ("SD", "USB")
	void SetRoot(const char* pRoot);
	const char* GetRoot();
}

#endif
