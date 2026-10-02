#pragma once

#include "dsp/Sequencer.h"
#include "dsp/SynthEngine.h"
#include "parameters/ParameterLayout.h"

#include <JuceHeader.h>

class TEW03AudioProcessor : public juce::AudioProcessor
{
public:
    TEW03AudioProcessor();
    ~TEW03AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TEW03"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    float raw (const char* id) const;
    float tempoBpm() const;
    bool seqShouldRun() const;

    tew::SynthEngine engine;
    tew::Sequencer sequencer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TEW03AudioProcessor)
};
