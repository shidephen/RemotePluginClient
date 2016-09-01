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
#include <memory>
#include <mutex>
#include <vector>
#include "VstSyncData.h"
#include "MidiEvent.h"
#include "Communication.h"
#include "JuceHeader.h"

class RemoteClientBase
{
public:
	RemoteClientBase(int32_t key_in, int32_t key_out);
	RemoteClientBase(
		std::shared_ptr<shmFifo>& shm_in,
		std::shared_ptr<shmFifo>& shm_out);

	RemoteClientBase();

	virtual ~RemoteClientBase();

	//-------------------------- Message routines ------------------------------
	int SendMessage(const message& m);
	message RecieveMessage();
	message WaitForMessage(const message& m, bool busyWaiting = false);
	message FetchAndProcessNextMessage();
	void Invalidate();
	virtual bool ProcessMessage(const message& m);
	//--------------------------------------------------------------------------

	//--------------------------- Thread sync ----------------------------------
	inline bool Lock() { _m.lock(); }
	inline void Unlock() { _m.unlock(); }
	std::mutex& GetMutex() { return _m; }
	//--------------------------------------------------------------------------

	//---------------------------- IPC -----------------------------------------
	inline int32_t KeyIn() const { return _key_in; }
	inline int32_t KeyOut() const { return _key_out; }
	std::shared_ptr<shmFifo>& In() { return _in; }
	std::shared_ptr<shmFifo>& Out() { return _out; }
	//--------------------------------------------------------------------------

protected:
	float* _shm;
	std::shared_ptr<shmFifo> _in;
	std::shared_ptr<shmFifo> _out;

private:
	void _SetShmKey(int32_t key);

	SharedMemory _shmObj;

	// lock this only when processing audio or read&write parameters.
	std::mutex _m;

	int32_t _key_in;
	int32_t _key_out;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RemoteClientBase);
};

class VstClientSlim 
	:public VSTPluginFormat::ExtraFunctions,
	public AudioProcessorListener,
	public RemoteClientBase
{
public:
	
	VstClientSlim(int32_t key_in, int32_t key_out);
	~VstClientSlim();

	VstSyncData* QtVstShm() const {
		return _vstSyncData;
	};

	//------------------------ Plugin information ------------------------------
	std::string PluginName() const { 
		return _plugin->getName().toStdString(); 
	};
	std::string PluginVendor() const {
		return "TUYA";
	};
	std::string PluginProduct() const {
		return "Unkown";
	};
	std::string Version() const {
		return "1";
	};
	//--------------------------------------------------------------------------

	void DoProcessing();

	//-------------------------- Message routines ------------------------------
	bool ProcessMessage(const message& m) override;
	//--------------------------------------------------------------------------
	
	//------------------------- Parameters -------------------------------------
	void SetParameters(const message& m);
	message DumpParameters();
	bool SaveSettingsToFile(const std::string& path);
	bool LoadSettingsFromFile(const std::string& path);
	bool SaveChuckToFile(const std::string& path);
	bool LoadChuckFromFile(const std::string& path);
	//--------------------------------------------------------------------------

	uint32_t SampleRate() const { return _sample_rate; };
	int16_t BufferSize() const { return _buffer_size; };

	bool LoadPlugin(const std::string& path);
	bool UnloadPlugin();

	void OpenEditor();
	void HideEditor();

	bool IsInitialized() const { return _loaded; };

	//----------------------- Override -----------------------------------------
	virtual int64 getTempoAt(int64 samplePos) override;

	virtual int getAutomationState() override;

	virtual void audioProcessorParameterChanged(
		AudioProcessor* processor,
		int parameterIndex,
		float newValue) override;

	virtual void audioProcessorChanged(AudioProcessor* processor) override;
	//--------------------------------------------------------------------------

	//---------------------- Audio & MIDI processing ---------------------------
	void _ProcessAudio(float* &buffer);
	void _ProcessMidiEvent(const MidiEvent&, int32_t);
	//--------------------------------------------------------------------------

	//----------------------- Buses & buffers ----------------------------------
	void _UpdateBufferSize() const;
	int _InputCount() const { return _plugin->getTotalNumInputChannels(); };
	void _InputCount(int n);
	int _OutputCount() const { return _plugin->getTotalNumInputChannels(); };
	void _OutputCount(int n);
	void _SetIOCount(int input, int output);
	//--------------------------------------------------------------------------
	
	//---------------------------- Program -------------------------------------
	void _SetProgram(int program);
	int _GetCurrentProgram() const { return _plugin->getCurrentProgram(); };
	void _RotateProgram(int offset);
	std::string _GetProgramNames();
	//--------------------------------------------------------------------------

private:

	VstSyncData* _vstSyncData;

	size_t _old_input_count;
	size_t _old_output_count;
	uint32_t _sample_rate;
	int16_t _buffer_size;
	uint16_t _bpm;

	ScopedPointer<AudioPluginInstance> _plugin;
	ScopedPointer<AudioPluginFormat> _plugin_format;
	MidiBuffer _midi_buffer;
	PluginDescription _description;

	bool _loaded;

	VstHostLanguages _language = LanguageEnglish;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VstClientSlim);
};

#endif  // REMOTECLIENT_H_INCLUDED
