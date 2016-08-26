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

class ProcessingThread : Thread
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



#endif  // THREADS_H_INCLUDED
