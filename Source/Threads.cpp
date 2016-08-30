/*
  ==============================================================================

    Threads.cpp
    Created: 26 Aug 2016 11:26:54am
    Author:  shidephen

  ==============================================================================
*/

#include "Threads.h"
#include <TlHelp32.h>
#include <mutex>
#include <cassert>
#include <exception>
#include <iostream>

using namespace std;

ProcessingThread::ProcessingThread(
	VstClientSlim& client, 
	std::mutex& mu, 
	std::condition_variable& cond, 
	std::vector<message>& queue)
	: Thread("ProcessingThread"),
	_client(client),
	_mutex(mu),
	_cond(cond),
	_messages(queue)
{

}

ProcessingThread::~ProcessingThread()
{

}

void ProcessingThread::run()
{
	unique_lock<mutex> lock(_mutex);

	while (!threadShouldExit())
	{
		_cond.wait(lock, [&]() {
			return !_messages.empty();
		});

		
		for (auto it = _messages.begin(); it != _messages.end(); ++it)
			_client.ProcessMessage(*it);

		_messages.clear();
		lock.unlock();
	}
}


DWORD GetParentProcessID(unsigned int currentPid)
{
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32W pe;
	pe.dwSize = sizeof(pe);
	if (!Process32FirstW(snap, &pe))
		return 0;

	while (Process32NextW(snap, &pe))
	{
		if (pe.th32ProcessID == currentPid)
			break;
	}
	CloseHandle(snap);

	return pe.th32ParentProcessID;
}



LMMSMonitorThread::LMMSMonitorThread(volatile bool& exitCondition) 
	: _exit_signal(exitCondition),
	Thread("LMMSMonitorThread")
{
	DWORD lmms_pid = GetParentProcessID(GetCurrentProcessId());
	cout << "LMMS pid: " << lmms_pid << endl;

	_parent = OpenProcess(SYNCHRONIZE, true, lmms_pid);
	if (_parent <= 0)
	{
		DWORD dwErrNo = GetLastError();
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dwErrNo,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);

		throw new exception((const char*)lpMsgBuf);
	}
}

void LMMSMonitorThread::run()
{
	DWORD state;
	do 
	{
		state = WaitForSingleObject(_parent, 1000);
	} while (!threadShouldExit() && state == WAIT_TIMEOUT);
	
	if (state == WAIT_OBJECT_0)
		_exit_signal = true;
}
