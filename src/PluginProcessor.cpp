#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "midi/MidiHandler.h"

namespace
{
constexpr int kMidiBytes = 512;
constexpr int kMaxEvents = 32;

const juce::Identifier kPattern { "PATTERN" };
const juce::Identifier kStep { "STEP" };
const juce::Identifier kIndex { "index" };
const juce::Identifier kNote { "note" };
const juce::Identifier kAccent { "accent" };
const juce::Identifier kSlide { "slide" };

tew::Sequencer::Step stepFromTree (const juce::ValueTree& t)
{
    return { (int) t.getProperty (kNote, 36),
             (bool) t.getProperty (kAccent, false),
             (bool) t.getProperty (kSlide, false) };
}

juce::ValueTree makeStepTree (int index, tew::Sequencer::Step s)
{
    juce::ValueTree t (kStep);
    t.setProperty (kIndex, index, nullptr);
    t.setProperty (kNote, s.note, nullptr);
    t.setProperty (kAccent, s.accent, nullptr);
    t.setProperty (kSlide, s.slide, nullptr);
    return t;
}

juce::ValueTree makeDefaultPatternTree()
{
    tew::Sequencer::Step baked[tew::Sequencer::numSteps];
    tew::Sequencer::fillDefault (baked);

    juce::ValueTree pattern (kPattern);
    for (int i = 0; i < tew::Sequencer::numSteps; ++i)
        pattern.appendChild (makeStepTree (i, baked[i]), nullptr);
    return pattern;
}

juce::ValueTree findStepChild (juce::ValueTree pattern, int index)
{
    for (int i = 0; i < pattern.getNumChildren(); ++i)
    {
        auto child = pattern.getChild (i);
        if ((int) child.getProperty (kIndex, -1) == index)
            return child;
    }
    return {};
}
} // namespace

TEW03AudioProcessor::TEW03AudioProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    loadPatternFromState();
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

bool TEW03AudioProcessor::seqShouldRun() const
{
    if (raw (ParamID::seqPlay) < 0.5f)
        return false;

    // Standalone, or a host that never handed us a playhead: Seq Play is the clock.
    if (wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        return true;

    if (auto* head = getPlayHead())
    {
        if (const auto pos = head->getPosition())
            return pos->getIsPlaying();
    }

    return true;
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
                    raw (ParamID::volume),
                    raw (ParamID::waveform) >= 0.5f);
    engine.setFilter (raw (ParamID::cutoff),
                      raw (ParamID::resonance),
                      raw (ParamID::drive),
                      raw (ParamID::classicMode) >= 0.5f);

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    // Wheel still works while the sequencer owns the notes.
    tew::MidiHandler::applyPitchBend (midi, engine.getVoice());

    const bool playing = seqShouldRun();
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
            engine.getVoice().noteOn (events[i].note, events[i].accent, events[i].slide);
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

void TEW03AudioProcessor::loadPatternFromState()
{
    auto pattern = apvts.state.getChildWithName (kPattern);
    if (! pattern.isValid())
    {
        apvts.state.appendChild (makeDefaultPatternTree(), nullptr);
        pattern = apvts.state.getChildWithName (kPattern);
    }

    tew::Sequencer::Step baked[tew::Sequencer::numSteps];
    tew::Sequencer::fillDefault (baked);
    tew::Sequencer::Step loaded[tew::Sequencer::numSteps];

    for (int i = 0; i < tew::Sequencer::numSteps; ++i)
    {
        auto child = findStepChild (pattern, i);
        loaded[i] = child.isValid() ? stepFromTree (child) : baked[i];
        if (! child.isValid())
            pattern.appendChild (makeStepTree (i, loaded[i]), nullptr);
    }

    sequencer.loadAll (loaded);
}

void TEW03AudioProcessor::writeStepToState (int index, tew::Sequencer::Step step)
{
    auto pattern = apvts.state.getChildWithName (kPattern);
    if (! pattern.isValid())
    {
        apvts.state.appendChild (makeDefaultPatternTree(), nullptr);
        pattern = apvts.state.getChildWithName (kPattern);
    }

    auto child = findStepChild (pattern, index);
    if (! child.isValid())
    {
        pattern.appendChild (makeStepTree (index, step), nullptr);
        return;
    }

    child.setProperty (kNote, step.note, nullptr);
    child.setProperty (kAccent, step.accent, nullptr);
    child.setProperty (kSlide, step.slide, nullptr);
}

void TEW03AudioProcessor::setPatternStep (int index, tew::Sequencer::Step step)
{
    sequencer.setStep (index, step);
    writeStepToState (index, step);
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
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            loadPatternFromState();
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TEW03AudioProcessor();
}
