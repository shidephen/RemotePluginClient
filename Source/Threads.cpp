/*
  ==============================================================================

    Threads.cpp
    Created: 26 Aug 2016 11:26:54am
    Author:  shidephen

  ==============================================================================
*/

#include "Threads.h"
#include <mutex>

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
	bool willRun = true;
	while (willRun)
	{
		_cond.wait(lock, [&]() {
			return !_messages.empty();
		});

		
		for (auto it = _messages.begin(); it != _messages.end(); ++it)
			willRun = _client.ProcessMessage(*it);

		lock.unlock();
	}
}
