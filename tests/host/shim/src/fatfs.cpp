//
// fatfs.cpp
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

#include <fatfs/ff.h>

#include <circle/sched/scheduler.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fnmatch.h>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace
{
	// Function-local so tests can set it from static initialisers
	std::string& Root()
	{
		static std::string Path = ".";
		return Path;
	}

	struct TEntry
	{
		std::string Name;
		bool bDirectory;
		FSIZE_t nSize;
	};

	// "SD:soundfonts/x" or "SD:/soundfonts/x" -> <root>/SD/soundfonts/x
	std::string HostPath(const char* pPath)
	{
		std::string Path(pPath);
		const size_t nColon = Path.find(':');
		if (nColon == std::string::npos)
			return Root() + "/SD/" + Path;

		std::string Rest = Path.substr(nColon + 1);
		while (!Rest.empty() && Rest[0] == '/')
			Rest.erase(0, 1);
		return Root() + "/" + Path.substr(0, nColon) + "/" + Rest;
	}

	void FillInfo(FILINFO* fno, const TEntry& Entry)
	{
		memset(fno, 0, sizeof(*fno));
		strncpy(fno->fname, Entry.Name.c_str(), FF_MAX_LFN);
		fno->fattrib = Entry.bDirectory ? AM_DIR : AM_ARC;
		fno->fsize = Entry.nSize;
	}

	FRESULT NextMatch(DIR* dp, FILINFO* fno)
	{
		auto* pEntries = static_cast<std::vector<TEntry>*>(dp->pEntries);
		while (pEntries && dp->nIndex < pEntries->size())
		{
			const TEntry& Entry = (*pEntries)[dp->nIndex++];
			if (fnmatch(dp->Pattern, Entry.Name.c_str(), 0) == 0)
			{
				FillInfo(fno, Entry);
				return FR_OK;
			}
		}

		// End of directory: empty name
		memset(fno, 0, sizeof(*fno));
		return FR_OK;
	}
}

namespace FakeFatFs
{
	void SetRoot(const char* pRoot) { Root() = pRoot; }
	const char* GetRoot() { return Root().c_str(); }
}

FRESULT f_open(FIL* fp, const TCHAR* path, BYTE mode)
{
	const char* pMode = (mode & FA_WRITE) ? ((mode & (FA_CREATE_ALWAYS | FA_CREATE_NEW)) ? "wb" : "r+b") : "rb";
	fp->pFile = fopen(HostPath(path).c_str(), pMode);
	if (!fp->pFile)
		return FR_NO_FILE;

	fseek(fp->pFile, 0, SEEK_END);
	fp->fsize = ftell(fp->pFile);
	fseek(fp->pFile, 0, SEEK_SET);
	fp->fptr = 0;
	return FR_OK;
}

FRESULT f_close(FIL* fp)
{
	if (!fp->pFile)
		return FR_INVALID_OBJECT;
	fclose(fp->pFile);
	fp->pFile = nullptr;
	return FR_OK;
}

FRESULT f_read(FIL* fp, void* buff, UINT btr, UINT* br)
{
	// The SD/USB drivers yield while they wait (NO_BUSY_WAIT)
	CScheduler::Get()->Yield();

	const size_t nRead = fread(buff, 1, btr, fp->pFile);
	fp->fptr += nRead;
	if (br)
		*br = static_cast<UINT>(nRead);
	return ferror(fp->pFile) ? FR_DISK_ERR : FR_OK;
}

FRESULT f_write(FIL* fp, const void* buff, UINT btw, UINT* bw)
{
	const size_t nWritten = fwrite(buff, 1, btw, fp->pFile);
	fp->fptr += nWritten;
	fp->fsize = std::max(fp->fsize, fp->fptr);
	if (bw)
		*bw = static_cast<UINT>(nWritten);
	return nWritten == btw ? FR_OK : FR_DISK_ERR;
}

FRESULT f_lseek(FIL* fp, FSIZE_t ofs)
{
	if (fseek(fp->pFile, static_cast<long>(ofs), SEEK_SET) != 0)
		return FR_DISK_ERR;
	fp->fptr = ofs;
	return FR_OK;
}

FRESULT f_opendir(DIR* dp, const TCHAR* path)
{
	return f_findfirst(dp, nullptr, path, "*");
}

FRESULT f_closedir(DIR* dp)
{
	delete static_cast<std::vector<TEntry>*>(dp->pEntries);
	dp->pEntries = nullptr;
	return FR_OK;
}

FRESULT f_readdir(DIR* dp, FILINFO* fno)
{
	return NextMatch(dp, fno);
}

FRESULT f_findfirst(DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern)
{
	namespace fs = std::filesystem;

	dp->pEntries = nullptr;
	dp->nIndex = 0;
	strncpy(dp->Pattern, pattern, sizeof(dp->Pattern) - 1);
	dp->Pattern[sizeof(dp->Pattern) - 1] = '\0';

	std::error_code Error;
	fs::directory_iterator Iterator(HostPath(path), Error);
	if (Error)
		return FR_NO_PATH;

	// FatFs leaves directory order to the file system; sort for repeatable tests.
	// Callers never close a find, so keep the list small and accept the leak.
	auto* pEntries = new std::vector<TEntry>;
	for (const auto& Entry : Iterator)
		pEntries->push_back({Entry.path().filename().string(), Entry.is_directory(), Entry.is_directory() ? 0 : static_cast<FSIZE_t>(Entry.file_size())});
	std::sort(pEntries->begin(), pEntries->end(), [](const TEntry& A, const TEntry& B) { return A.Name < B.Name; });
	dp->pEntries = pEntries;

	if (!fno)
		return FR_OK;
	return NextMatch(dp, fno);
}

FRESULT f_findnext(DIR* dp, FILINFO* fno)
{
	return NextMatch(dp, fno);
}

FRESULT f_stat(const TCHAR* path, FILINFO* fno)
{
	struct stat Stat;
	if (stat(HostPath(path).c_str(), &Stat) != 0)
		return FR_NO_FILE;
	if (fno)
	{
		memset(fno, 0, sizeof(*fno));
		fno->fsize = Stat.st_size;
		fno->fattrib = S_ISDIR(Stat.st_mode) ? AM_DIR : AM_ARC;
	}
	return FR_OK;
}

FRESULT f_mount(FATFS* fs, const TCHAR* path, BYTE opt)
{
	return FR_OK;
}
