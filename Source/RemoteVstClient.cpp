/*
  ==============================================================================

    RemoteClient.cpp
    Created: 23 Aug 2016 4:39:21pm
    Author:  shidephen

  ==============================================================================
*/

#include "RemoteClient.h"
#include <cassert>
#include <cmath>
#include <sstream>

using namespace std;

VstClientSlim::VstClientSlim(int32_t key_in, int32_t key_out)
	: RemoteClientBase(key_in, key_out),
	_vstSyncData(nullptr), _bpm(0),
	_sample_rate(44100), _buffer_size(512), _old_input_count(0),
	_old_output_count(0), _loaded(false)
{
	// TODO: Initialize
	_plugin_format = new VSTPluginFormat();
}

bool VstClientSlim::ProcessMessage(const message& m)
{
	message reply(m.id);
	bool needReplied = false;
	switch (m.id)
	{
	case IdUndefined:
		return false;

	case IdQuit:
		return false;

	case IdSampleRateInformation:
		_sample_rate = m.getInt();
		_UpdateBufferSize();
		break;

	case IdBufferSizeInformation:
		_buffer_size = m.getInt();
		_UpdateBufferSize();
		break;

	case IdMidiEvent:
	{
		MidiEvent e(
			static_cast<MidiEventTypes>(m.getInt(0)),
			m.getInt(1), m.getInt(2), m.getInt(3));

		_ProcessMidiEvent(e, m.getInt(4));

		break;
	}

	case IdStartProcessing:
		DoProcessing();
		reply.id = IdProcessingDone;
		needReplied = true;
		break;

	case IdChangeSharedMemoryKey:
		// TODO: 
		break;

	case IdInitDone:
		break;

	case IdVstLoadPlugin:
		LoadPlugin(m.getString());
		break;

	case IdVstPluginWindowInformation:
		break;

	case IdVstSetTempo:
		_bpm = m.getInt();
		break;

	case IdVstSetLanguage:
		_language = static_cast<VstHostLanguages>(m.getInt());
		
		break;

	case IdVstGetParameterDump:
		reply = DumpParameters();
		needReplied = true;
		break;

	case IdVstSetParameterDump:
		SetParameters(m);
		break;

	case IdSaveSettingsToFile:
		SaveChuckToFile(m.getString());
		needReplied = true;
		reply = message(IdSaveSettingsToFile);
		break;

	case IdLoadSettingsFromFile:
		LoadChuckFromFile(m.getString());
		needReplied = true;
		reply = message(IdLoadSettingsFromFile);
		break;

	case IdLoadPresetFile:
		LoadSettingsFromFile(m.getString());
		needReplied = true;
		reply = message(IdLoadPresetFile);
		break;

	case IdSavePresetFile:
		SaveSettingsToFile(m.getString());
		needReplied = true;
		reply = message(IdSavePresetFile);
		break;

	case IdVstSetProgram:
		_SetProgram(m.getInt(0));
		reply = message(IdVstSetProgram);
		needReplied = true;
		// sendCurrentProgramName
		break;

	case IdVstCurrentProgram:
		reply = message(IdVstCurrentProgram);
		reply.addInt(_GetCurrentProgram());
		needReplied = true;
		break;

	case IdVstRotateProgram:
		_RotateProgram(m.getInt(0));
		needReplied = true;
		reply = message(IdVstRotateProgram);
		// sendCurrentProgramName
		break;

	case IdVstProgramNames:
		reply = message(IdVstProgramNames).addString(_GetProgramNames());
		needReplied = true;
		break;

	case IdVstSetParameter:
	{
		lock_guard<mutex> lock(GetMutex());
		_plugin->setParameter(m.getInt(0), m.getFloat(1));
		break;
	}

	case IdVstIdleUpdate:
		// TODO: Vst Idle
		// int newCurrentProgram = pluginDispatch( effGetProgram );
// 		if (newCurrentProgram != m_currentProgram)
// 		{
// 			m_currentProgram = newCurrentProgram;
// 			sendCurrentProgramName();
// 		}
		break;

	case IdShowUI:
		OpenEditor();
		break;
	case IdHideUI:
		HideEditor();
		break;
	default:
		break;
	}

	if (needReplied)
		SendMessage(reply);

	return true;
}

void VstClientSlim::_ProcessAudio(float* &shm)
{
	if (_plugin != nullptr)
	{
		lock_guard<mutex> lock(GetMutex());

		_plugin->prepareToPlay(_sample_rate, _buffer_size);

		AudioSampleBuffer buffer;
		buffer.setDataToReferTo(
			&shm,
			_plugin->getTotalNumInputChannels() 
			+ _plugin->getTotalNumOutputChannels(),
			_plugin->getBlockSize());

		_plugin->processBlock(buffer, _midi_buffer);
	}
}

