/*
  ==============================================================================

    RemoteClient.h
    Created: 23 Aug 2016 4:39:21pm
    Author:  shidephen

  ==============================================================================
*/

#ifndef REMOTECLIENT_H_INCLUDED
#define REMOTECLIENT_H_INCLUDED
#include <cstdint>
#include "Communication.h"
#include <memory>
#include <mutex>
#include <vector>
#include "VstSyncData.h"
#include "MidiEvent.h"
#include "JuceHeader.h"

class VstClientSlim
{
public:
	enum RemoteMessageIDs
	{
		IdUndefined,
		IdInitDone,
		IdQuit,
		IdSampleRateInformation,
		IdBufferSizeInformation,
		IdMidiEvent,
		IdStartProcessing,
		IdProcessingDone,
		IdChangeSharedMemoryKey,
		IdChangeInputCount,
		IdChangeOutputCount,
		IdShowUI,
		IdHideUI,
		IdSaveSettingsToString,
		IdSaveSettingsToFile,
		IdLoadSettingsFromString,
		IdLoadSettingsFromFile,
		IdSavePresetFile,
		IdLoadPresetFile,
		IdDebugMessage,
		IdUserBase = 64
	};

	enum VstRemoteMessageIDs
	{
		// vstPlugin -> remoteVstPlugin
		IdVstLoadPlugin = IdUserBase,
		IdVstPluginWindowInformation,
		IdVstClosePlugin,
		IdVstSetTempo,
		IdVstSetLanguage,
		IdVstGetParameterCount,
		IdVstGetParameterDump,
		IdVstSetParameterDump,
		IdVstProgramNames,
		IdVstCurrentProgram,
		IdVstCurrentProgramName,
		IdVstSetProgram,
		IdVstRotateProgram,
		IdVstIdleUpdate,

		// remoteVstPlugin -> vstPlugin
		IdVstFailedLoadingPlugin,
		IdVstBadDllFormat,
		IdVstPluginWindowID,
		IdVstPluginEditorGeometry,
		IdVstPluginName,
		IdVstPluginVersion,
		IdVstPluginVendorString,
		IdVstPluginProductString,
		IdVstPluginPresetsString,
		IdVstPluginUniqueID,
		IdVstSetParameter,
		IdVstParameterCount,
		IdVstParameterDump

	};

	VstClientSlim(int32_t key_in, int32_t key_out);
	~VstClientSlim();

	VstSyncData* QtVstShm();

	//------------------------ Plugin information ------------------------------
	std::string PluginName();
	std::string PluginVendor();
	std::string PluginProduct();
	std::string Version();
	//--------------------------------------------------------------------------
	//-------------------------- Message routines ------------------------------
	int SendMessage(const message& m);
	message RecieveMessage();
	message WaitForMessage(const message& m, bool busyWaiting = false);
	message FetchAndProcessNextMessage();
	void Invalidate();
	bool ProcessMessage(const message& m);
	//--------------------------------------------------------------------------
	//---------------------- Audio & MIDI processing ---------------------------
	void ProcessAudio(float** in, float** out);
	void ProcessMidiEvent(const MidiEvent&, int32_t);
	//--------------------------------------------------------------------------
	//----------------------- Buses & buffers ----------------------------------
	void UpdateBufferSize() const;
	uint32_t SampleRate() const;
	int16_t BufferSize() const;
	int InputCount() const;
	void InputCount(int n);
	int OutputCount() const;
	void OutputCount(int n);
	void SetIOCount(int input, int output);
	//--------------------------------------------------------------------------
	//--------------------------- Thread sync ----------------------------------
	inline bool Lock() { _m.lock(); }
	inline void Unlock() { _m.unlock(); }
	std::mutex& GetMutex() { return _m; }
	//--------------------------------------------------------------------------
	//------------------------- Parameters -------------------------------------
	void SetParameters(const message& m);
	message DumpParameters();
	//--------------------------------------------------------------------------

	bool LoadPlugin(const std::string& path);
	bool UnloadPlugin();

	void OpenEditor();

	bool IsInitialized() const;

	float* Memory();
	std::shared_ptr<const shmFifo> In() const { return _in; }
	std::shared_ptr<const shmFifo> Out() const { return _out; }
private:
	void _SetShmKey(int32_t key, size_t size);
	void _DoProcessing();

	SharedMemory _shmObj;
	SharedMemory _shmQtId;
	VstSyncData* _vstSyncData;

	float* _shm;
	size_t _input_count;
	size_t _output_count;
	uint32_t _sample_rate;
	int16_t _buffer_size;

	ScopedPointer<AudioPluginInstance> _plugin;
	ScopedPointer<AudioPluginFormat> _plugin_format;
	MidiBuffer _midi_buffer;
	PluginDescription _description;

	bool _loaded;
	// lock this only when processing audio or read&write parameters.
	std::mutex _m;

	std::shared_ptr<shmFifo> _in;
	std::shared_ptr<shmFifo> _out;
};

#endif  // REMOTECLIENT_H_INCLUDED
