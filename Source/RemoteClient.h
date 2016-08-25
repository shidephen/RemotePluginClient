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
	void ProcessAudio(float* &buffer);
	void ProcessMidiEvent(const MidiEvent&, int32_t);
	//--------------------------------------------------------------------------
	//----------------------- Buses & buffers ----------------------------------
	void UpdateBufferSize() const;
	uint32_t SampleRate() const { return _sample_rate; };
	int16_t BufferSize() const { return _buffer_size; };
	int InputCount() const { return _plugin->getTotalNumInputChannels(); };
	void InputCount(int n);
	int OutputCount() const { return _plugin->getTotalNumInputChannels(); };
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
	bool SaveSettingsToFile(const std::string& path);
	bool LoadSettingsFromFile(const std::string& path);
	//--------------------------------------------------------------------------
	//---------------------------- Program -------------------------------------
	void SetProgram(int program);
	int GetCurrentProgram() const { return _plugin->getCurrentProgram(); };
	void RotateProgram(int offset);
	std::string GetProgramNames();
	//--------------------------------------------------------------------------

	bool LoadPlugin(const std::string& path);
	bool UnloadPlugin();

	void OpenEditor();
	void HideEditor();

	bool IsInitialized() const { return _loaded; };

	float* Memory();
	std::shared_ptr<const shmFifo> In() const { return _in; }
	std::shared_ptr<const shmFifo> Out() const { return _out; }
private:
	void _SetShmKey(int32_t key);
	void _DoProcessing();

	SharedMemory _shmObj;
	SharedMemory _shmQtId;
	VstSyncData* _vstSyncData;

	float* _shm;
	size_t _input_count;
	size_t _output_count;
	uint32_t _sample_rate;
	int16_t _buffer_size;
	uint16_t _bpm;

	ScopedPointer<AudioPluginInstance> _plugin;
	ScopedPointer<AudioPluginFormat> _plugin_format;
	MidiBuffer _midi_buffer;
	PluginDescription _description;

	bool _loaded;

	VstHostLanguages _language = LanguageEnglish;
	// lock this only when processing audio or read&write parameters.
	std::mutex _m;

	std::shared_ptr<shmFifo> _in;
	std::shared_ptr<shmFifo> _out;
};

#endif  // REMOTECLIENT_H_INCLUDED
