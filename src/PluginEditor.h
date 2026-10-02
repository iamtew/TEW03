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

    TEW03AudioProcessor& proc;

    struct PitchCell : public juce::Component
    {
        PitchCell (TEW03AudioProcessor& p, int stepIndex);
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseUp (const juce::MouseEvent& e) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override;
        void refresh();

        TEW03AudioProcessor& proc;
        int index = 0;
        int lastNote = 36;
        int dragStartY = 0;
        int dragStartNote = 36;
        bool dragged = false;
        bool lit = false;
    };

    struct StepColumn : public juce::Component
    {
        StepColumn (TEW03AudioProcessor& p, int stepIndex);
        void paint (juce::Graphics& g) override;
        void resized() override;
        void refreshPitch();
        void setLit (bool on);

        PitchCell pitch;
        juce::ToggleButton accent { "A" };
        juce::ToggleButton slide { "S" };
        int index = 0;
    };

    struct PanelLnF : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
        juce::Font getLabelFont (juce::Label&) override;
    };

    struct ParamCell : public juce::Component
    {
        ParamCell (juce::AudioProcessorValueTreeState& state,
                   juce::RangedAudioParameter& param);
        void resized() override;

        juce::Label label;
        juce::Slider slider;
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAtt;
        bool isBool = false;
        bool isFlip = false;
    };

    PanelLnF panelLnF;
    juce::TooltipWindow tooltipWindow { this };
    juce::OwnedArray<StepColumn> steps;
    juce::OwnedArray<ParamCell> seqCells;
    juce::OwnedArray<ParamCell> filterCells;
    juce::OwnedArray<ParamCell> masterCells;
    ParamCell* tempoCell = nullptr;
    juce::Rectangle<int> seqArea, filterArea, masterArea;
    int lastPlayhead = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TEW03AudioProcessorEditor)
};
