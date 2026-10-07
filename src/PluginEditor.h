#pragma once

#include "PluginProcessor.h"
#include "ui/EffectsPage.h"
#include "ui/EqPage.h"

#include <functional>

class TEW03AudioProcessorEditor : public juce::AudioProcessorEditor,
                                  public juce::DragAndDropContainer,
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
    void refreshLfo();
    void assignDest (int dest, int lfoIndex);
    void setDestAmt (int dest, float amt);
    void syncSteps();
    void refreshLibraryNames();
    void openPatchMenu();
    void openBankMenu();
    void openPatternMenu();
    void openPatternClipMenu();
    void cyclePatch (int delta);
    void cycleBank (int delta);
    void cyclePattern (int delta);
    void chooseSaveAs (bool patch);
    void chooseExport (bool patch);
    void chooseImport (bool patch);
    void setEditorPage (int page);
    void openPageMenu();
    void cyclePage (int delta);
    void applyPageVisibility();
    void applyLock();

    TEW03AudioProcessor& proc;

    struct PianoRoll : public juce::Component,
                       private juce::Timer
    {
        explicit PianoRoll (TEW03AudioProcessor& p);
        ~PianoRoll() override;
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseUp (const juce::MouseEvent& e) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override;
        void timerCallback() override;
        void setPlayhead (int step);
        int lockNote (int note) const;
        void scrollBy (int steps);
        void stopHold();

        TEW03AudioProcessor& proc;
        int playhead = -1;
        int dragStep = -1;
        int keyRoot = 0; // C
        int viewLow = 36; // C2, one octave window
        int gutterStartY = 0;
        int gutterStartView = 36;
        int holdDir = 0; // +1 top overlay, -1 bottom, 0 idle
        bool minor = false;
        bool locked = false;
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

    struct SlotBar : public juce::Component,
                     public juce::SettableTooltipClient
    {
        void paint (juce::Graphics& g) override;
        void mouseUp (const juce::MouseEvent& e) override;
        void setText (const juce::String& t);

        juce::String text;
        std::function<void (int)> onStep;
        std::function<void()> onOpen;
        std::function<void()> onPopup;
    };

    struct ParamCell : public juce::Component,
                       public juce::DragAndDropTarget
    {
        ParamCell (TEW03AudioProcessorEditor& ed,
                   juce::RangedAudioParameter& param);
        void resized() override;
        void paintOverChildren (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseUp (const juce::MouseEvent& e) override;
        bool isInterestedInDragSource (const SourceDetails&) override;
        void itemDropped (const SourceDetails&) override;
        void refreshMod();
        juce::Rectangle<int> badgeBounds() const;

        TEW03AudioProcessorEditor& editor;
        juce::String paramId;
        juce::Label label;
        juce::Slider slider;
        juce::ToggleButton button;
        juce::ComboBox combo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtt;
        int dest = -1;
        int lfoSrc = 0;
        float lfoAmt = 0.f;
        float amtDragStart = 0.f;
        int amtDragY = 0;
        bool amtDragging = false;
        bool isBool = false;
        bool isFlip = false;
        bool isChoice = false;
    };

    struct LfoHandle : public juce::Component
    {
        LfoHandle (TEW03AudioProcessorEditor& ed, int i);
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent& e) override;

        TEW03AudioProcessorEditor& editor;
        int index = 0;
        bool dragging = false;
    };

    struct LfoShapeView : public juce::Component
    {
        LfoShapeView (TEW03AudioProcessorEditor& ed, int i);
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent& e) override;
        juce::Point<float> toScreenPt (const tew::LfoShape& s, int i) const;
        int hitPoint (juce::Point<float> p) const;
        int sixteenths() const;

        TEW03AudioProcessorEditor& editor;
        int index = 0;
        int dragPt = -1;
    };

    struct LfoLane : public juce::Component
    {
        LfoLane (TEW03AudioProcessorEditor& ed, int i);
        void resized() override;
        void refresh();
        void cyclePreset (int delta);

        TEW03AudioProcessorEditor& editor;
        int index = 0;
        LfoHandle handle;
        LfoShapeView view;
        SlotBar nameBar;
        juce::Label modeLabel, tempoLabel, smoothLabel;
        juce::ComboBox modeBox, divBox;
        juce::ToggleButton syncBtn { "Sync" };
        juce::Slider rateSlider, smoothSlider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt, divAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAtt, smoothAtt;
    };

    PanelLnF panelLnF;
    juce::TooltipWindow tooltipWindow { this };
    juce::Label keyLabel, scaleLabel, bankLabel, patternLabel;
    juce::ComboBox keyBox, scaleBox, bankBox;
    juce::ToggleButton lockBtn { "Lock" };
    juce::ToggleButton x2Btn { "2x" };
    juce::ToggleButton clearBtn { "Clear" };
    juce::ToggleButton runBtn { "Run" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> x2Att, runAtt, lockAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> bankAtt, keyAtt, scaleAtt;
    SlotBar patchBar, bankLibBar, patBar, pageBar;
    EffectsPage effectsPage;
    EqPage eqPage;
    int editorPage = 0;
    std::unique_ptr<juce::FileChooser> chooser;
    PianoRoll pianoRoll;
    juce::OwnedArray<StepColumn> steps;
    juce::OwnedArray<ParamCell> seqCells;
    juce::OwnedArray<ParamCell> filterCells;
    juce::OwnedArray<ParamCell> masterCells;
    ParamCell* tempoCell = nullptr;
    juce::Rectangle<int> seqArea, filterArea, masterArea, lfoArea;
    int lastPlayhead = -1;
    int lastBank = -1;
    int lastPattern = -1;

    LfoLane lfoLane0 { *this, 0 };
    LfoLane lfoLane1 { *this, 1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TEW03AudioProcessorEditor)
};
