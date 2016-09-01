/*
  ==============================================================================

    RemoteClientBase.cpp
    Created: 1 Sep 2016 9:59:53am
    Author:  shidephen

  ==============================================================================
*/

#include "RemoteClient.h"
#include <string>

RemoteClientBase::
RemoteClientBase(int32_t key_in, int32_t key_out)
	:_shmObj(""), _shm(nullptr),
	 _key_in(key_in), _key_out(key_out)
{
	_in = std::make_shared<shmFifo>(key_in);
	_out = std::make_shared<shmFifo>(key_out);
}

RemoteClientBase::
RemoteClientBase(
	std::shared_ptr<shmFifo>& shm_in,
	std::shared_ptr<shmFifo>& shm_out)
	:_shmObj(""), _shm(nullptr),
	_in(shm_in), _out(shm_out)
{
	
}

RemoteClientBase::RemoteClientBase()
	: _shmObj(""), _shm(nullptr)
{
	_in = std::make_shared<shmFifo>();
	_out = std::make_shared<shmFifo>();
	_key_in = _in->shmKey();
	_key_out = _out->shmKey();
}

RemoteClientBase::~RemoteClientBase()
{
	SendMessage(IdQuit);
	_shmObj.Detach();
}

int RemoteClientBase::SendMessage(const message& m)
{
	_out->lock();
	_out->writeInt(m.id);
	_out->writeInt(m.data.size());
	size_t written = 8;

	for (auto i = m.data.begin(); i != m.data.end(); i++)
	{
		_out->writeString(*i);
		// contains a int represent the length of string.
		written += 4 + i->size();
	}

	_out->unlock();
	_out->messageSent();

	return written;
}

message RemoteClientBase::RecieveMessage()
{
	_in->waitForMessage();
	_in->lock();

	message m;
	m.id = _in->readInt();
	size_t s = _in->readInt();
	for (; s-- > 0; m.data.push_back(_in->readString()));

	_in->unlock();

	return m;
}

message
RemoteClientBase::WaitForMessage(
	const message& m,
	bool busyWaiting /*= false*/)
{
	while (_in->isInvalid())
	{
		message msg = RecieveMessage();
		ProcessMessage(msg);
		if (msg.id == m.id)
			return msg;
		else if (msg.id == IdUndefined)
			return msg;
	}

	return message();
}

message RemoteClientBase::FetchAndProcessNextMessage()
{
	message m = RecieveMessage();
	ProcessMessage(m);
	return m;
}

void RemoteClientBase::Invalidate()
{
	_in->invalidate();
	_out->invalidate();
	_in->messageSent();
}

bool RemoteClientBase::ProcessMessage(const message& m)
{
	switch (m.id)
	{
	case IdUndefined:
		return false;
	case IdQuit:
		return false;
	case IdChangeSharedMemoryKey:
		_SetShmKey(m.getInt(0));
		break;
	default:
		break;
	}
	return true;
}

void RemoteClientBase::_SetShmKey(int32_t key)
{
	_shmObj.Key(std::to_string(key));
	if (_shmObj.Attach() && _shmObj.Error() == ERROR_SUCCESS)
		_shm = (float*)_shmObj.Data();
	else
		_shm = nullptr;
}
