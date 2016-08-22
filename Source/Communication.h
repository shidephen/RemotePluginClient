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
#include <windows.h>

class SystemSemaphore
{
public:
	enum AccessMode
	{
		Open,
		Create
	};

	SystemSemaphore() = delete;
	SystemSemaphore(const std::string& key, int initialValue = 0, AccessMode mode = Open);
	~SystemSemaphore();

	void Key(const std::string& key);
	std::string Key() const;

	void NativeKey(const std::string& nativeKey);
	std::string NativeKey() const;

	bool Acquire();
	bool Release(size_t count = 1);

private:
	static constexpr char* prefix = "qipc_systemsem_";
	std::string _key;
	std::string _native_key;

	AccessMode _mode;
	int _value;

	HANDLE _semaphore_handle;
	HANDLE _aquire_handle();
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
	SharedMemory(const std::string& key);
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

private:
	static constexpr char* prefix = "qipc_sharedmemory_";
	
	std::string _key;
	std::string _native_key;

	//noncopyable
	SharedMemory(const SharedMemory&) = delete;
	SharedMemory& operator=(const SharedMemory&) = delete;
};


#endif  // QTCOMMUNICATION_H_INCLUDED
