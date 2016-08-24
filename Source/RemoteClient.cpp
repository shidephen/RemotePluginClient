/*
  ==============================================================================

    RemoteClient.cpp
    Created: 23 Aug 2016 4:39:21pm
    Author:  shidephen

  ==============================================================================
*/

#include "RemoteClient.h"

VstClientSlim::VstClientSlim(int32_t key_in, int32_t key_out)
{
	_plugin_format = new VSTPluginFormat();
}

void VstClientSlim::ProcessAudio(float** in, float** out)
{
	if (_plugin != nullptr)
	{
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

bool VstClientSlim::LoadPlugin(const std::string& path)
{
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
