#include <gtest/gtest.h>
#include "Communication.h"
#include <memory>
#include <chrono>
#include <future>
#include <thread>
using namespace std;

TEST(SemaphoreTest, TestSetKey)
{
	shared_ptr<SystemSemaphore> sem = make_shared<SystemSemaphore>("");

	// default
	ASSERT_EQ(sem->Key(), "");
	ASSERT_EQ(sem->Error(), ERROR_SUCCESS);
	
	// non-alphabet
	sem->Key("12");
	ASSERT_EQ(sem->Key(), "12");
	ASSERT_EQ(sem->Error(), ERROR_SUCCESS);
	ASSERT_EQ(
		sem->NativeKey(),
		make_platform_key("qipc_systemsem_", "12"));

	// alphabet
	sem->Key("jazz");
	ASSERT_EQ(sem->Key(), "jazz");
	ASSERT_EQ(sem->Error(), ERROR_SUCCESS);
	ASSERT_EQ(
		sem->NativeKey(),
		make_platform_key("qipc_systemsem_", "jazz"));
}

TEST(SemaphoreTest, TestCreate)
{
	const string key = "a";
	const string native_key = make_platform_key("qipc_systemsem_", key);

	HANDLE handle = OpenSemaphore(
		SEMAPHORE_ALL_ACCESS,
		FALSE,
		native_key.c_str()
	);

	ASSERT_EQ((int)handle, 0);

	shared_ptr<SystemSemaphore> sem 
		= make_shared<SystemSemaphore>(key, 0, SystemSemaphore::Create);


	handle = OpenSemaphore(
		SEMAPHORE_ALL_ACCESS,
		FALSE,
		native_key.c_str()
	);

	ASSERT_GT((int)handle, 0);
}

TEST(SemaphoreTest, TestAquirement)
{
	const string key = "ab";
	const string native_key = make_platform_key("qipc_systemsem_", key);

	HANDLE handle = CreateSemaphore(
		NULL,
		1,
		MAXLONG,
		native_key.c_str()
	);

	shared_ptr<SystemSemaphore> sem 
		= make_shared<SystemSemaphore>(key, 0, SystemSemaphore::Open);
	
	auto handler = std::async([&] {
		sem->Acquire();
	});

	// already signal one
	ASSERT_EQ(handler.wait_for(chrono::seconds{ 1 }), future_status::ready);

	handler = std::async([&] {
		sem->Acquire();
	});

	// none remained
	EXPECT_EQ(handler.wait_for(chrono::seconds{ 1 }), future_status::timeout);
	// future created by async will block when distoryed.
	ReleaseSemaphore(handle, 1, NULL);
	CloseHandle(handle);
}

TEST(SemaphoreTest, TestRelease)
{
	const string key = "abc";
	const string native_key = make_platform_key("qipc_systemsem_", key);

	HANDLE handle = CreateSemaphore(
		NULL,
		0,
		MAXLONG,
		native_key.c_str()
	);

	shared_ptr<SystemSemaphore> sem
		= make_shared<SystemSemaphore>(key, 0, SystemSemaphore::Open);

	auto handler = std::async([&] {
		sem->Acquire();
	});

	// has no semaphore
	ASSERT_EQ(handler.wait_for(chrono::seconds{ 1 }), future_status::timeout);

	// signaled
	ASSERT_TRUE(sem->Release(1));
	ASSERT_EQ(handler.wait_for(chrono::seconds{ 1 }), future_status::ready);

	CloseHandle(handle);
}
