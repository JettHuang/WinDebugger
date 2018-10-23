// \brief
//		Windows Process & Threads Helper Functions
//

#pragma once

#include <cstdint>
#include <tchar.h>
#include <string>
#include <vector>


// snap shot tool
class FSnapshotTool
{
public:
	enum SNAPFLAGS
	{
		SNAP_PROCESS = 0x01,
		SNAP_THREAD  = 0x02,
		SNAP_MODULE  = 0x04,
		SNAP_HEAP    = 0x08,
		SNAP_ALL     = (SNAP_PROCESS | SNAP_THREAD | SNAP_MODULE | SNAP_HEAP)
	};

	struct FSnapProcessInfo
	{
		uint32_t	ProcessId;
		uint32_t	ParentId;
		uint32_t	ThreadsCnt;
		std::wstring ExeFilename;
	};

	struct FSnapThreadInfo
	{
		uint32_t ThreadId;
		uint32_t ProcessId; // owner process
	};

	struct FSnapModuleInfo
	{
		void		   *BaseAddr;
		uint32_t		BaseSize; // bytes
		HMODULE			hModule;
		std::wstring	Name;
		std::wstring    ExeFilename;
	};

	#define kHeapBlock_Fixed	1
	#define kHeapBlock_Free		2
	#define kHeapBlock_Moveable	4
	struct FSnapHeapBlock
	{
		HANDLE    hHandle;
		void     *Address;
		uint32_t  BlockSize;
		uint32_t  Flags;
	};

	#define kDefaultHeap		1
	#define kSharedHeap			2
	struct FSnapHeapInfo
	{
		uint32_t	ProcessId;
		uint32_t	FlagValue; // 1-default heap, 2-shared heap

		std::vector<FSnapHeapBlock>  Blocks;
	};

	FSnapshotTool(uint32_t InProcessId, uint32_t InFlags);
	~FSnapshotTool();

	// get process
	bool GetProcessList(std::vector<FSnapProcessInfo> &OutProcesses) const;
	// get threads
	bool GetThreadList(std::vector<FSnapThreadInfo> &OutThreads) const;
	// get modules
	bool GetModuleList(std::vector<FSnapModuleInfo> &OutModules) const;
	// get heaps
	bool GetHeapList(std::vector<FSnapHeapInfo> &OutHeaps) const;

	static const TCHAR* GetHeapFlagsDesc(uint32_t InFlags);
	static const TCHAR* GetHeapBlockFlagsDesc(uint32_t InFlags);
protected:
	HANDLE		hSnapshotHandle;
	uint32_t	ProcessId;
};


