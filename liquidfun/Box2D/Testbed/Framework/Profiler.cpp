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

#define MAXSAMPLENUM 10000

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

    std::ofstream logFile("ProfileReport.csv");

    logFile << "Function, Hit Count, Percentage, Time(ms), Index\n";

    //const size_t size = samples.size();
    const size_t s = SampleVector.size();
	if(s > 0)
	{
		for(const auto& v : SampleVector)
		{
			for(const auto& data : v)
			{
                std::string name = data.second.SymbolName_;
                std::string::iterator end_pos = std::remove(name.begin(), name.end(), ',');
                name.erase(end_pos, name.end());
                logFile << name << ",";
                unsigned hitCount = data.second.HitCount_;
                logFile << hitCount << ",";
                logFile << (double)hitCount / (double)MAXSAMPLENUM * 100.0 << ",";
                logFile << data.second.TimeDuration_ << ",";
                logFile << data.second.index_ << '\n';
			}
            logFile << "-------------------------\n";
		}
	}
    /*if (size > 0)
    {
        for (const auto& data : samples)
        {
            logFile << data.second.SymbolName_ << ",";
            unsigned hitCount = data.second.HitCount_;
            logFile << hitCount << ",";
            logFile << (double)hitCount / (double)MAXSAMPLENUM * 100.0 << ",";
            logFile << data.second.TimeDuration_ << ",";
            logFile << data.second.index_ << '\n';
        }
    }*/

    logFile.close();
}

void Profiler::Sample() {

    while (SampleNumber_ < MAXSAMPLENUM)
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        auto timerEnd = std::chrono::high_resolution_clock::now();
        timer = std::chrono::duration_cast<std::chrono::seconds>(timerEnd - timerStart).count();

        

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

        /*for(int32_t count{1};;count++)
        {
	        if(!count%5)
	        {
		        
	        }
        }*/
        if (tempTimer != timer)
        {
            tempTimer = timer;
            std::cout << tempTimer << std::endl;
            SampleVector.push_back(samples);
            samples.clear();
        }

        ++samples[symbols->Address].HitCount_;
        samples[symbols->Address].Addr_ = symbols->Address;
        samples[symbols->Address].SymbolName_ = symbols->Name;
        samples[symbols->Address].TimeDuration_ += std::chrono::duration< double, std::milli >(endTime - startTime).count();
        samples[symbols->Address].index_ = symbols->TypeIndex;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        ++SampleNumber_;
    }
}
