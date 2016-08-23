#include "CppUTest/TestHarness.h"
#include "../../Source/Communication.h"
#include <memory>
#include <string>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QString>

TEST_GROUP(Communication)
{
	std::shared_ptr<SharedMemory> src_shared_memory;
    QSharedMemory* target_shared_memory;

	std::shared_ptr<SystemSemaphore> src_semaphore;
    QSystemSemaphore* target_semaphore;

	void setup()
	{
		src_shared_memory = std::make_shared<SharedMemory>("2");
		/*target_shared_memory = std::make_shared<QSharedMemory>("2");*/
        target_shared_memory = new QSharedMemory(QString::number(2));
		src_semaphore = std::make_shared<SystemSemaphore>("3");
		/*target_semaphore = std::make_shared<QSystemSemaphore>("3");*/
        target_semaphore = new QSystemSemaphore(QString::number(3));
	}

	void teardown()
	{
		delete target_shared_memory;
		delete target_semaphore;
	}
};

TEST(Communication, KeyTest)
{
	auto key1 = src_shared_memory->Key();
	auto key2 = target_shared_memory->key().toStdString();
	CHECK(key1 == key2);

	auto native_key1 = src_shared_memory->NativeKey();
	auto native_key2 = target_shared_memory->nativeKey().toStdString();
	CHECK(native_key1 == native_key2);
}
