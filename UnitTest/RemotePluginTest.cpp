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
	FakeRemotePlugin server;
	VstClientSlim vst_client(server.KeyIn(), server.KeyOut());

	ASSERT_TRUE(vst_client.LoadPlugin(
		"E:\\Projects\\RemotePluginClient\\UnitTest\\Minihost.dll"));

	ASSERT_TRUE(vst_client.IsInitialized());
}

TEST(ClientTest, TestSaveLoadPreset)
{
	const char* preset_file = "preset.fxb";
	FakeRemotePlugin server;
	VstClientSlim vst_client(server.KeyIn(), server.KeyOut());

	vst_client.LoadPlugin(
		"E:\\Projects\\RemotePluginClient\\UnitTest\\Minihost.dll");

	ASSERT_TRUE(vst_client.SaveChuckToFile(preset_file));
	
	File f(preset_file);
	ASSERT_TRUE(f.existsAsFile());

	EXPECT_TRUE(vst_client.LoadChuckFromFile(preset_file));
	f.deleteFile();
}

TEST(ClientTest, TestSaveLoadSettings)
{
	const char* preset_file = "preset.fxp";
	FakeRemotePlugin server;
	VstClientSlim vst_client(server.KeyIn(), server.KeyOut());

	vst_client.LoadPlugin(
		"E:\\Projects\\RemotePluginClient\\UnitTest\\Minihost.dll");

	ASSERT_TRUE(vst_client.SaveSettingsToFile(preset_file));

	File f(preset_file);
	ASSERT_TRUE(f.existsAsFile());

	EXPECT_TRUE(vst_client.LoadSettingsFromFile(preset_file));
	f.deleteFile();
}