void VstClientSlim::_ProcessMidiEvent(const MidiEvent& event, int32_t offset)
{
	MidiMessage message;
	auto channel = event.channel();
	switch (event.type())
	{
	case MidiPitchBend:
		message = MidiMessage::pitchWheel(
			channel,
			event.pitchBend());
		break;
	case MidiNoteOff:
		message = MidiMessage::noteOn(
			channel,
			event.key(), 
			event.velocity());
		break;
	case MidiNoteOn:
		message = MidiMessage::noteOff(
			channel,
			event.key(),
			event.velocity());
		break;
	case MidiKeyPressure:
		// Todo: MidiKeyPressure
		break;
	case MidiControlChange:
		// TODO: MidiControlChange
		break;
	case MidiProgramChange:
		message = MidiMessage::programChange(
			channel,
			event.program());
		break;
	case MidiChannelPressure:
		message = MidiMessage::channelPressureChange(
			channel, 
			event.channelPressure());
		break;
	case MidiSysEx:
		// TODO: MidiSysEx
		break;
	case MidiTimeCode:
		// TODO: MidiTimeCode
		break;
	case MidiSongPosition:
		// TODO: MidiSongPosition
		break;
	case MidiSongSelect:
		// TODO: MidiSongSelect
		break;
	case MidiTuneRequest:
		// TODO: MidiTuneRequest
		break;
	case MidiEOX:
		// TODO: MidiEOX
		break;
	case MidiSync:
		// TODO: MidiSync
		break;
	case MidiTick:
		// TODO: MidiTick
		break;
	case MidiStart:
		message = MidiMessage::midiStart();
		break;
	case MidiContinue:
		message = MidiMessage::midiContinue();
		break;
	case MidiStop:
		message = MidiMessage::midiStop();
		break;
	case MidiActiveSensing:
		// TODO: MidiActiveSensing
		break;
	case MidiSystemReset:
		// TODO: MidiSystemReset
		break;
	default:
		break;
	}
	_midi_buffer.addEvent(message, offset);
}

inline void VstClientSlim::_UpdateBufferSize() const
{
	_plugin->setPlayConfigDetails(
		_InputCount(), 
		_OutputCount(),
		_sample_rate, 
		_buffer_size);
}

inline void VstClientSlim::_InputCount(int n)
{
	int output = _OutputCount();
	_SetIOCount(n, output);
}

inline void VstClientSlim::_OutputCount(int n)
{
	int input = _InputCount();
	_SetIOCount(input, n);
}

inline void VstClientSlim::_SetIOCount(int input, int output)
{
	_plugin->setPlayConfigDetails(input, output, _sample_rate, _buffer_size);
}

void VstClientSlim::SetParameters(const message& m)
{
	if (_plugin == nullptr)
		return;

	lock_guard<mutex> lock(GetMutex());

	size_t n = m.getInt(0), params = _plugin->getNumParameters();
	params = (n > params ? params : n);

	for (size_t p = 0, i = 0; i < params; ++i)
	{
		int index = m.getInt(++p);  
		string label = m.getString(++p);
		float value = m.getFloat(++p);
		_plugin->setParameter(index, value);
	}
}

message VstClientSlim::DumpParameters()
{
	if (_plugin == nullptr)
		return message();

	lock_guard<mutex> lock(GetMutex());

	message m(IdVstParameterDump);
	size_t nums = _plugin->getNumParameters();
	m.addInt(nums);

	const auto &params = _plugin->getParameters();
	for (size_t i = 0; i < nums; ++i)
	{
		AudioProcessorParameter* parameter = params[i];
		m.addInt(parameter->getParameterIndex());
		m.addString(parameter->getName(32).toStdString());
		m.addFloat(parameter->getValue());
	}

	return m;
}

bool VstClientSlim::SaveSettingsToFile(const std::string& path)
{
	File f(path);
	if (!f.create())
		return false;
	
	bool isFXB = path.find_last_of("fxb") != path.npos;
	ScopedPointer<FileOutputStream> stream = f.createOutputStream();

	MemoryBlock mem;
	VSTPluginFormat::saveToFXBFile(_plugin, mem, isFXB);
	//_plugin->getStateInformation(mem);
	size_t size = mem.getSize();

	stream->write(mem.getData(), size);

	stream->flush();

	return true;
}

bool VstClientSlim::LoadSettingsFromFile(const std::string& path)
{
	File f(path);
	if (!f.existsAsFile())
		return false;

	ScopedPointer<FileInputStream> stream = f.createInputStream();
	MemoryBlock mem;

	size_t size = stream->readIntoMemoryBlock(mem);

	VSTPluginFormat::loadFromFXBFile(_plugin, mem.getData(), size);
	// _plugin->setStateInformation(mem.getData(), size);

	return true;
}

