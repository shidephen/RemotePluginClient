/*
  ==============================================================================

    Communication.h
    Created: 19 Aug 2016 3:05:41pm
    Author:  shidephen

  ==============================================================================
*/

#ifndef QTCOMMUNICATION_H_INCLUDED
#define QTCOMMUNICATION_H_INCLUDED
#include <string>
#include <vector>
#include <windows.h>

std::string 
make_platform_key(
	const std::string& prefix, 
	const std::string& key);

class SystemSemaphore
{
public:
	enum AccessMode
	{
		Open,
		Create
	};

	SystemSemaphore() = delete;
	explicit SystemSemaphore(
		const std::string& key,
		int initialValue = 0,
		AccessMode mode = Create);
	~SystemSemaphore();

	void Key(
		const std::string& key, 
		int initValue = 0,
		AccessMode mode = Create);

	std::string Key() const;

	void NativeKey(const std::string& nativeKey);
	std::string NativeKey() const;

	bool Acquire();
	bool Release(size_t count = 1);

	DWORD Error() const { return _last_error; }

private:
	static const char* prefix;
	std::string _key;
	std::string _native_key;

	int _value;
	DWORD _last_error;

	HANDLE _semaphore_handle;
	HANDLE _aquire_handle(AccessMode mode = Create);
	void _release_handle();

	//noncopyable
	SystemSemaphore(const SystemSemaphore&) = delete;
	SystemSemaphore& operator = (const SystemSemaphore&) = delete;
};

class SharedMemory
{
public:
	enum AccessMode
	{
		ReadOnly,
		ReadWrite
	};

	SharedMemory() = default;
	explicit SharedMemory(const std::string& key);
	~SharedMemory();

	void Key(const std::string& key);
	std::string Key() const;

	void NativeKey(const std::string& nativeKey);
	std::string NativeKey() const;

	bool Create(size_t size, AccessMode mode = ReadWrite);
	size_t Size() const;

	bool Attach(AccessMode mode = ReadWrite);
	bool IsAttached() const;
	bool Detach();

	void* Data();
	const void* ConstData() const;

	bool Lock();
	bool Unlock();

	int Error() { return _last_error; }

private:
	static const char* prefix;
	
	std::string _key;
	std::string _native_key;
	bool _InitKey(SystemSemaphore::AccessMode semMode);

	void* _memory;
	size_t _size;

	HANDLE _handle;
	DWORD _last_error;
	HANDLE _aquire_handle();
	void _release_handle();

	SystemSemaphore _semaphore;
	bool _locked_by_me;

	//noncopyable
	SharedMemory(const SharedMemory&) = delete;
	SharedMemory& operator=(const SharedMemory&) = delete;
};

// sometimes we need to exchange bigger messages (e.g. for VST parameter dumps)
// so set a usable value here
const int SHM_FIFO_SIZE = 512 * 1024;

// implements a FIFO inside a shared memory segment
class shmFifo
{
	union sem32_t
	{
		int semKey;
		char fill[32];
	};
	struct shmData
	{
		sem32_t dataSem;	// semaphore for locking this
							// FIFO management data
		sem32_t messageSem;	// semaphore for incoming messages
		volatile int32_t startPtr; // current start of FIFO in memory
		volatile int32_t endPtr;   // current end of FIFO in memory
		char data[SHM_FIFO_SIZE];  // actual data
	};

public:
	shmFifo();
	shmFifo(int32_t);
	~shmFifo();

	bool isInvalid() const;
	void invalidate();

	bool isMaster() const;

	void lock();
	void unlock();

	void waitForMessage();
	void messageSent();

	int32_t readInt();
	void writeInt(const int32_t&);

	std::string readString();
	void writeString(const std::string&);

	bool messagesLeft();
	int shmKey() const;

private:
	static void fastMemCpy(void*, const void*, int);
	void read(void*, int);
	void write(const void*, int);

	volatile bool m_invalid;
	bool m_master;
	int32_t m_shmKey;
	SharedMemory m_shmObj;
	shmData* m_data;
	SystemSemaphore m_dataSem;
	SystemSemaphore m_messageSem;
	volatile LONG m_lockDepth;
};

struct message
{
	message() :
		id(0),
		data()
	{
	}

	message(const message & _m) :
		id(_m.id),
		data(_m.data)
	{
	}

	message(int _id) :
		id(_id),
		data()
	{
	}

	inline message & addString(const std::string & _s)
	{
		data.push_back(_s);
		return *this;
	}

	message & addInt(int _i)
	{
		data.push_back(std::to_string(_i));
		return *this;
	}

	message & addFloat(float _f)
	{
		data.push_back(std::to_string(_f));
		return *this;
	}

	inline std::string getString(int _p = 0) const
	{
		return data[_p];
	}

	inline int getInt(int _p = 0) const
	{
		return std::stoi(data[_p]);
	}

	inline float getFloat(int _p) const
	{
		return std::stof(data[_p]);
	}

	inline bool operator==(const message & _m) const
	{
		return(id == _m.id);
	}

	int id;

	std::vector<std::string> data;
};

#endif  // QTCOMMUNICATION_H_INCLUDED
