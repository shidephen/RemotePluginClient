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
	ASSERT_EQ(mem->Key(), "12");
	string key = mem->NativeKey();
	ASSERT_TRUE(key.find_first_of("qipc_sharedmemory_") != key.npos);

	// alphabet
	mem->Key("jazz");
	ASSERT_EQ(mem->Key(), "jazz");
	key = mem->NativeKey();
	ASSERT_TRUE(key.find_first_of("qipc_sharedmemory_jazz") != key.npos);
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
	CloseHandle(handle);
}

TEST(SharedMemoryTest, TestAttach)
{
	const string key = "jackb";
	// need a semaphore to aquire
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

	bool is_success = mem->Attach();
	ASSERT_EQ(mem->Error(), ERROR_SUCCESS);
	ASSERT_TRUE(is_success);

	ASSERT_TRUE(mem->IsAttached());
}

TEST(SharedMemoryTest, TestDataWR)
{
	const string key = "jackc";
	// need a semaphore to aquire
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
	ASSERT_EQ(mem->Error(), ERROR_SUCCESS);
	ASSERT_TRUE(is_success);

	ASSERT_TRUE(mem->IsAttached());

	int_block = (int*)mem->Data();
	ASSERT_EQ(int_block[0], 5566);
}

TEST(SharedMemoryTest, TestDetach)
{
	const string key = "jackd";
	// need a semaphore to aquire
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

	bool is_success = mem->Attach();
	ASSERT_EQ(mem->Error(), ERROR_SUCCESS);
	ASSERT_TRUE(is_success);

	ASSERT_TRUE(mem->IsAttached());

	ASSERT_TRUE(mem->Detach());
	ASSERT_FALSE(mem->IsAttached());
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