bool VstClientSlim::SaveChuckToFile(const std::string& path)
{
	File f(path);
	if (!f.create())
		return false;

	ScopedPointer<FileOutputStream> stream = f.createOutputStream();

	MemoryBlock mem;
	VSTPluginFormat::getChunkData(_plugin, mem, true);
	size_t size = mem.getSize();

	stream->write(mem.getData(), size);

	stream->flush();

	return true;
}

bool VstClientSlim::LoadChuckFromFile(const std::string& path)
{
	File f(path);
	if (!f.existsAsFile())
		return false;

	ScopedPointer<FileInputStream> stream = f.createInputStream();
	MemoryBlock mem;

	size_t size = stream->readIntoMemoryBlock(mem);

	VSTPluginFormat::setChunkData(_plugin, mem.getData(), size, true);

	return true;
}

void VstClientSlim::_SetProgram(int program)
{
	if (!IsInitialized())
		return;

	if (program < 0)
		program = 0;
	if (program >= _plugin->getNumPrograms())
		program = _plugin->getNumPrograms() - 1;

	_plugin->setCurrentProgram(program);
}

void VstClientSlim::_RotateProgram(int offset)
{
	if (!IsInitialized())
		return;

	int newProgram = _GetCurrentProgram() + offset;

	_SetProgram(newProgram);
}

std::string VstClientSlim::_GetProgramNames()
{
	if (!IsInitialized())
		return "";

	size_t progNum = _plugin->getNumPrograms();
	progNum = progNum >= 256 ? 256 : progNum;

	if (progNum <= 1)
		return _plugin->getProgramName(
			_plugin->getCurrentProgram()).toStdString();

	ostringstream fmt;

	for (size_t i = 0; i < progNum; ++i)
	{
		String name = _plugin->getProgramName(i);
		if (i == 0) fmt << name.toStdString();
		else fmt << '|' << name.toStdString();
	}

	return fmt.str();
}

bool VstClientSlim::LoadPlugin(const std::string& path)
{
	assert(_plugin == nullptr); // must unload first.

	OwnedArray<PluginDescription> plugin_info;
	_plugin_format->findAllTypesForFile(plugin_info, path);
	if (plugin_info.isEmpty())
	{
		Logger::outputDebugString("This is not a plugin file.");
		return false;
	}

	_plugin = _plugin_format->createInstanceFromDescription(
		*plugin_info.getFirst(),
		(double)_sample_rate, 
		(int)_buffer_size);

	if (_plugin == nullptr)
	{
		Logger::outputDebugString("Cannot create plugin.");
		return false;
	}

	VSTPluginFormat::setExtraFunctions(_plugin, this);
	_loaded = true;
	return true;
}

bool VstClientSlim::UnloadPlugin()
{
	if (_plugin != nullptr)
	{
		if (_plugin->hasEditor())
			_plugin->getActiveEditor()->userTriedToCloseWindow();

		_plugin = nullptr;
		_loaded = false;
		return true;
	}

	return false;
}

void VstClientSlim::OpenEditor()
{
	
	if (_plugin != nullptr)
	{
		AudioProcessorEditor* editor = _plugin->createEditorIfNeeded();
		editor->setVisible(true);
		editor->toFront(false);
	}
}

void VstClientSlim::HideEditor()
{
	if (_plugin != nullptr && _plugin->hasEditor())
	{
		_plugin->getActiveEditor()->setVisible(false);
	}
}

void VstClientSlim::DoProcessing()
{
	if (_shm == nullptr)
		return;

	_ProcessAudio(_shm);
}

int64 VstClientSlim::getTempoAt(int64 samplePos)
{
	UNREFERENCED_PARAMETER(samplePos);
	return _bpm * 10000;
}

int VstClientSlim::getAutomationState()
{
	/** This should return the host's automation state.
	@returns 0 = not supported, 1 = off, 2 = read, 3 = write, 4 = read/write
	*/
	return 4;
}

void VstClientSlim::audioProcessorParameterChanged(AudioProcessor* processor, int parameterIndex, float newValue)
{
	processor->setParameter(parameterIndex, newValue);
}

// UNDONE: 此处是插件IO变化通知LMMS，即实现host callback，原实现也是LMMS不主动控制IO
void VstClientSlim::audioProcessorChanged(AudioProcessor* processor)
{
	if (_old_input_count != _InputCount())
	{
		_old_input_count = _InputCount();
		SendMessage(message(IdChangeInputCount)
			.addInt(processor->getTotalNumInputChannels()));
	}
	
	if (_old_output_count != _OutputCount())
	{
		_old_output_count = _OutputCount();
		SendMessage(message(IdChangeOutputCount)
			.addInt(processor->getTotalNumOutputChannels()));
	}
}

VstClientSlim::~VstClientSlim()
{
	UnloadPlugin();
}
