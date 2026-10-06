#pragma once

#include "dsp/Lfo.h"
#include "dsp/Sequencer.h"
#include "dsp/SynthEngine.h"
#include "parameters/Library.h"
#include "parameters/ParameterLayout.h"

#include <JuceHeader.h>
#include <atomic>
#include <vector>

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
    void copyCurrentPattern();
    void pasteCurrentPattern();
    void clearCurrentPattern();
    bool hasPatternClip() const { return patternClipValid; }
    void syncSeqLength (bool doubled);
    void selectSlot (int bank, int pat);
    int currentBank() const { return curBank.load (std::memory_order_relaxed); }
    int currentPattern() const { return curPattern.load (std::memory_order_relaxed); }
    float tempoBpm() const;
    bool usesHostTempo() const;

    void setLfoShape (int index, const tew::LfoShape& shape);
    tew::LfoShape getLfoShape (int index) const;
    float lfoPhase (int index) const;
    bool lfoPlayheadOn() const;
    void resetLfoShapes();

    juce::File patchesDir() const;
    juce::File banksDir() const;
    juce::StringArray patchNames() const;
    juce::StringArray bankNames() const;
    juce::String patchName() const;
    juce::String bankName() const;
    void initPatch();
    void initBank();
    bool loadPatchByName (const juce::String& name);
    bool loadBankByName (const juce::String& name);
    bool savePatch();
    bool saveBank();
    bool savePatchAs (const juce::String& name);
    bool saveBankAs (const juce::String& name);
    bool exportPatch (const juce::File& dest);
    bool exportBank (const juce::File& dest);
    bool importPatch (const juce::File& src);
    bool importBank (const juce::File& src);

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
    void applyCurrentPattern (const tew::Sequencer::Step src[tew::Sequencer::maxSteps]);
    std::atomic<uint32_t>* slotPacked (int bank, int pat);
    void copyLiveBanks (tew::Sequencer::Step dest[tew::Sequencer::numBanks]
                                                 [tew::Sequencer::patternsPerBank]
                                                 [tew::Sequencer::maxSteps]);
    void applyBankSteps (const tew::Sequencer::Step src[tew::Sequencer::numBanks]
                                                      [tew::Sequencer::patternsPerBank]
                                                      [tew::Sequencer::maxSteps]);
    void applyPatchParams (const std::vector<tew::PatchParam>& params);
    std::vector<tew::PatchParam> currentPatchParams() const;
    bool loadPatchFile (const juce::File& src, const juce::String& shownName);
    bool loadBankFile (const juce::File& src, const juce::String& shownName);
    void writeLfosToState();
    void loadLfosFromState();
    void publishLfoShapes();
    void fillVoiceMod (tew::VoiceMod& mod);
    void retriggerLfos();
    float lfoRateHz (int index) const;

    tew::SynthEngine engine;
    tew::Sequencer sequencer;
    tew::Lfo lfo[tew::numLfos];
    tew::LfoShape uiLfo[tew::numLfos];
    tew::LfoShape lfoBank[2][tew::numLfos];
    std::atomic<int> lfoPublished { 0 };
    int lfoSeen { -1 };
    std::atomic<float> lfoPhaseUi[tew::numLfos] {};
    std::atomic<bool> keyboardSounding { false };
    std::atomic<uint32_t> patterns[tew::Sequencer::numBanks][tew::Sequencer::patternsPerBank][tew::Sequencer::maxSteps] {};
    tew::Sequencer::Step patternClip[tew::Sequencer::maxSteps] {};
    bool patternClipValid = false;
    std::atomic<int> curBank { 0 };
    std::atomic<int> curPattern { 0 };
    std::atomic<int> heldKey { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TEW03AudioProcessor)
};
