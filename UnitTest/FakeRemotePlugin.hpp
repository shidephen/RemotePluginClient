#pragma once

#include "RemoteClient.h"
using namespace std;

class FakeRemotePlugin : public RemoteClientBase
{
public:
	FakeRemotePlugin(RemoteClientBase& client)
		: RemoteClientBase(client.In(), client.Out())
	{

	}

	FakeRemotePlugin(int32_t keyIn, int32_t keyOut)
		: RemoteClientBase(keyIn, keyOut)
	{
		_in = make_shared<shmFifo>();
	}
};