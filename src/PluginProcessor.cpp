#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "midi/MidiHandler.h"

namespace
{
constexpr int kMidiBytes = 512;
constexpr int kMaxEvents = 32;
} // namespace

TEW03AudioProcessor::TEW03AudioProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

float TEW03AudioProcessor::raw (const char* id) const
{
    return apvts.getRawParameterValue (id)->load();
}

float TEW03AudioProcessor::tempoBpm() const
{
    // Standalone has no DAW clock. The Seq Tempo knob drives it.
    // A plugin follows the host BPM, and falls back to the knob if the host omits it.
    if (wrapperType != juce::AudioProcessor::wrapperType_Standalone)
    {
        if (auto* head = getPlayHead())
        {
            if (const auto pos = head->getPosition())
            {
                if (const auto bpm = pos->getBpm())
                {
                    if (*bpm > 0.0)
                        return (float) *bpm;
                }
            }
        }
    }

    return raw (ParamID::seqTempo);
}

void TEW03AudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    sequencer.prepare (sampleRate);
}

void TEW03AudioProcessor::releaseResources() {}

void TEW03AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // First callback may grow the host buffer. Later blocks stay at this size.
    midi.ensureSize (kMidiBytes);

    engine.setTone (raw (ParamID::decay),
                    raw (ParamID::accent),
                    raw (ParamID::glide),
                    raw (ParamID::volume));
    engine.setFilter (raw (ParamID::cutoff),
                      raw (ParamID::resonance),
                      raw (ParamID::drive),
                      raw (ParamID::classicMode) >= 0.5f);

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    // Wheel still works while the sequencer owns the notes.
    tew::MidiHandler::applyPitchBend (midi, engine.getVoice());

    const bool playing = raw (ParamID::seqPlay) >= 0.5f;
    const float bpm = tempoBpm();

    if (! playing)
    {
        tew::SeqEvent stopped[1];
        const int nStop = sequencer.advance (numSamples, bpm, false, stopped, 1);
        for (int i = 0; i < nStop; ++i)
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, stopped[i].note), stopped[i].offset);
            engine.getVoice().noteOff (stopped[i].note);
        }

        tew::MidiHandler::applyNotes (midi, engine.getVoice());
        engine.render (buffer, 0, numSamples);
        return;
    }

    // Replace host notes with the pattern so a DAW records the sequence.
    midi.clear();

    tew::SeqEvent events[kMaxEvents];
    const int nEvents = sequencer.advance (numSamples, bpm, true, events, kMaxEvents);

    int rendered = 0;
    for (int i = 0; i < nEvents; ++i)
    {
        const int at = events[i].offset;
        if (at > rendered)
        {
            engine.render (buffer, rendered, at - rendered);
            rendered = at;
        }

        if (events[i].on)
        {
            const auto velocity = events[i].accent ? (juce::uint8) 127 : (juce::uint8) 100;
            midi.addEvent (juce::MidiMessage::noteOn (1, events[i].note, velocity), at);
            engine.getVoice().noteOn (events[i].note, events[i].accent);
        }
        else
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, events[i].note), at);
            engine.getVoice().noteOff (events[i].note);
        }
    }

    if (rendered < numSamples)
        engine.render (buffer, rendered, numSamples - rendered);
}

juce::AudioProcessorEditor* TEW03AudioProcessor::createEditor()
{
    return new TEW03AudioProcessorEditor (*this);
}

void TEW03AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void TEW03AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TEW03AudioProcessor();
}
