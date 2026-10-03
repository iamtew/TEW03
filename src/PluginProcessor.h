#pragma once

#include "dsp/Sequencer.h"
#include "dsp/SynthEngine.h"
#include "parameters/ParameterLayout.h"

#include <JuceHeader.h>

class TEW03AudioProcessor : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener
{
public:
    TEW03AudioProcessor();
    ~TEW03AudioProcessor() override;

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

    tew::Sequencer& getSequencer() { return sequencer; }
    void setPatternStep (int index, tew::Sequencer::Step step);
    void syncSeqLength (bool doubled);
    void selectSlot (int bank, int pat);
    int currentBank() const { return curBank.load (std::memory_order_relaxed); }
    int currentPattern() const { return curPattern.load (std::memory_order_relaxed); }
    float tempoBpm() const;
    bool usesHostTempo() const;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    float raw (const char* id) const;
    float hostTempoBpm() const;
    bool patternShouldRun() const;
    bool handleKeyTriggers (const juce::MidiBuffer& midi);
    void loadBanksFromState();
    void writeBanksToState();
    void writeStepToState (int bank, int pat, int index, tew::Sequencer::Step step);
    void loadLiveFromSlot();
    void storeLiveToSlot();
    std::atomic<uint32_t>* slotPacked (int bank, int pat);

    tew::SynthEngine engine;
    tew::Sequencer sequencer;
    std::atomic<uint32_t> patterns[tew::Sequencer::numBanks][tew::Sequencer::patternsPerBank][tew::Sequencer::maxSteps] {};
    std::atomic<int> curBank { 0 };
    std::atomic<int> curPattern { 0 };
    std::atomic<int> heldKey { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TEW03AudioProcessor)
};
