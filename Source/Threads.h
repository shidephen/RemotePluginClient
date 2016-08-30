/*
  ==============================================================================

    Threads.h
    Created: 26 Aug 2016 11:26:54am
    Author:  shidephen

  ==============================================================================
*/

#ifndef THREADS_H_INCLUDED
#define THREADS_H_INCLUDED

#include "JuceHeader.h"
#include <condition_variable>
#include <vector>
#include "RemoteClient.h"

// 音频处理线程
class ProcessingThread : public Thread
{
public:
	ProcessingThread(
		VstClientSlim&,
		std::mutex&,
		std::condition_variable&,
		std::vector<message>&);
	~ProcessingThread();
	virtual void run() override;

private:
	std::condition_variable& _cond;
	std::mutex& _mutex;
	VstClientSlim& _client;
	std::vector<message>& _messages;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessingThread);
};

// LMMS监控线程（如果LMMS退出，Host进程也退出）
class LMMSMonitorThread : public Thread 
{

public:
	explicit LMMSMonitorThread(volatile bool& exitCondition);

	virtual void run() override;

private:
	volatile bool& _exit_signal;
	HANDLE _parent;
};

#endif  // THREADS_H_INCLUDED
