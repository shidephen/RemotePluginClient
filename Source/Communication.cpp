/*
  ==============================================================================

    Communication.cpp
    Created: 19 Aug 2016 3:05:41pm
    Author:  shidephen

  ==============================================================================
*/

#include "Communication.h"
#include <windows.h>
#include <Wincrypt.h>
#include <regex>
#include <sstream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace std;

//===================== Platform key generator =================================
class CryptoProvider
{
public:
	CryptoProvider()
	{
		_init_provider();
	}

	~CryptoProvider()
	{
		_release_provider();
	}

	string get_sha1_hash(const string& data)
	{
		BYTE buffer[30];
		fill_n(buffer, 30, 0);
		DWORD data_size;

		if (!CryptHashData(_hHash, (const BYTE*)data.c_str(), data.length(), 0))
			return "";

		if (!CryptGetHashParam(_hHash, HP_HASHVAL, buffer, &data_size, 0))
			return "";

		ostringstream out;
		out.setf(ios::hex, ios::basefield);

		for_each(buffer, buffer + data_size, [&](BYTE ele) {
			out << (int)ele;
		});

		return out.str();
	}

private:
	HCRYPTPROV _hProv;
	HCRYPTHASH _hHash;

	bool _init_provider()
	{
		if (!CryptAcquireContext(&_hProv, NULL, NULL,
				PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
			return false;

		if (!CryptCreateHash(_hProv, CALG_SHA1, 0, 0, &_hHash))
			return false;

		return true;
	}

	void _release_provider()
	{
		if (_hHash > 0)
			CryptDestroyHash(_hHash);
		if (_hProv > 0)
			CryptReleaseContext(_hProv, 0);
	}
};

static CryptoProvider provider;

string make_platform_key(const string& prefix, const string& key)
{
	// Qt 将key中的非字母移除掉，并用整个key生成sha1的hash拼接在结尾
	regex key_pattern("[^A-Za-z]");
	string key_part = regex_replace(key, key_pattern, "");
	string key_hash = provider.get_sha1_hash(key);

	return prefix + key_part + key_hash;
}

//==============================================================================
//======================== SystemSemaphore methods =============================
const char* SystemSemaphore::prefix = "qipc_systemsem_";

HANDLE SystemSemaphore::_aquire_handle()
{
	if (_key.empty())
		return 0;

	if (_semaphore_handle <= 0)
		_semaphore_handle = 
		CreateSemaphoreEx(NULL,					// security attribute
						_value,					// initial value
						MAXLONG,				// max value
						_native_key.c_str(),	// filename
						0,						//flags
						SEMAPHORE_ALL_ACCESS);	// access

	return _semaphore_handle;
}

void SystemSemaphore::_release_handle()
{
	if (_semaphore_handle >= 0)
		CloseHandle(_semaphore_handle);

	_semaphore_handle = 0;
}

SystemSemaphore::SystemSemaphore(const std::string& key,
								int initialValue /*= 0*/,
								AccessMode mode /*= Open*/)
	: _semaphore_handle(0), _value(0), _key(""), _native_key("")
{
	Key(key, initialValue, mode);
}

SystemSemaphore::~SystemSemaphore()
{
	_release_handle();
}

inline void SystemSemaphore::Key(
	const std::string& key,
	int initValue /* = 0 */,
	AccessMode mode /* = Open */)
{
	_release_handle();

	_key = key;
	_native_key = make_platform_key(prefix, key);
	_value = initValue;

	_semaphore_handle = _aquire_handle();
}

inline std::string SystemSemaphore::Key() const
{
	return _key;
}

inline void SystemSemaphore::NativeKey(const std::string& nativeKey)
{
	_native_key = nativeKey;
	_semaphore_handle = _aquire_handle();
}

inline std::string SystemSemaphore::NativeKey() const
{
	return _native_key;
}

bool SystemSemaphore::Acquire()
{
	if (_semaphore_handle <= 0)
		return false;

	if (WAIT_OBJECT_0 != WaitForSingleObjectEx(
							_semaphore_handle,
							INFINITE,
							FALSE))
		return false;

	return true;
}

bool SystemSemaphore::Release(size_t count /*= 1*/)
{
	if (0 == ReleaseSemaphore(_semaphore_handle, count, 0))
		return false;

	return true;
}

//==============================================================================
//======================== SharedMemory methods ================================

const char* SharedMemory::prefix = "qipc_sharedmemory_";

bool SharedMemory::_InitKey()
{
	_release_handle();

	_semaphore.Key(_key, 1);

	return GetLastError() == ERROR_SUCCESS;
}

HANDLE SharedMemory::_aquire_handle()
{
	if (_handle <= 0)
		return 0;

	_handle = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, _native_key.c_str());
	_last_error = GetLastError();
	if (_handle <= 0)
		return 0;

	return _handle;
}

void SharedMemory::_release_handle()
{
	if (_handle > 0)
		CloseHandle(_handle);

	_last_error = GetLastError();
	_handle = 0;
}

SharedMemory::SharedMemory(const std::string& key)
	: _semaphore(key), _key(""), _handle(0), _last_error(ERROR_SUCCESS),
	_native_key(""), _size(0), _locked_by_me(false), _memory(nullptr)
{
	Key(key);
}

SharedMemory::~SharedMemory()
{
	Key("");
}

void SharedMemory::Key(const std::string& key)
{
	if (_key == key && make_platform_key(prefix, key) == _native_key)
		return;

	if (IsAttached())
		Detach();

	_release_handle();
	_key = key;
	_native_key = make_platform_key(prefix, key);
}

inline std::string SharedMemory::Key() const
{
	return _key;
}

void SharedMemory::NativeKey(const std::string& nativeKey)
{
	if (_native_key == nativeKey && _key.empty())
		return;

	if (IsAttached())
		Detach();

	_release_handle();
	_key = "";
	_native_key = nativeKey;
}

inline std::string SharedMemory::NativeKey() const
{
	return _native_key;
}

bool SharedMemory::Create(size_t size, AccessMode mode /*= ReadWrite*/)
{
	if (_native_key.empty() || _handle > 0)
		return false;

	if (!_key.empty() && !_InitKey())
		return false;

	if (!_key.empty() && !_semaphore.Acquire())
		return false;

	_handle = CreateFileMapping(
		INVALID_HANDLE_VALUE,	// file
		NULL,					// security attributes
		PAGE_READWRITE,			// page protect
		0,						// max size high
		size,					// max size low
		_native_key.c_str());	// name

	_last_error = GetLastError();
	if (_last_error == ERROR_ALREADY_EXISTS && _handle <= 0)
	{
		_semaphore.Release();
		return false;
	}

	_semaphore.Release();

	return true;
}

inline size_t SharedMemory::Size() const
{
	return _size;
}

bool SharedMemory::Attach(AccessMode mode /*= ReadWrite*/)
{
	if (IsAttached() || !_InitKey())
		return false;

	_handle = _aquire_handle();

	if (_handle <= 0)
		return false;

	if (!_key.empty() && !_semaphore.Acquire())
		return false;

	int permissions = (mode == ReadOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS);

	_memory = MapViewOfFile(_handle, permissions, 0, 0, 0);

	if (NULL == _memory)
	{
		_last_error = GetLastError();
		_release_handle();
		_semaphore.Release();
		return false;
	}

	MEMORY_BASIC_INFORMATION info;
	if (!VirtualQuery(_memory, &info, sizeof(info)))
	{
		_semaphore.Release();
		return false;
	}

	_size = info.RegionSize;
	_semaphore.Release();
	return true;
}

bool SharedMemory::IsAttached() const
{
	return _memory != NULL;
}

bool SharedMemory::Detach()
{
	if (!IsAttached())
		return false;

	if (!_key.empty() && !_semaphore.Acquire())
		return false;

	if (!UnmapViewOfFile(_memory))
	{
		_last_error = GetLastError();
		_semaphore.Release();
		return false;
	}

	_memory = NULL;
	_size = 0;

	_release_handle();

	return _semaphore.Release();
}

inline void* SharedMemory::Data()
{
	return _memory;
}

inline const void* SharedMemory::ConstData() const
{
	return _memory;
}

bool SharedMemory::Lock()
{
	if (_locked_by_me)
		return true;

	if (_semaphore.Acquire())
	{
		_locked_by_me = true;
		return true;
	}

	_last_error = GetLastError();
	return false;
}

bool SharedMemory::Unlock()
{
	if (!_locked_by_me)
		return false;

	_locked_by_me = false;
	if (_semaphore.Release())
		return true;

	_last_error = GetLastError();
	return false;
}

//==============================================================================

//============================ shmFifo methods =================================
// constructor for master-side
shmFifo::shmFifo() :
	m_invalid(false),
	m_master(true),
	m_shmKey(0),
	m_shmObj(""),
	m_data(NULL),
	m_dataSem(""),
	m_messageSem(""),
	m_lockDepth(0)
{
	do
	{
		m_shmObj.Key(to_string(++m_shmKey));
		m_shmObj.Create(sizeof(shmData));
	} while (m_shmObj.Error() != ERROR_SUCCESS);

	m_data = (shmData *)m_shmObj.Data();

	assert(m_data != NULL);
	m_data->startPtr = m_data->endPtr = 0;
	static int k = 0;
	m_data->dataSem.semKey = (GetCurrentProcessId() << 10) + ++k;
	m_data->messageSem.semKey = (GetCurrentProcessId() << 10) + ++k;
	m_dataSem.Key(to_string(m_data->dataSem.semKey),
		1, SystemSemaphore::Create);
	m_messageSem.Key(to_string(
		m_data->messageSem.semKey),
		0, SystemSemaphore::Create);
}

// constructor for remote-/client-side - use _shm_key for making up
// the connection to master
shmFifo::shmFifo(int32_t _shm_key) :
	m_invalid(false),
	m_master(false),
	m_shmKey(0),
	m_shmObj(to_string(_shm_key)),
	m_data(NULL),
	m_dataSem(""),
	m_messageSem(""),
	m_lockDepth(0)
{
	if (m_shmObj.Attach())
	{
		m_data = (shmData *)m_shmObj.Data();
	}
	assert(m_data != NULL);
	m_dataSem.Key(to_string(m_data->dataSem.semKey));
	m_messageSem.Key(to_string(
		m_data->messageSem.semKey));
}

shmFifo::~shmFifo()
{

}

bool shmFifo::isInvalid() const
{
	return m_invalid;
}

void shmFifo::invalidate()
{
	m_invalid = true;
}

// do we act as master (i.e. not as remote-process?)
inline bool shmFifo::isMaster() const
{
	return m_master;
}

// recursive lock
void shmFifo::lock()
{
	if (!isInvalid() && InterlockedAdd(&m_lockDepth, 1) == 1)
	{
		m_dataSem.Acquire();
	}
}

// recursive unlock
void shmFifo::unlock()
{
	if (InterlockedAddAcquire(&m_lockDepth, -1) <= 0)
	{
		m_dataSem.Release();
	}
}

// wait until message-semaphore is available
void shmFifo::waitForMessage()
{
	if (!isInvalid())
	{
		m_messageSem.Acquire();
	}
}

// increase message-semaphore
void shmFifo::messageSent()
{
	m_messageSem.Release();
}


int32_t shmFifo::readInt()
{
	int32_t i;
	read(&i, sizeof(i));
	return i;
}

void shmFifo::writeInt(const int32_t & _i)
{
	write(&_i, sizeof(_i));
}

std::string shmFifo::readString()
{
	const int len = readInt();
	if (len)
	{
		char * sc = new char[len + 1];
		read(sc, len);
		sc[len] = 0;
		std::string s(sc);
		delete[] sc;
		return s;
	}
	return std::string();
}


void shmFifo::writeString(const std::string & _s)
{
	const int len = _s.size();
	writeInt(len);
	write(_s.c_str(), len);
}


bool shmFifo::messagesLeft()
{
	if (isInvalid())
	{
		return false;
	}
	lock();
	const bool empty = (m_data->startPtr == m_data->endPtr);
	unlock();
	return !empty;
}


int shmFifo::shmKey() const
{
	return m_shmKey;
}

void shmFifo::fastMemCpy(void * _dest, const void * _src,
	const int _len)
{
	// calling memcpy() for just an integer is obsolete overhead
	if (_len == 4)
	{
		*((int32_t *)_dest) = *((int32_t *)_src);
	}
	else
	{
		memcpy(_dest, _src, _len);
	}
}

void shmFifo::read(void * _buf, int _len)
{
	if (isInvalid())
	{
		memset(_buf, 0, _len);
		return;
	}
	lock();
	while (isInvalid() == false &&
		_len > m_data->endPtr - m_data->startPtr)
	{
		unlock();
		this_thread::sleep_for(chrono::nanoseconds{ 5 });
		lock();
	}
	fastMemCpy(_buf, m_data->data + m_data->startPtr, _len);
	m_data->startPtr += _len;
	// nothing left?
	if (m_data->startPtr == m_data->endPtr)
	{
		// then reset to 0
		m_data->startPtr = m_data->endPtr = 0;
	}
	unlock();
}

void shmFifo::write(const void * _buf, int _len)
{
	if (isInvalid() || _len > SHM_FIFO_SIZE)
	{
		return;
	}
	lock();
	while (_len > SHM_FIFO_SIZE - m_data->endPtr)
	{
		// if no space is left, try to move data to front
		if (m_data->startPtr > 0)
		{
			memmove(m_data->data,
				m_data->data + m_data->startPtr,
				m_data->endPtr - m_data->startPtr);
			m_data->endPtr = m_data->endPtr -
				m_data->startPtr;
			m_data->startPtr = 0;
		}
		unlock();
		this_thread::sleep_for(chrono::nanoseconds{ 5 });
		lock();
	}
	fastMemCpy(m_data->data + m_data->endPtr, _buf, _len);
	m_data->endPtr += _len;
	unlock();
}

//==============================================================================
