#pragma once

#include "PluginProcessor.h"

class TEW03AudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit TEW03AudioProcessorEditor (TEW03AudioProcessor&);
    ~TEW03AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshPlayhead();
    void refreshHostTempo();
    void refreshSlot();
    void syncSteps();

    TEW03AudioProcessor& proc;

    struct PianoRoll : public juce::Component
    {
        explicit PianoRoll (TEW03AudioProcessor& p);
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseUp (const juce::MouseEvent& e) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override;
        void setPlayhead (int step);
        int lockNote (int note) const;
        void scrollBy (int semitones);

        TEW03AudioProcessor& proc;
        int playhead = -1;
        int dragStep = -1;
        int keyRoot = 0; // C
        int viewLow = 36; // C2, one octave window
        int gutterStartY = 0;
        int gutterStartView = 36;
        bool minor = false;
        bool locked = true;
        bool dragged = false;
        bool gutterDrag = false;
    };

    struct StepColumn : public juce::Component
    {
        StepColumn (TEW03AudioProcessor& p, int stepIndex, juce::Component& roll);
        void paint (juce::Graphics& g) override;
        void resized() override;
        void setLit (bool on);
        void syncFrom (TEW03AudioProcessor& p);

        juce::Component& roll;
        juce::ToggleButton accent { "A" };
        juce::ToggleButton slide { "S" };
        int index = 0;
        bool lit = false;
    };

    struct PanelLnF : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
        void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
        juce::Font getLabelFont (juce::Label&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
    };

    struct ParamCell : public juce::Component
    {
        ParamCell (juce::AudioProcessorValueTreeState& state,
                   juce::RangedAudioParameter& param);
        void resized() override;

        juce::Label label;
        juce::Slider slider;
        juce::ToggleButton button;
        juce::ComboBox combo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtt;
        bool isBool = false;
        bool isFlip = false;
        bool isChoice = false;
    };

    PanelLnF panelLnF;
    juce::TooltipWindow tooltipWindow { this };
    juce::Label keyLabel, scaleLabel, bankLabel, patternLabel;
    juce::ComboBox keyBox, scaleBox, bankBox, patternBox;
    juce::TextButton prevPat { "<" }, nextPat { ">" };
    juce::ToggleButton lockBtn { "Lock" };
    juce::ToggleButton x2Btn { "2x" };
    juce::ToggleButton runBtn { "Run" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> x2Att, runAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> bankAtt, patternAtt;
    PianoRoll pianoRoll;
    juce::OwnedArray<StepColumn> steps;
    juce::OwnedArray<ParamCell> seqCells;
    juce::OwnedArray<ParamCell> filterCells;
    juce::OwnedArray<ParamCell> masterCells;
    ParamCell* tempoCell = nullptr;
    juce::Rectangle<int> seqArea, filterArea, masterArea;
    int lastPlayhead = -1;
    int lastBank = -1;
    int lastPattern = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TEW03AudioProcessorEditor)
};
