
#include "CrashHandler.h"
#pragma comment(lib, "dbghelp.lib")

LONG __stdcall CrashHandler::WriteDump(EXCEPTION_POINTERS* pException)
{
    MINIDUMP_EXCEPTION_INFORMATION exceptionParam;
    HANDLE hDumpFile = CreateFileA("crash_mini.dmp", GENERIC_WRITE, NULL, NULL,
                                   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    exceptionParam.ThreadId = GetCurrentThreadId();
    exceptionParam.ExceptionPointers = pException;
    exceptionParam.ClientPointers = FALSE;

    MINIDUMP_TYPE mDumpValue = (MINIDUMP_TYPE)(
	MiniDumpNormal | MiniDumpWithDataSegs |
	MiniDumpWithCodeSegs | MiniDumpWithIndirectlyReferencedMemory |
	MiniDumpWithUnloadedModules | MiniDumpWithFullMemory
	);

    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile,
	mDumpValue, &exceptionParam, NULL, NULL);

    CloseHandle(hDumpFile);

    return EXCEPTION_CONTINUE_SEARCH;
}

void CrashHandler::catchStackOverflow()
{
    ULONG stackOverflowSize = 16385; //16K + 1byte.
    SetThreadStackGuarantee(&stackOverflowSize);
}
