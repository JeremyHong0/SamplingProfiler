#pragma once
#include <Windows.h>
#include <DbgHelp.h>

class CrashHandler
{
public:
    static LONG WINAPI WriteDump(EXCEPTION_POINTERS* pException);
    static void catchStackOverflow();
};