#include <gtest/gtest.h>
#include "RemoteClient.h"
#include "FakeRemotePlugin.hpp"

const char* plugin_path = "E:\\Projects\\RemotePluginClient\\UnitTest\\Minihost.dll";

TEST(ClientTest, TestConstruction)
{
	FakeRemotePlugin server;
	VstClientSlim vst_client(server.KeyIn(), server.KeyOut());
}

TEST(ClientTest, TestLoadPlugin)
{
	FAIL();
}
