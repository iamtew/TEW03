#pragma once

#include "../PluginProcessor.h"

#include <JuceHeader.h>

class EffectsPage : public juce::Component
{
public:
    static constexpr int kRailW = 110;
    static constexpr int kRowH = 96;
    static constexpr int kStripW = 72;

    explicit EffectsPage (TEW03AudioProcessor& p) : proc (p)
    {
        viewport.setScrollBarsShown (true, false);
        viewport.setViewedComponent (&stack, false);
        addAndMakeVisible (viewport);

        for (int t = 0; t < tew::fxCount; ++t)
        {
            auto* rail = rails.add (new RailButton (*this, t));
            addAndMakeVisible (rail);
            auto* row = rows.add (new FxRow (*this, t));
            stack.addAndMakeVisible (row);
        }
        refresh();
    }

    void paint (juce::Graphics& g) override
    {
        auto body = getLocalBounds();
        auto rail = body.removeFromLeft (kRailW);
        g.setColour (kLaneBlack);
        g.fillRoundedRectangle (rail.toFloat(), 4.f);
        g.setColour (kInk.withAlpha (0.35f));
        g.drawRoundedRectangle (rail.toFloat().reduced (0.5f), 4.f, 1.f);

        if (nVisible == 0)
        {
            g.setColour (kInk);
            g.setFont (juce::Font (juce::FontOptions (13.f).withStyle ("Bold")));
            g.drawText ("Click an effect to add it",
                        body.reduced (12), juce::Justification::centred, true);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds();
        railArea = r.removeFromLeft (kRailW);
        viewport.setBounds (r.reduced (4, 0));
        layoutRail();
        layoutStack();
    }

    void refresh()
    {
        proc.getFxOrder (railOrder, nRail);
        nRail = tew::completeFxOrder (railOrder, nRail);

        auto on = [&] (int t)
        {
            auto* v = proc.apvts.getRawParameterValue (ParamID::fxOnIds[t]);
            return v != nullptr && v->load() >= 0.5f;
        };

        nVisible = 0;
        for (int t = 0; t < tew::fxCount; ++t)
        {
            rails[t]->setOn (on (t));
            rows[t]->setVisible (false);
        }
        for (int i = 0; i < nRail; ++i)
        {
            const int t = railOrder[i];
            if (! on (t))
                continue;
            rows[t]->setVisible (true);
            rows[t]->toFront (false);
            rows[t]->syncDelay();
            visOrder[nVisible++] = t;
        }
        layoutRail();
        layoutStack();
        viewport.setVisible (nVisible > 0);
        repaint();
    }

private:
    static const juce::Colour kInk;
    static const juce::Colour kCream;
    static const juce::Colour kKnob;
    static const juce::Colour kLedOn;
    static const juce::Colour kLedOff;
    static const juce::Colour kLaneBlack;

    static juce::Font labelFont (float h)
    {
        return juce::Font (juce::FontOptions (h).withStyle ("Bold"));
    }

    struct FxKnob : public juce::Component
    {
        FxKnob (TEW03AudioProcessor& p, const char* id, const juce::String& name) : proc (p)
        {
            label.setJustificationType (juce::Justification::centred);
            label.setFont (labelFont (11.f));
            label.setColour (juce::Label::textColourId, kCream);
            label.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            addAndMakeVisible (label);
            combo.setColour (juce::ComboBox::textColourId, kCream);
            combo.setColour (juce::ComboBox::arrowColourId, kCream);
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 52, 12);
            slider.setColour (juce::Slider::textBoxTextColourId, kCream);
            slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
            slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            bind (id, name);
        }

        void bind (const char* id, const juce::String& name)
        {
            label.setText (name, juce::dontSendNotification);
            sliderAtt.reset();
            btnAtt.reset();
            comboAtt.reset();

            auto* param = proc.apvts.getParameter (id);
            auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param);
            auto* boolean = dynamic_cast<juce::AudioParameterBool*> (param);

            button.setVisible (false);
            combo.setVisible (false);
            slider.setVisible (false);

            if (boolean != nullptr)
            {
                isBool = true;
                isChoice = false;
                button.setButtonText (name);
                button.setComponentID ("led");
                button.setVisible (true);
                addAndMakeVisible (button);
                btnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                    proc.apvts, id, button);
            }
            else if (choice != nullptr)
            {
                isBool = false;
                isChoice = true;
                combo.clear (juce::dontSendNotification);
                for (int i = 0; i < choice->choices.size(); ++i)
                    combo.addItem (choice->choices[i], i + 1);
                combo.setVisible (true);
                addAndMakeVisible (combo);
                comboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                    proc.apvts, id, combo);
            }
            else if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            {
                isBool = false;
                isChoice = false;
                slider.setDoubleClickReturnValue (true, ranged->convertFrom0to1 (ranged->getDefaultValue()));
                slider.setVisible (true);
                addAndMakeVisible (slider);
                sliderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                    proc.apvts, id, slider);
            }
            resized();
        }

        void resized() override
        {
            auto r = getLocalBounds();
            label.setBounds (r.removeFromTop (12));
            if (isBool)
                button.setBounds (r.removeFromTop (22).reduced (2, 2));
            else if (isChoice)
                combo.setBounds (r.removeFromTop (22).reduced (2, 2));
            else
                slider.setBounds (r);
        }

        TEW03AudioProcessor& proc;
        juce::Label label;
        juce::Slider slider;
        juce::ToggleButton button;
        juce::ComboBox combo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> btnAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtt;
        bool isBool = false;
        bool isChoice = false;
    };

    struct FxRow : public juce::Component
    {
        FxRow (EffectsPage& page, int t) : owner (page), type (t)
        {
            setName (ParamID::fxNames[t]);
            addKnobs();
        }

        void addKnobs()
        {
            auto add = [this] (const char* id, const char* name)
            {
                knobs.add (new FxKnob (owner.proc, id, name));
                addAndMakeVisible (knobs.getLast());
            };
            switch (type)
            {
                case 0:
                    add (ParamID::fxChoRate, "Rate");
                    add (ParamID::fxChoDepth, "Depth");
                    add (ParamID::fxChoFb, "Fb");
                    add (ParamID::fxChoMix, "Mix");
                    break;
                case 1:
                    add (ParamID::fxCmpThr, "Thresh");
                    add (ParamID::fxCmpRat, "Ratio");
                    add (ParamID::fxCmpAtk, "Atk");
                    add (ParamID::fxCmpRel, "Rel");
                    add (ParamID::fxCmpMix, "Mix");
                    break;
                case 2:
                    add (ParamID::fxDlySync, "Sync");
                    add (ParamID::fxDlyDiv, "Div");
                    add (ParamID::fxDlyTime, "Time");
                    add (ParamID::fxDlyFb, "Fb");
                    add (ParamID::fxDlyCut, "Cut");
                    add (ParamID::fxDlyMix, "Mix");
                    break;
                case 3:
                    add (ParamID::fxDstDrive, "Drive");
                    add (ParamID::fxDstMix, "Mix");
                    break;
                case 4:
                    for (int b = 0; b < tew::eqBandCount; ++b)
                    {
                        auto* pick = eqPick.add (new juce::ToggleButton (juce::String (b + 1)));
                        pick->setClickingTogglesState (true);
                        pick->setRadioGroupId (40);
                        pick->setComponentID ("plain");
                        addAndMakeVisible (pick);
                        pick->onClick = [this, b]
                        {
                            eqBand = b;
                            bindEq();
                        };
                    }
                    eqPick[0]->setToggleState (true, juce::dontSendNotification);
                    add (ParamID::fxEqType[0], "Type");
                    add (ParamID::fxEqFreq[0], "Freq");
                    add (ParamID::fxEqGain[0], "Gain");
                    add (ParamID::fxEqQ[0], "Q");
                    break;
                case 5:
                    add (ParamID::fxFltType, "Type");
                    add (ParamID::fxFltCut, "Cutoff");
                    add (ParamID::fxFltRes, "Res");
                    add (ParamID::fxFltMix, "Mix");
                    break;
                case 6:
                    add (ParamID::fxFlaRate, "Rate");
                    add (ParamID::fxFlaDepth, "Depth");
                    add (ParamID::fxFlaFb, "Fb");
                    add (ParamID::fxFlaMix, "Mix");
                    break;
                case 7:
                    add (ParamID::fxPhaRate, "Rate");
                    add (ParamID::fxPhaDepth, "Depth");
                    add (ParamID::fxPhaFb, "Fb");
                    add (ParamID::fxPhaCentre, "Centre");
                    add (ParamID::fxPhaMix, "Mix");
                    break;
                case 8:
                    add (ParamID::fxRevSize, "Size");
                    add (ParamID::fxRevDamp, "Damp");
                    add (ParamID::fxRevWidth, "Width");
                    add (ParamID::fxRevMix, "Mix");
                    break;
                default:
                    break;
            }
        }

        void bindEq()
        {
            if (type != 4 || knobs.size() < 4)
                return;
            knobs[0]->bind (ParamID::fxEqType[eqBand], "Type");
            knobs[1]->bind (ParamID::fxEqFreq[eqBand], "Freq");
            knobs[2]->bind (ParamID::fxEqGain[eqBand], "Gain");
            knobs[3]->bind (ParamID::fxEqQ[eqBand], "Q");
        }

        void syncDelay()
        {
            if (type != 2 || knobs.size() < 3)
                return;
            auto* v = owner.proc.apvts.getRawParameterValue (ParamID::fxDlySync);
            const bool sync = v != nullptr && v->load() >= 0.5f;
            knobs[1]->setVisible (sync);
            knobs[2]->setVisible (! sync);
            resized();
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (2.f);
            g.setColour (kKnob);
            g.fillRoundedRectangle (r, 4.f);
            g.setColour (kInk.withAlpha (0.4f));
            g.drawRoundedRectangle (r.reduced (0.5f), 4.f, 1.f);

            auto strip = r.removeFromLeft ((float) kStripW).reduced (3.f, 4.f);
            g.setColour (juce::Colour (0xff2e2e2e));
            g.fillRoundedRectangle (strip, 3.f);
            g.setColour (kCream);
            g.setFont (labelFont (11.f));
            g.drawFittedText (ParamID::fxNames[type], strip.toNearestInt(),
                              juce::Justification::centred, 3);
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (4, 4);
            r.removeFromLeft (kStripW);
            if (type == 4 && eqPick.size() == tew::eqBandCount)
            {
                auto picks = r.removeFromLeft (150);
                const int pw = picks.getWidth() / tew::eqBandCount;
                for (int b = 0; b < tew::eqBandCount; ++b)
                    eqPick[b]->setBounds (picks.removeFromLeft (b == tew::eqBandCount - 1 ? picks.getWidth() : pw)
                                               .reduced (2, 18));
            }
            int vis = 0;
            for (auto* k : knobs)
                if (k->isVisible())
                    ++vis;
            if (vis <= 0)
                return;
            const int w = r.getWidth() / vis;
            int i = 0;
            for (auto* k : knobs)
            {
                if (! k->isVisible())
                    continue;
                ++i;
                k->setBounds (r.removeFromLeft (i == vis ? r.getWidth() : w));
            }
        }

        EffectsPage& owner;
        int type = 0;
        int eqBand = 0;
        juce::OwnedArray<juce::ToggleButton> eqPick;
        juce::OwnedArray<FxKnob> knobs;
    };

    struct RailButton : public juce::Component,
                       public juce::DragAndDropTarget
    {
        RailButton (EffectsPage& page, int t) : owner (page), type (t)
        {
            setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        }

        void setOn (bool v)
        {
            if (on == v)
                return;
            on = v;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            g.setColour (on ? juce::Colour (0xff3a3a3a) : kKnob);
            g.fillRoundedRectangle (r, 4.f);
            g.setColour (on ? kInk : kInk.withAlpha (0.45f));
            g.drawRoundedRectangle (r.reduced (0.5f), 4.f, 1.f);

            const float led = 8.f;
            g.setColour (on ? kLedOn : kLedOff);
            g.fillEllipse (r.getX() + 5.f, r.getCentreY() - led * 0.5f, led, led);

            drawIcon (g, juce::Rectangle<float> (r.getRight() - 28.f, r.getY() + 4.f, 22.f, r.getHeight() - 8.f));

            g.setColour (kCream);
            g.setFont (labelFont (11.f));
            g.drawFittedText (ParamID::fxNames[type],
                              juce::Rectangle<int> ((int) r.getX() + 16, (int) r.getY(),
                                                    (int) r.getWidth() - 42, (int) r.getHeight()),
                              juce::Justification::centredLeft, 2);
        }

        void drawIcon (juce::Graphics& g, juce::Rectangle<float> box)
        {
            g.setColour (kCream.withAlpha (on ? 1.f : 0.65f));
            juce::Path p;
            const float x = box.getX(), y = box.getY(), w = box.getWidth(), h = box.getHeight();
            switch (type)
            {
                case 0: // overlapping ovals
                    g.drawEllipse (x, y + h * 0.25f, w * 0.55f, h * 0.5f, 1.4f);
                    g.drawEllipse (x + w * 0.22f, y + h * 0.25f, w * 0.55f, h * 0.5f, 1.4f);
                    g.drawEllipse (x + w * 0.44f, y + h * 0.25f, w * 0.55f, h * 0.5f, 1.4f);
                    break;
                case 1: // compressor bars
                    for (int i = 0; i < 4; ++i)
                    {
                        const float bh = h * (0.25f + 0.15f * (float) (i % 3));
                        g.fillRect (x + 3.f * (float) i, y + (h - bh) * 0.5f, 2.2f, bh);
                    }
                    break;
                case 2: // delay taps
                    for (int i = 0; i < 4; ++i)
                        g.fillRect (x + 4.f * (float) i, y + h * 0.2f, 2.f, h * (0.6f - 0.12f * (float) i));
                    break;
                case 3: // diamond
                    p.addTriangle (x + w * 0.5f, y + 2.f, x + w - 2.f, y + h * 0.5f, x + w * 0.5f, y + h - 2.f);
                    p.addTriangle (x + w * 0.5f, y + 2.f, x + 2.f, y + h * 0.5f, x + w * 0.5f, y + h - 2.f);
                    g.strokePath (p, juce::PathStrokeType (1.3f));
                    break;
                case 4: // eq nodes
                    g.drawLine (x, y + h * 0.6f, x + w, y + h * 0.45f, 1.3f);
                    g.fillEllipse (x + w * 0.2f - 2.f, y + h * 0.55f - 2.f, 4.f, 4.f);
                    g.fillEllipse (x + w * 0.55f - 2.f, y + h * 0.4f - 2.f, 4.f, 4.f);
                    break;
                case 5: // filter hump
                    p.startNewSubPath (x, y + h * 0.75f);
                    p.quadraticTo (x + w * 0.45f, y + 2.f, x + w, y + h * 0.7f);
                    g.strokePath (p, juce::PathStrokeType (1.4f));
                    break;
                case 6: // comb
                    for (int i = 0; i < 5; ++i)
                        g.drawLine (x + 3.5f * (float) i, y + h * 0.2f, x + 3.5f * (float) i, y + h * 0.8f, 1.2f);
                    break;
                case 7: // sine
                    p.startNewSubPath (x, y + h * 0.5f);
                    p.quadraticTo (x + w * 0.25f, y + 2.f, x + w * 0.5f, y + h * 0.5f);
                    p.quadraticTo (x + w * 0.75f, y + h - 2.f, x + w, y + h * 0.5f);
                    g.strokePath (p, juce::PathStrokeType (1.4f));
                    break;
                case 8: // cube
                    g.drawRect (x + 3.f, y + h * 0.3f, w * 0.55f, h * 0.5f, 1.2f);
                    g.drawLine (x + 3.f, y + h * 0.3f, x + 8.f, y + h * 0.15f, 1.2f);
                    g.drawLine (x + 3.f + w * 0.55f, y + h * 0.3f, x + 8.f + w * 0.55f, y + h * 0.15f, 1.2f);
                    g.drawLine (x + 8.f, y + h * 0.15f, x + 8.f + w * 0.55f, y + h * 0.15f, 1.2f);
                    break;
                default:
                    break;
            }
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (e.getDistanceFromDragStart() > 6)
                if (auto* c = juce::DragAndDropContainer::findParentDragContainerFor (this))
                    c->startDragging ("fx:" + juce::String (type), this);
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (! e.mouseWasClicked())
                return;
            auto* v = owner.proc.apvts.getRawParameterValue (ParamID::fxOnIds[type]);
            const bool next = ! (v != nullptr && v->load() >= 0.5f);
            owner.proc.setFxEnabled (type, next);
            owner.refresh();
        }

        bool isInterestedInDragSource (const SourceDetails& d) override
        {
            return d.description.toString().startsWith ("fx:");
        }

        void itemDropped (const SourceDetails& d) override
        {
            const int from = d.description.toString().fromFirstOccurrenceOf (":", false, false).getIntValue();
            if (from == type)
                return;
            owner.proc.moveFx (from, type);
            owner.refresh();
        }

        EffectsPage& owner;
        int type = 0;
        bool on = false;
    };

    void layoutRail()
    {
        auto rail = railArea;
        const int n = juce::jmax (1, nRail);
        const int h = juce::jmax (1, rail.getHeight() / n);
        for (int i = 0; i < nRail; ++i)
            rails[railOrder[i]]->setBounds (rail.removeFromTop (i == nRail - 1 ? rail.getHeight() : h)
                                                .reduced (3, 2));
    }

    void layoutStack()
    {
        const int w = juce::jmax (1, viewport.getWidth());
        stack.setSize (w, juce::jmax (viewport.getHeight(), nVisible * kRowH));
        int y = 0;
        for (int i = 0; i < nVisible; ++i)
        {
            rows[visOrder[i]]->setBounds (0, y, w, kRowH);
            y += kRowH;
        }
    }

    TEW03AudioProcessor& proc;
    juce::Viewport viewport;
    juce::Component stack;
    juce::OwnedArray<RailButton> rails;
    juce::OwnedArray<FxRow> rows;
    juce::Rectangle<int> railArea;
    int railOrder[tew::fxCount] {};
    int nRail = 0;
    int visOrder[tew::fxCount] {};
    int nVisible = 0;
};

inline const juce::Colour EffectsPage::kInk { 0xff111111 };
inline const juce::Colour EffectsPage::kCream { 0xfff4ead0 };
inline const juce::Colour EffectsPage::kKnob { 0xff1a1a1a };
inline const juce::Colour EffectsPage::kLedOn { 0xffff2200 };
inline const juce::Colour EffectsPage::kLedOff { 0xff5a1808 };
inline const juce::Colour EffectsPage::kLaneBlack { 0xff2a2a2a };
