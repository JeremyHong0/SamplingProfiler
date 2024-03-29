// #include "Profiler.h"
// #include "Internal.h"

// int getValue() {
//     return 1;
// }
#include "Profiler.h"

#include <DbgHelp.h>
#include <intrin.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <mutex>
#include <map>
#include <vector>
#include <windows.h>

#pragma comment( lib, "dbghelp" )

#define MAXSAMPLENUM 5000

HANDLE Profiler::MainThread_;
DWORD Profiler::MainThreadId_;
std::thread* Profiler::WorkerThread_;

unsigned Profiler::SampleNumber_ = 0;
long long timer = 0;
long long tempTimer = 0;
std::chrono::time_point<std::chrono::steady_clock> timerStart;

std::map<ULONG64, SampleInfo> samples;
std::vector<std::map<ULONG64, SampleInfo>> SampleVector;

	static PSYMBOL_INFO GetSymbol(DWORD64 address, PSYMBOL_INFO buff) {
    PDWORD64 displacement = 0;
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)buff;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;
    SymFromAddr(GetCurrentProcess(), address, displacement, symbol);
    return symbol;
}

void Profiler::Init() {

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(GetCurrentProcess(), NULL, true)) {
	return;
    }
    timerStart = std::chrono::high_resolution_clock::now();
}

void Profiler::Start() {

    MainThreadId_ = GetCurrentThreadId();

    MainThread_ = OpenThread(
	THREAD_SUSPEND_RESUME |
	THREAD_GET_CONTEXT |
	THREAD_QUERY_INFORMATION,
	0,
	MainThreadId_
    );

    WorkerThread_ = new std::thread(Sample);
    WorkerThread_->detach();
}

void Profiler::Exit() {

	if(WorkerThread_ != nullptr)
		WorkerThread_->~thread();

    std::ofstream logFile("ProfileReport.csv");

    logFile << "Function,HitCount,Percentage,Time(ms)\n";

    const size_t size = samples.size();
	if(size > 0)
	{
		for(const auto& data : samples)
		{
            std::string name = data.second.SymbolName_;
            if (name.empty() || name.find(':') == std::string::npos)
                continue;
            std::string::iterator end_pos = std::remove(name.begin(), name.end(), ',');
            name.erase(end_pos, name.end());
            logFile << name << ",";
            unsigned hitCount = data.second.HitCount_;
            logFile << hitCount << ",";
            logFile << (double)hitCount / (double)MAXSAMPLENUM * 100.0 << ",";
            logFile << data.second.TimeDuration_ << '\n';
		}
	}

    logFile.close();
}

void Profiler::Sample() {

    while (SampleNumber_ < MAXSAMPLENUM)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        MainThread_ = OpenThread(
            THREAD_SUSPEND_RESUME |
            THREAD_GET_CONTEXT |
            THREAD_QUERY_INFORMATION,
            0,
            MainThreadId_
        );

        HRESULT result = 0;

        if(MainThread_ != 0)
            result = SuspendThread(MainThread_);

        if (result == 0xffffffff)
        {
            return;
        }

        CONTEXT context = {0};
        context.ContextFlags = WOW64_CONTEXT_i386 | CONTEXT_CONTROL;

        if (MainThread_ != 0)
            result = GetThreadContext(MainThread_, &context);

        if (FAILED(result))
        {
            return;
        }

        if (MainThread_ != 0)
            result = ResumeThread(MainThread_);

        if (FAILED(result))
        {
            return;
        }
        auto endTime = std::chrono::high_resolution_clock::now();

        ULONG64 buff[(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)]; 
        memset(buff, 0, sizeof(buff));

        const PSYMBOL_INFO symbols = GetSymbol(context.Rip, (PSYMBOL_INFO)buff);

        ++samples[symbols->Address].HitCount_;
        samples[symbols->Address].Addr_ = symbols->Address;
        samples[symbols->Address].SymbolName_ = symbols->Name;
        samples[symbols->Address].TimeDuration_ += std::chrono::duration< double, std::milli >(endTime - startTime).count();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        ++SampleNumber_;
    }
    std::cout << "Sampling Done!\n";
    WorkerThread_->~thread();

}
