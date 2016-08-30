#include <gtest/gtest.h>
#include "Communication.h"

TEST(SemaphoreTest, TestSetKey)
{
	SystemSemaphore* sem = new SystemSemaphore("");
	ASSERT_EQ(sem->Key(), "");
	ASSERT_EQ(sem->NativeKey(), "");
	delete sem;
}

TEST(SemaphoreTest, TestAquirement)
{
	FAIL();
}

TEST(SemaphoreTest, TestRelease)
{
	FAIL();
}

TEST(SemaphoreTest, TestDistory)
{
	FAIL();
}
