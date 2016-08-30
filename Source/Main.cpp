/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "RemoteClient.h"
#include "Threads.h"
#include <iostream>
#include <memory>
#include <string>
#include <sstream>

using namespace std;

std::mutex mu;
std::condition_variable cond;
std::vector<message> queue;
volatile bool will_exit = false;

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nShowCmd);

	ScopedJuceInitialiser_GUI initializer;

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	istringstream cmd_in(lpCmdLine);
	string param;
	int num_in, num_out;

	if (!getline(cmd_in, param, ' '))
	{
		cout << "Read 1st param error." << endl;
		return EXIT_FAILURE;
	}
	num_in = stoi(param);

	if (!getline(cmd_in, param, ' '))
	{
		cout << "Read 2nd param error." << endl;
		return EXIT_FAILURE;
	}
	num_out = stoi(param);

	ScopedPointer<VstClientSlim> client = new VstClientSlim(num_in, num_out);

	LMMSMonitorThread monitor_thread(will_exit);
	ProcessingThread process_thread(*client, mu, cond, queue);
	monitor_thread.startThread();
	process_thread.startThread(7);

	while (will_exit)
	{
		message m = client->RecieveMessage();
		if (m.id == IdStartProcessing)
		{
			// this will notify processing thread that 
			// there are data to process.
			lock_guard<mutex> lock(mu);
			queue.push_back(m);
			cond.notify_one();
		}
		else
		{
			will_exit = client->ProcessMessage(m);
		}
	}

	process_thread.signalThreadShouldExit();
	monitor_thread.signalThreadShouldExit();
	monitor_thread.waitForThreadToExit(1000);
	process_thread.waitForThreadToExit(10000); // wait for 10s to exit.

	return EXIT_SUCCESS;
}