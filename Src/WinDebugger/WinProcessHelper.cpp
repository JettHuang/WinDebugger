// \brief
//		process information helper.
//

#include <Windows.h>
#include <tlhelp32.h>
#include "WinProcessHelper.h"


FSnapshotTool::FSnapshotTool(uint32_t InProcessId, uint32_t InFlags)
{
	DWORD dwFlags = 0;
	dwFlags |= InFlags & SNAP_PROCESS ? TH32CS_SNAPPROCESS : 0;
	dwFlags |= InFlags & SNAP_THREAD ? TH32CS_SNAPTHREAD : 0;
	dwFlags |= InFlags & SNAP_MODULE ? TH32CS_SNAPMODULE : 0;
	dwFlags |= InFlags & SNAP_HEAP ? TH32CS_SNAPHEAPLIST : 0;

	ProcessId = InProcessId;
	hSnapshotHandle = CreateToolhelp32Snapshot(dwFlags, InProcessId);
}

FSnapshotTool::~FSnapshotTool()
{
	if (hSnapshotHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hSnapshotHandle);
	}
}

// get process
bool FSnapshotTool::GetProcessList(std::vector<FSnapProcessInfo> &OutProcesses) const
{
	if (hSnapshotHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);

	if (!Process32First(hSnapshotHandle, &pe32))
	{
		return false;
	}

	do
	{
		FSnapProcessInfo Entry;

		Entry.ProcessId = pe32.th32ProcessID;
		Entry.ParentId = pe32.th32ParentProcessID;
		Entry.ThreadsCnt = pe32.cntThreads;
		Entry.ExeFilename = pe32.szExeFile;

		OutProcesses.push_back(Entry);
	} while (Process32Next(hSnapshotHandle, &pe32));

	return true;
}

// get threads
bool FSnapshotTool::GetThreadList(std::vector<FSnapThreadInfo> &OutThreads) const
{
	if (hSnapshotHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	THREADENTRY32 te32;
	te32.dwSize = sizeof(te32);

	if (!Thread32First(hSnapshotHandle, &te32))
	{
		return false;
	}

	do
	{
		if (ProcessId == -1 || ProcessId == te32.th32OwnerProcessID)
		{
			FSnapThreadInfo Entry;

			Entry.ThreadId = te32.th32ThreadID;
			Entry.ProcessId = te32.th32OwnerProcessID;
			OutThreads.push_back(Entry);
		}
	} while (Thread32Next(hSnapshotHandle, &te32));

	return true;
}

// get modules
bool FSnapshotTool::GetModuleList(std::vector<FSnapModuleInfo> &OutModules) const
{
	if (hSnapshotHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	MODULEENTRY32 me32;
	me32.dwSize = sizeof(me32);

	if (!Module32First(hSnapshotHandle, &me32))
	{
		return false;
	}

	do
	{
		FSnapModuleInfo Entry;

		Entry.BaseAddr = me32.modBaseAddr;
		Entry.BaseSize = me32.modBaseSize;
		Entry.hModule = me32.hModule;
		Entry.Name = me32.szModule;
		Entry.ExeFilename = me32.szExePath;

		OutModules.push_back(Entry);
	} while (Module32Next(hSnapshotHandle, &me32));

	return true;
}

// get heaps
bool FSnapshotTool::GetHeapList(std::vector<FSnapHeapInfo> &OutHeaps) const
{
	if (hSnapshotHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	HEAPLIST32  hl32;
	hl32.dwSize = sizeof(hl32);

	if (!Heap32ListFirst(hSnapshotHandle, &hl32))
	{
		return false;
	}

	OutHeaps.reserve(32);
	do
	{
		HEAPENTRY32 he32;
		ZeroMemory(&he32, sizeof(he32));
		he32.dwSize = sizeof(he32);

		if (Heap32First(&he32, ProcessId, hl32.th32HeapID))
		{
			OutHeaps.push_back(FSnapHeapInfo());
			FSnapHeapInfo &NewHeap = OutHeaps.back();

			NewHeap.ProcessId = hl32.th32ProcessID;
			NewHeap.FlagValue = hl32.dwFlags;
			do
			{
				FSnapHeapBlock Block;

				Block.hHandle = he32.hHandle;
				Block.Address = (void *)he32.dwAddress;
				Block.BlockSize = he32.dwBlockSize;
				Block.Flags = he32.dwFlags;

				NewHeap.Blocks.push_back(Block);
			} while (Heap32Next(&he32));
		}

	} while (Heap32ListNext(hSnapshotHandle, &hl32));

	return  true;
}

const TCHAR* FSnapshotTool::GetHeapFlagsDesc(uint32_t InFlags)
{
	if (InFlags == kDefaultHeap)
	{
		return TEXT("default");
	}
	else if (InFlags == kSharedHeap)
	{
		return TEXT("shared");
	}

	return TEXT("--");
}

const TCHAR* FSnapshotTool::GetHeapBlockFlagsDesc(uint32_t InFlags)
{
	if (InFlags == kHeapBlock_Fixed)
	{
		return TEXT("fixed");
	}
	else if (InFlags == kHeapBlock_Free)
	{
		return TEXT("free");
	}
	else if (InFlags == kHeapBlock_Moveable)
	{
		return TEXT("moveable");
	}

	return TEXT("--");
}
