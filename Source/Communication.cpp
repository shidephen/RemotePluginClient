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

using namespace std;

//===================== Platform key generator ================================
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
		if (!CryptAcquireContext(&_hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
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

//=============================================================================

HANDLE SystemSemaphore::_aquire_handle()
{
	if (_key.empty())
		return 0;

	if (_semaphore_handle <= 0)
		_semaphore_handle = 
		CreateSemaphoreEx(NULL, // security attribute
						_value, // initial value
						MAXLONG, // max value
						_native_key.c_str(), // filename
						0, //flags
						SEMAPHORE_ALL_ACCESS); // access

	return _semaphore_handle;
}

void SystemSemaphore::_release_handle()
{
	if (_semaphore_handle > 0)
		CloseHandle(_semaphore_handle);

	_semaphore_handle = 0;
}

//======================== SystemSemaphore methods ============================
SystemSemaphore::SystemSemaphore(const std::string& key,
								int initialValue /*= 0*/,
								AccessMode mode /*= Open*/)
{
	Key(key);
}

SystemSemaphore::~SystemSemaphore()
{
	_release_handle();
}

inline void SystemSemaphore::Key(const std::string& key)
{
	_key = key;
	_native_key = make_platform_key(prefix, key);
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

	if (WAIT_OBJECT_0 != WaitForSingleObjectEx(_semaphore_handle, INFINITE, FALSE))
		return false;

	return true;
}

bool SystemSemaphore::Release(size_t count /*= 1*/)
{
	if (0 == ReleaseSemaphore(_semaphore_handle, count, 0))
		return false;

	return true;
}
