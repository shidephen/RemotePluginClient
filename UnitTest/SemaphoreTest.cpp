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
	string key = sem->NativeKey();
	ASSERT_TRUE(key.find_first_of("qipc_systemsem_") != key.npos);

	// alphabet
	sem->Key("jazz");
	ASSERT_EQ(sem->Key(), "jazz");
	ASSERT_EQ(sem->Error(), ERROR_SUCCESS);
	key = sem->NativeKey();
	ASSERT_TRUE(key.find_first_of("qipc_systemsem_jazz") != key.npos);
}

TEST(SemaphoreTest, TestAquirement)
{
	shared_ptr<SystemSemaphore> sem = make_shared<SystemSemaphore>("abc", 1);
	
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
	sem->Release(1);
}

TEST(SemaphoreTest, TestRelease)
{
	shared_ptr<SystemSemaphore> sem = make_shared<SystemSemaphore>("abc");

	auto handler = std::async([&] {
		sem->Acquire();
	});

	// has no semaphore
	ASSERT_EQ(handler.wait_for(chrono::seconds{ 1 }), future_status::timeout);

	// signaled
	ASSERT_TRUE(sem->Release(1));
	ASSERT_EQ(handler.wait_for(chrono::seconds{ 1 }), future_status::ready);
}
