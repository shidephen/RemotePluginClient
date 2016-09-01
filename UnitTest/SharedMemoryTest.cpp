#include <gtest/gtest.h>
#include "Communication.h"
#include <memory>
#include <future>
using namespace std;

TEST(SharedMemoryTest, TestSetKey)
{
	shared_ptr<SharedMemory> mem = make_shared<SharedMemory>("");
	ASSERT_EQ(mem->Key(), "");
	ASSERT_FALSE(mem->NativeKey().empty());

	// non-alphabet
	mem->Key("12");
	ASSERT_EQ(mem->Error(), ERROR_SUCCESS);
	ASSERT_EQ(mem->Key(), "12");
	ASSERT_EQ(
		mem->NativeKey(), 
		make_platform_key("qipc_sharedmemory_", "12"));

	// alphabet
	mem->Key("jazz");
	ASSERT_EQ(mem->Error(), ERROR_SUCCESS);
	ASSERT_EQ(mem->Key(), "jazz");
	ASSERT_EQ(
		mem->NativeKey(), 
		make_platform_key("qipc_sharedmemory_", "jazz"));
}

TEST(SharedMemoryTest, TestSwitchKey)
{
	const string old_key = "123";
	const string old_native_key
		= make_platform_key("qipc_sharedmemory_", old_key);
	const string new_key = "jazz";
	const string new_native_key
		= make_platform_key("qipc_sharedmemory_", new_key);

	shared_ptr<SharedMemory> mem = make_shared<SharedMemory>(old_key);
	
	HANDLE handle = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		false,
		old_native_key.c_str()
	);

	ASSERT_EQ(GetLastError(), ERROR_FILE_NOT_FOUND);
	ASSERT_EQ((int)handle, 0);

	mem->Create(512);

	handle = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		false,
		old_native_key.c_str()
	);

	ASSERT_GT((int)handle, 0);

	// switch to new key
	mem->Key(new_key);
	mem->Create(512);

	// old memory should be recycled

	handle = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		false,
		new_native_key.c_str()
	);

	ASSERT_GT((int)handle, 0);
	// when open success last error will not be set to ERROR_SUCESS
	//ASSERT_EQ(GetLastError(), ERROR_SUCCESS);

	CloseHandle(handle);
}

TEST(SharedMemoryTest, TestCreate)
{
	shared_ptr<SharedMemory> mem = make_shared<SharedMemory>("jacka");
	mem->Create(512);
	
	HANDLE handle = OpenFileMapping(
		FILE_MAP_ALL_ACCESS, 
		false, 
		mem->NativeKey().c_str());

	ASSERT_EQ(GetLastError(), ERROR_SUCCESS);
	ASSERT_GT((long)handle, 0);
	CloseHandle(handle);
}

TEST(SharedMemoryTest, TestAttach)
{
	const string key = "jackb";

	shared_ptr<SharedMemory> mem = make_shared<SharedMemory>(key);

	// need a semaphore to aquire
	shared_ptr<SystemSemaphore> sem
		= make_shared<SystemSemaphore>(key, 1, SystemSemaphore::Create);
	
	HANDLE handle = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		512,
		mem->NativeKey().c_str()
	);

	ASSERT_EQ(GetLastError(), ERROR_SUCCESS);

	bool is_success = mem->Attach();
	ASSERT_EQ(mem->Error(), ERROR_SUCCESS);
	ASSERT_TRUE(is_success);

	ASSERT_TRUE(mem->IsAttached());

	CloseHandle(handle);
}

TEST(SharedMemoryTest, TestDataWR)
{
	const string key = "jackc";
	// need a semaphore to aquire
	shared_ptr<SystemSemaphore> sem
		= make_shared<SystemSemaphore>(key, 1, SystemSemaphore::Create);

	shared_ptr<SharedMemory> mem = make_shared<SharedMemory>(key);

	HANDLE handle = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		512,
		mem->NativeKey().c_str()
	);

	ASSERT_EQ(GetLastError(), ERROR_SUCCESS);

	void* block = MapViewOfFile(
		handle,
		FILE_MAP_ALL_ACCESS, 
		0, 0, 0);

	ASSERT_NE(block, nullptr);
	ASSERT_EQ(GetLastError(), ERROR_SUCCESS);

	memset(block, 0, 512);

	int* int_block = (int*)block;

	int_block[0] = 5566;

	bool is_success = mem->Attach();
	ASSERT_TRUE(is_success);

	ASSERT_TRUE(mem->IsAttached());

	int_block = (int*)mem->Data();
	ASSERT_EQ(int_block[0], 5566);

	CloseHandle(handle);
}

TEST(SharedMemoryTest, TestDetach)
{
	const string key = "jackd";
	shared_ptr<SharedMemory> mem = make_shared<SharedMemory>(key);

	// need a semaphore to aquire
	shared_ptr<SystemSemaphore> sem
		= make_shared<SystemSemaphore>(key, 1, SystemSemaphore::Create);

	HANDLE handle = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		512,
		mem->NativeKey().c_str()
	);

	ASSERT_EQ(GetLastError(), ERROR_SUCCESS);

	bool is_success = mem->Attach();
	ASSERT_EQ(mem->Error(), ERROR_SUCCESS);
	ASSERT_TRUE(is_success);

	ASSERT_TRUE(mem->IsAttached());

	ASSERT_TRUE(mem->Detach());
	ASSERT_FALSE(mem->IsAttached());

	CloseHandle(handle);
}

TEST(SharedMemoryTest, TestDestory)
{
	const string key = "jacke";

	shared_ptr<SharedMemory> memory = make_shared<SharedMemory>(key);

	ASSERT_TRUE(memory->Create(512));

	memory = nullptr;

	memory = make_shared<SharedMemory>(key);
	ASSERT_FALSE(memory->Attach());
};