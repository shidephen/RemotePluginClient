/*
  ==============================================================================

    RemoteClient.cpp
    Created: 23 Aug 2016 4:39:21pm
    Author:  shidephen

  ==============================================================================
*/

#include "RemoteClient.h"
#include <cassert>

using namespace std;

VstClientSlim::VstClientSlim(int32_t key_in, int32_t key_out)
{
	_plugin_format = new VSTPluginFormat();
}

int VstClientSlim::SendMessage(const message& m)
{
	_out->lock();
	_out->writeInt(m.id);
	_out->writeInt(m.data.size());
	size_t written = 8;

	for (auto i = m.data.begin(); i != m.data.end(); i++)
	{
		_out->writeString(*i);
		// contains a int represent the length of string.
		written += 4 + i->size();
	}

	_out->unlock();
	_out->messageSent();

	return written;
}

message VstClientSlim::RecieveMessage()
{
	_in->waitForMessage();
	_in->lock();

	message m;
	m.id = _in->readInt();
	size_t s = _in->readInt();
	for (; s-- > 0; m.data.push_back(_in->readString()));

	_in->unlock();

	return m;
}

message
VstClientSlim::WaitForMessage(
	const message& m, 
	bool busyWaiting /*= false*/)
{
	while (_in->isInvalid())
	{
		message msg = RecieveMessage();
		ProcessMessage(msg);
		if (msg.id == m.id)
			return msg;
		else if (msg.id == IdUndefined)
			return msg;
	}

	return message();
}

inline message VstClientSlim::FetchAndProcessNextMessage()
{
	message m = RecieveMessage();
	ProcessMessage(m);
	return m;
}

inline void VstClientSlim::Invalidate()
{
	_in->invalidate();
	_out->invalidate();
	_in->messageSent();
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
		UpdateBufferSize();
		break;

	case IdBufferSizeInformation:
		_buffer_size = m.getInt();
		UpdateBufferSize();
		break;

	case IdMidiEvent:
	{
		MidiEvent e(
			static_cast<MidiEventTypes>(m.getInt(0)),
			m.getInt(1), m.getInt(2), m.getInt(3));

		ProcessMidiEvent(e, m.getInt(4));

		break;
	}

	case IdStartProcessing:
		doProcessing();
		reply.id = IdProcessingDone;
		needReplied = true;
		break;

	case IdChangeSharedMemoryKey:
		break;

	case IdInitDone:
		break;
	default:
		break;
	}

	if (needReplied)
		SendMessage(reply);

	return true;
}

void VstClientSlim::ProcessAudio(float** in, float** out)
{
	if (_plugin != nullptr)
	{
		lock_guard<mutex> lock(_m);
		AudioSampleBuffer buffer;
		buffer.setDataToReferTo(
			in,
			_plugin->getNumInputChannels(),
			_plugin->getBlockSize());

		_plugin->processBlock(buffer, _midi_buffer);
	}
}

void VstClientSlim::ProcessMidiEvent(const MidiEvent& event, int32_t offset)
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
		// Todo
		break;
	case MidiControlChange:
		// TODO
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
		// TODO
		break;
	case MidiTimeCode:
		// TODO
		break;
	case MidiSongPosition:
		// TODO
		break;
	case MidiSongSelect:
		// TODO
		break;
	case MidiTuneRequest:
		// TODO
		break;
	case MidiEOX:
		// TODO
		break;
	case MidiSync:
		// TODO
		break;
	case MidiTick:
		// TODO
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
		// TODO
		break;
	case MidiSystemReset:
		// TODO
		break;
	default:
		break;
	}
	_midi_buffer.addEvent(message, offset);
}

void VstClientSlim::SetParameters(const message& m)
{
	if (_plugin == nullptr)
		return;

	lock_guard<mutex> lock(_m);

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

	lock_guard<mutex> lock(_m);

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

	return true;
}

bool VstClientSlim::UnloadPlugin()
{
	if (_plugin != nullptr)
	{
		if (_plugin->hasEditor())
			_plugin->getActiveEditor()->userTriedToCloseWindow();

		_plugin = nullptr;
	}
}

void VstClientSlim::OpenEditor()
{
	
	if (_plugin != nullptr)
	{
		AudioProcessorEditor* editor = _plugin->createEditorIfNeeded();
		editor->toFront(false);
	}
}
