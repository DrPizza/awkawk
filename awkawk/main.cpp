//  Copyright (C) 2006 Peter Bright
//  
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//  
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//  
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//  
//  Peter Bright <drpizza@quiscalusmexicanus.org>

#include "stdafx.h"

#include "awkawk.h"

#ifdef _DEBUG
#define SET_CRT_DEBUG_FIELD(a) _CrtSetDbgFlag((a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#define CLEAR_CRT_DEBUG_FIELD(a) _CrtSetDbgFlag(~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#define USES_MEMORY_CHECK	\
_CrtMemState before = {0}, after = {0}, difference = {0};\
_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);\
_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);\
_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);\
_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);\
_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);\
_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);\
SET_CRT_DEBUG_FIELD(_CRTDBG_LEAK_CHECK_DF);

//SET_CRT_DEBUG_FIELD(_CRTDBG_DELAY_FREE_MEM_DF);\
//SET_CRT_DEBUG_FIELD(_CRTDBG_CHECK_EVERY_16_DF)

#define MEM_CHK_BEFORE	\
_CrtMemCheckpoint(&before)

#define MEM_CHK_AFTER	\
_CrtMemCheckpoint(&after);\
do\
{\
	if(TRUE == _CrtMemDifference(&difference, &before, &after))\
	{\
		_CrtMemDumpStatistics(&difference);\
	}\
}\
while(0)

#else // _DEBUG
#define SET_CRT_DEBUG_FIELD(a) ((void) 0)
#define CLEAR_CRT_DEBUG_FIELD(a) ((void) 0)
#define USES_MEMORY_CHECK
#define MEM_CHK_BEFORE
#define MEM_CHK_AFTER
#endif // _DEBUG

LONG WINAPI exception_filter(EXCEPTION_POINTERS* exception_pointers)
{
	HMODULE dbghelp(::LoadLibraryW(L"dbghelp.dll"));
	if(dbghelp == NULL)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	ON_BLOCK_EXIT(&::FreeLibrary, dbghelp);
	typedef BOOL (WINAPI* MINIDUMPWRITEDUMP)(HANDLE process, DWORD pid, HANDLE file, MINIDUMP_TYPE dump_type, const MINIDUMP_EXCEPTION_INFORMATION*, const MINIDUMP_USER_STREAM_INFORMATION*, const MINIDUMP_CALLBACK_INFORMATION*);
	MINIDUMPWRITEDUMP mini_dump_write_dump(reinterpret_cast<MINIDUMPWRITEDUMP>(::GetProcAddress(dbghelp, "MiniDumpWriteDump")));
	if(mini_dump_write_dump == NULL)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	boost::scoped_array<wchar_t> temp_path(new wchar_t[_MAX_PATH + 1]);
	::GetTempPathW(_MAX_PATH, temp_path.get());
	std::wstringstream temp_file_name;
	std::time_t t(::time(NULL));
	temp_file_name << temp_path.get() << L"awkawk-" << std::hex << ::GetCurrentProcessId() << L"-" << utility::settimeformat(L"%Y%m%dT%H%M%SZ") << *std::gmtime(&t) << L".dmp";
	HANDLE dump_file(::CreateFileW(temp_file_name.str().c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL));
	if(dump_file == INVALID_HANDLE_VALUE)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	ON_BLOCK_EXIT(&::CloseHandle, dump_file);
	MINIDUMP_EXCEPTION_INFORMATION minidump_info = {0};
	minidump_info.ThreadId = ::GetCurrentThreadId();
	minidump_info.ExceptionPointers = exception_pointers;
	wdout << L"Writing crash dump to: " << temp_file_name.str() << std::endl;
	mini_dump_write_dump(::GetCurrentProcess(), ::GetCurrentProcessId(), dump_file, MiniDumpWithFullMemory, &minidump_info, NULL, NULL);
	return EXCEPTION_EXECUTE_HANDLER;
}

int real_main(int, wchar_t**)
{
	::SetUnhandledExceptionFilter(&exception_filter);
	::OleInitialize(NULL);
	//::InitCommonControls();
	ON_BLOCK_EXIT(&::OleUninitialize);
	awkawk p;
	p.create_ui(SW_RESTORE);
	return p.run_ui();
}

#define DEBUG_HEAP

#ifdef DEBUG

void show_deadlocks()
{
	std::set<std::pair<utility::lock_tracker::cs_sequence, utility::lock_tracker::cs_sequence> > deadlocks(utility::tracker.analyze_deadlocks());
	for(std::set<std::pair<utility::lock_tracker::cs_sequence, utility::lock_tracker::cs_sequence> >::const_iterator it(deadlocks.begin()), end(deadlocks.end()); it != end; ++it)
	{
		dout << "deadlock found" << std::endl;
		dout << "the sequences" << std::endl;
		for(std::list<utility::lock_tracker::lock_manipulation>::const_iterator it1(it->first.sequence.begin()), end1(it->first.sequence.end()); it1 != end1; ++it1)
		{
			dout << *it1 << std::endl;
		}
		dout << "and" << std::endl;
		for(std::list<utility::lock_tracker::lock_manipulation>::const_iterator it1(it->second.sequence.begin()), end1(it->second.sequence.end()); it1 != end1; ++it1)
		{
			dout << *it1 << std::endl;
		}
		dout << "can deadlock" << std::endl;
		dout << std::endl;
	}
}

void show_locks()
{
	for(std::map<DWORD, boost::shared_ptr<utility::lock_tracker::lock_info> >::const_iterator it(utility::tracker.info.begin()), end(utility::tracker.info.end()); it != end; ++it)
	{
		boost::shared_ptr<utility::lock_tracker::lock_info> info(it->second);
		if(info->current_sequence.empty())
		{
			continue;
		}
		dout << it->first << std::endl;
		for(std::list<utility::lock_tracker::lock_manipulation>::const_iterator lit(info->current_sequence.begin()), lend(info->current_sequence.end()); lit != lend; ++lit)
		{
			dout << "\t";
			switch(lit->operation)
			{
			case utility::lock_tracker::lock_manipulation::attempt:
				dout << "?";
				break;
			case utility::lock_tracker::lock_manipulation::acquire:
				dout << "+";
				break;
			case utility::lock_tracker::lock_manipulation::fail:
				dout << "!";
				break;
			case utility::lock_tracker::lock_manipulation::release:
				dout << "-";
				break;
			}
			dout << " " << lit->section << " ";
			utility::print_caller_info(dout, lit->return_address, lit->address_of_return_address);
			dout << std::endl;
		}
	}
}

BOOL WINAPI ctrl_c_handler(DWORD type)
{
	show_deadlocks();
	show_locks();
	switch(type)
	{
	case CTRL_BREAK_EVENT:
		return TRUE;
	default:
		return FALSE;
	}
}

int wmain(int argc, wchar_t* argv[])
{
	::SetConsoleCtrlHandler(&ctrl_c_handler, TRUE);
#ifdef DEBUG_HEAP
	USES_MEMORY_CHECK;
	MEM_CHK_BEFORE;
#endif
	int rv(real_main(argc, argv));
#ifdef DEBUG_HEAP
	MEM_CHK_AFTER;
#endif

#ifdef TRACK_LOCKS
	show_deadlocks();
#endif
	return rv;
}

#else

int __stdcall WinMain(HINSTANCE, HINSTANCE, char*, int cmdShow)
{
	int argc(0);
	wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
	ON_BLOCK_EXIT(&::LocalFree, argv);
	return real_main(argc, argv);
}

#endif
