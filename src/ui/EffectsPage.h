#pragma once

#include "../PluginProcessor.h"
#include "../dsp/EqBiquad.h"

#include <JuceHeader.h>

class EffectsPage : public juce::Component,
                    private juce::Timer
{
public:
    static constexpr int kRailW = 124;
    static constexpr int kGripW = 14;
    static constexpr int kVizH = 260;
    static constexpr int kKnobRowH = 118;

    explicit EffectsPage (TEW03AudioProcessor& p) : proc (p)
    {
        logo = juce::ImageCache::getFromMemory (BinaryData::icon_png, BinaryData::icon_pngSize);
        viewport.setScrollBarsShown (true, false);
        viewport.setViewedComponent (&stack, false);
        addAndMakeVisible (viewport);

        for (int t = 0; t < tew::fxCount; ++t)
        {
            auto* rail = rails.add (new RailButton (*this, t));
            addAndMakeVisible (rail);
            auto* row = rows.add (new FxRow (*this, t));
            stack.addAndMakeVisible (row);
            row->setVisible (false);
        }
        for (auto* rail : rails)
            rail->bindClick();
        refresh();
        startTimerHz (24);
    }

    ~EffectsPage() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        auto body = getLocalBounds();
        auto rail = body.removeFromLeft (kRailW);
        g.setColour (kLaneBlack);
        g.fillRoundedRectangle (rail.toFloat(), 4.f);
        g.setColour (kInk.withAlpha (0.35f));
        g.drawRoundedRectangle (rail.toFloat().reduced (0.5f), 4.f, 1.f);

        if (selected < 0)
        {
            auto msg = body.reduced (12);
            if (logo.isValid())
            {
                const int s = juce::jmin (180, msg.getWidth() - 24, msg.getHeight() / 2);
                auto logoR = juce::Rectangle<int> (0, 0, s, s)
                                 .withCentre ({ msg.getCentreX(), msg.getCentreY() - 28 });
                g.setOpacity (0.92f);
                g.drawImage (logo, logoR.toFloat(), juce::RectanglePlacement::centred);
                g.setOpacity (1.f);
                msg.removeFromTop (logoR.getBottom() - msg.getY());
            }
            g.setColour (kInk);
            g.setFont (juce::Font (juce::FontOptions (13.f).withStyle ("Bold")));
            g.drawText ("Click an effect to add it",
                        msg, juce::Justification::centredTop, true);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds();
        railArea = r.removeFromLeft (kRailW);
        viewport.setBounds (r);
        layoutRail();
        layoutDetail();
    }

    void refreshMods()
    {
        for (auto* row : rows)
            row->refreshMods();
    }

    void select (int type)
    {
        if (type < 0 || type >= tew::fxCount)
            return;
        if (selected == type)
            return;
        selected = type;
        layoutDetail();
        for (auto* rail : rails)
            rail->repaint();
        repaint();
    }

    void refresh()
    {
        proc.getFxOrder (railOrder, nRail);
        nRail = tew::completeFxOrder (railOrder, nRail);

        if (rails.size() < tew::fxCount || rows.size() < tew::fxCount)
            return;

        auto on = [&] (int t)
        {
            auto* v = proc.apvts.getRawParameterValue (ParamID::fxOnIds[t]);
            return v != nullptr && v->load() >= 0.5f;
        };

        for (int t = 0; t < tew::fxCount; ++t)
            rails[t]->setOn (on (t));

        if (selected >= 0)
            rows[selected]->syncDelay();
        layoutRail();
        viewport.setVisible (selected >= 0);
        for (auto* rail : rails)
            rail->repaint();
    }

private:
    static const juce::Colour kInk;
    static const juce::Colour kCream;
    static const juce::Colour kKnob;
    static const juce::Colour kLedOn;
    static const juce::Colour kLedOff;
    static const juce::Colour kLaneBlack;
    static const juce::Colour kEqSum;
    static const juce::Colour kEqBand[tew::eqBandCount];

    static juce::Font labelFont (float h)
    {
        return juce::Font (juce::FontOptions (h).withStyle ("Bold"));
    }

    void timerCallback() override
    {
        vizPhase += 0.035f;
        if (vizPhase >= 1.f)
            vizPhase -= 1.f;
        if (selected >= 0 && selected < rows.size())
            rows[selected]->tickViz();
    }

    struct FxKnob : public juce::Component,
                     public juce::DragAndDropTarget
    {
        FxKnob (TEW03AudioProcessor& p, const char* id, const juce::String& name) : proc (p)
        {
            label.setJustificationType (juce::Justification::centred);
            label.setFont (labelFont (11.f));
            label.setColour (juce::Label::textColourId, kInk);
            label.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            label.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (label);
            combo.setColour (juce::ComboBox::textColourId, kCream);
            combo.setColour (juce::ComboBox::arrowColourId, kCream);
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 52, 12);
            slider.setColour (juce::Slider::textBoxTextColourId, kInk);
            slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
            slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            slider.addMouseListener (this, false);
            bind (id, name);
        }

        void bind (const char* id, const juce::String& name)
        {
            paramId = id;
            slot = ParamID::fxModFor (id);
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
            refreshMod();
            resized();
        }

        void refreshMod()
        {
            lfoSrc = 0;
            lfoAmt = 0.f;
            if (slot != nullptr)
            {
                auto* src = proc.apvts.getRawParameterValue (slot->lfo);
                auto* amt = proc.apvts.getRawParameterValue (slot->amt);
                if (src != nullptr)
                    lfoSrc = juce::roundToInt (src->load());
                if (amt != nullptr)
                    lfoAmt = amt->load();
            }
            slider.getProperties().set ("lfoSrc", lfoSrc);
            slider.getProperties().set ("lfoAmt", lfoAmt);
            slider.getProperties().set ("lfoY", lfoSrc > 0 ? proc.lfoOut (lfoSrc - 1) : 0.5f);
            slider.repaint();
            repaint();
        }

        juce::Rectangle<int> badgeBounds() const
        {
            return { getWidth() - 16, 0, 14, 12 };
        }

        void paintOverChildren (juce::Graphics& g) override
        {
            if (slot == nullptr || lfoSrc <= 0)
                return;
            auto b = badgeBounds().toFloat();
            const auto col = lfoColour (lfoSrc);
            g.setColour (kKnob);
            g.fillRoundedRectangle (b, 3.f);
            g.setColour (col);
            g.drawRoundedRectangle (b, 3.f, 1.2f);
            g.setFont (labelFont (10.f));
            g.drawText (juce::String (lfoSrc), b.toNearestInt(), juce::Justification::centred, false);
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (slot == nullptr)
                return;
            const auto pos = e.getEventRelativeTo (this).getPosition();
            if (lfoSrc > 0 && badgeBounds().contains (pos))
            {
                amtDragging = true;
                amtDragStart = lfoAmt;
                amtDragY = pos.y;
                return;
            }
            if (! e.mods.isPopupMenu())
                return;

            juce::PopupMenu m;
            for (int i = 1; i <= tew::numLfos; ++i)
                m.addItem (i, "LFO " + juce::String (i), true, lfoSrc == i);
            m.addItem (tew::numLfos + 1, "None", true, lfoSrc == 0);
            juce::Component::SafePointer<FxKnob> safe (this);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                             [safe] (int result)
                             {
                                 if (safe == nullptr || result <= 0)
                                     return;
                                 safe->assign (result == tew::numLfos + 1 ? 0 : result);
                             });
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (! amtDragging || slot == nullptr)
                return;
            const auto pos = e.getEventRelativeTo (this).getPosition();
            const float next = juce::jlimit (-1.f, 1.f, amtDragStart - (float) (pos.y - amtDragY) / 80.f);
            setAmt (next);
        }

        void mouseUp (const juce::MouseEvent&) override { amtDragging = false; }

        bool isInterestedInDragSource (const SourceDetails& d) override
        {
            return slot != nullptr && d.description.toString().startsWith ("lfo:");
        }

        void itemDropped (const SourceDetails& d) override
        {
            const int src = d.description.toString().fromFirstOccurrenceOf (":", false, false).getIntValue();
            if (src >= 1 && src <= tew::numLfos)
                assign (src);
        }

        void assign (int srcIndex)
        {
            if (slot == nullptr)
                return;
            auto* src = proc.apvts.getParameter (slot->lfo);
            auto* amt = proc.apvts.getParameter (slot->amt);
            if (src == nullptr || amt == nullptr)
                return;
            src->beginChangeGesture();
            src->setValueNotifyingHost (src->convertTo0to1 ((float) juce::jlimit (0, tew::numLfos, srcIndex)));
            src->endChangeGesture();
            if (srcIndex > 0 && std::abs (lfoAmt) < 0.001f)
            {
                amt->beginChangeGesture();
                amt->setValueNotifyingHost (amt->convertTo0to1 (0.5f));
                amt->endChangeGesture();
            }
            refreshMod();
        }

        void setAmt (float amount)
        {
            if (slot == nullptr)
                return;
            auto* amt = proc.apvts.getParameter (slot->amt);
            if (amt == nullptr)
                return;
            amt->setValueNotifyingHost (amt->convertTo0to1 (juce::jlimit (-1.f, 1.f, amount)));
            refreshMod();
        }

        static juce::Colour lfoColour (int src)
        {
            static const juce::Colour cols[] = {
                juce::Colour (0xff3dcc8c), juce::Colour (0xffff6a00),
                juce::Colour (0xff4c8dff), juce::Colour (0xffff4fa3)
            };
            return cols[juce::jlimit (0, tew::numLfos - 1, src - 1)];
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
        juce::String paramId;
        const ParamID::FxModSlot* slot = nullptr;
        int lfoSrc = 0;
        float lfoAmt = 0.f;
        float amtDragStart = 0.f;
        int amtDragY = 0;
        bool amtDragging = false;
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
        FxRow (EffectsPage& page, int t) : owner (page), type (t), viz (*this)
        {
            setName (ParamID::fxNames[t]);
            addAndMakeVisible (viz);
            addKnobs();
            if (type == 4)
            {
                eqPlot = std::make_unique<EqPlot> (*this);
                addAndMakeVisible (*eqPlot);
                viz.setVisible (false);
            }
        }

        void refreshMods()
        {
            for (auto* k : knobs)
                k->refreshMod();
        }

        void tickViz()
        {
            if (type == 4 && eqPlot != nullptr)
                eqPlot->repaint();
            else
                viz.repaint();
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
                    add (ParamID::fxChoCentre, "Centre");
                    add (ParamID::fxChoWidth, "Width");
                    break;
                case 1:
                    add (ParamID::fxCmpThr, "Thresh");
                    add (ParamID::fxCmpRat, "Ratio");
                    add (ParamID::fxCmpAtk, "Atk");
                    add (ParamID::fxCmpRel, "Rel");
                    add (ParamID::fxCmpMix, "Mix");
                    add (ParamID::fxCmpMake, "Makeup");
                    add (ParamID::fxCmpKnee, "Knee");
                    add (ParamID::fxCmpHpf, "HPF");
                    break;
                case 2:
                    add (ParamID::fxDlySync, "Sync");
                    add (ParamID::fxDlyDiv, "Div");
                    add (ParamID::fxDlyTime, "Time");
                    add (ParamID::fxDlyFb, "Fb");
                    add (ParamID::fxDlyCut, "Cut");
                    add (ParamID::fxDlyMix, "Mix");
                    add (ParamID::fxDlyPing, "Ping");
                    add (ParamID::fxDlyHp, "HP");
                    add (ParamID::fxDlySpread, "Spread");
                    break;
                case 3:
                    add (ParamID::fxDstType, "Type");
                    add (ParamID::fxDstDrive, "Drive");
                    add (ParamID::fxDstBias, "Bias");
                    add (ParamID::fxDstTone, "Tone");
                    add (ParamID::fxDstMix, "Mix");
                    break;
                case 4:
                    add (ParamID::fxEqType[0], "Type");
                    add (ParamID::fxEqFreq[0], "Freq");
                    add (ParamID::fxEqGain[0], "Gain");
                    add (ParamID::fxEqQ[0], "Q");
                    add (ParamID::fxEqTrim, "Trim");
                    break;
                case 5:
                    add (ParamID::fxFltType, "Type");
                    add (ParamID::fxFltSlope, "Slope");
                    add (ParamID::fxFltCut, "Cutoff");
                    add (ParamID::fxFltRes, "Res");
                    add (ParamID::fxFltDrive, "Drive");
                    add (ParamID::fxFltMix, "Mix");
                    break;
                case 6:
                    add (ParamID::fxFlaRate, "Rate");
                    add (ParamID::fxFlaDepth, "Depth");
                    add (ParamID::fxFlaFb, "Fb");
                    add (ParamID::fxFlaMix, "Mix");
                    add (ParamID::fxFlaCentre, "Centre");
                    add (ParamID::fxFlaWidth, "Width");
                    break;
                case 7:
                    add (ParamID::fxPhaRate, "Rate");
                    add (ParamID::fxPhaDepth, "Depth");
                    add (ParamID::fxPhaFb, "Fb");
                    add (ParamID::fxPhaCentre, "Centre");
                    add (ParamID::fxPhaStages, "Stages");
                    add (ParamID::fxPhaMix, "Mix");
                    break;
                case 8:
                    add (ParamID::fxRevSize, "Size");
                    add (ParamID::fxRevDamp, "Damp");
                    add (ParamID::fxRevWidth, "Width");
                    add (ParamID::fxRevMix, "Mix");
                    add (ParamID::fxRevPre, "Pre");
                    add (ParamID::fxRevFreeze, "Freeze");
                    add (ParamID::fxRevLo, "Lo Cut");
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
            if (eqPlot != nullptr)
                eqPlot->repaint();
        }

        void setEqParam (const char* id, float v)
        {
            auto* p = dynamic_cast<juce::RangedAudioParameter*> (owner.proc.apvts.getParameter (id));
            if (p == nullptr)
                return;
            p->setValueNotifyingHost (p->convertTo0to1 (p->getNormalisableRange().snapToLegalValue (v)));
        }

        void beginEq (const char* id)
        {
            if (auto* p = owner.proc.apvts.getParameter (id))
                p->beginChangeGesture();
        }
        void endEq (const char* id)
        {
            if (auto* p = owner.proc.apvts.getParameter (id))
                p->endChangeGesture();
        }

        float loadEq (const char* id) const
        {
            auto* v = owner.proc.apvts.getRawParameterValue (id);
            return v != nullptr ? v->load() : 0.f;
        }

        void resetEqId (const char* id)
        {
            auto* p = owner.proc.apvts.getParameter (id);
            if (p == nullptr)
                return;
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->getDefaultValue());
            p->endChangeGesture();
        }

        void resetEqBand (int b)
        {
            resetEqId (ParamID::fxEqType[b]);
            resetEqId (ParamID::fxEqFreq[b]);
            resetEqId (ParamID::fxEqGain[b]);
            resetEqId (ParamID::fxEqQ[b]);
            bindEq();
        }

        float load (const char* id, float fallback = 0.f) const
        {
            auto* v = owner.proc.apvts.getRawParameterValue (id);
            return v != nullptr ? v->load() : fallback;
        }

        struct EqPlot : public juce::Component
        {
            explicit EqPlot (FxRow& row) : fx (row)
            {
                setMouseCursor (juce::MouseCursor::CrosshairCursor);
            }

            float sr() const
            {
                const double s = fx.owner.proc.getSampleRate();
                return s > 1.0 ? (float) s : 44100.f;
            }

            juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced (4.f, 6.f); }

            static float xToHz (float x01)
            {
                return 20.f * std::pow (10.f, juce::jlimit (0.f, 1.f, x01) * 3.f);
            }
            static float hzToX (float hz)
            {
                return std::log10 (juce::jlimit (20.f, 20000.f, hz) / 20.f) / 3.f;
            }
            static float yToDb (float y01)
            {
                return (1.f - juce::jlimit (0.f, 1.f, y01)) * 36.f - 18.f;
            }
            static float dbToY (float db)
            {
                return 1.f - juce::jlimit (0.f, 1.f, (db + 18.f) / 36.f);
            }
            static float specToY (float db)
            {
                return 1.f - juce::jlimit (0.f, 1.f, (db + 72.f) / 72.f);
            }

            juce::Point<float> bandPos (const tew::EqBandParam& p, const tew::Biquad& bq) const
            {
                auto r = plot();
                float db = p.gainDb;
                if (p.type == tew::eqTypeHP || p.type == tew::eqTypeLP || p.type <= tew::eqTypeOff)
                {
                    if (p.type > tew::eqTypeOff)
                    {
                        tew::Biquad one[1] { bq };
                        const float mag = tew::eqMagnitude (one, 1, sr(), p.hz);
                        db = mag > 1.0e-8f ? 20.f * std::log10 (mag) : 0.f;
                    }
                    else
                        db = 0.f;
                }
                return { r.getX() + hzToX (p.hz) * r.getWidth(),
                         r.getY() + dbToY (db) * r.getHeight() };
            }

            int hitBand (juce::Point<float> pos, const tew::EqBandParam* params, const tew::Biquad* bq) const
            {
                int best = -1;
                float bestD = 14.f * 14.f;
                for (int b = 0; b < tew::eqBandCount; ++b)
                {
                    const auto pt = bandPos (params[b], bq[b]);
                    const float d = pos.getDistanceSquaredFrom (pt);
                    if (d < bestD)
                    {
                        bestD = d;
                        best = b;
                    }
                }
                return best;
            }

            void readBands (tew::EqBandParam* params, tew::Biquad* bq) const
            {
                tew::readEqBands (fx.owner.proc.apvts, ParamID::fxEqType, ParamID::fxEqFreq,
                                  ParamID::fxEqGain, ParamID::fxEqQ, params);
                for (int b = 0; b < tew::eqBandCount; ++b)
                    bq[b] = tew::eqBiquad (params[b].type, sr(), params[b].hz, params[b].q, params[b].gainDb);
            }

            void paint (juce::Graphics& g) override
            {
                auto bounds = getLocalBounds().toFloat();
                g.setColour (juce::Colour (0xff12151a));
                g.fillRoundedRectangle (bounds, 3.f);

                auto r = plot();
                const float zeroY = r.getY() + dbToY (0.f) * r.getHeight();
                g.setColour (juce::Colour (0xff2a2e32));
                g.drawHorizontalLine ((int) zeroY, r.getX(), r.getRight());
                for (float hz : { 100.f, 1000.f, 10000.f })
                    g.drawVerticalLine ((int) (r.getX() + hzToX (hz) * r.getWidth()), r.getY(), r.getBottom());

                tew::EqBandParam params[tew::eqBandCount];
                tew::Biquad bq[tew::eqBandCount];
                readBands (params, bq);
                const int n = juce::jmax (2, (int) r.getWidth());
                if ((int) specDb.size() != n)
                    specDb.resize ((size_t) n);
                fx.owner.proc.eqAnalyser().copyLogDb (specDb.data(), n);
                juce::Path spec;
                spec.startNewSubPath (r.getX(), r.getBottom());
                for (int i = 0; i < n; ++i)
                {
                    const float x01 = (float) i / (float) (n - 1);
                    spec.lineTo (r.getX() + x01 * r.getWidth(),
                                 r.getY() + specToY (specDb[(size_t) i]) * r.getHeight());
                }
                spec.lineTo (r.getRight(), r.getBottom());
                spec.closeSubPath();
                g.setColour (juce::Colour (0x55a0a8b0));
                g.fillPath (spec);

                for (int b = 0; b < tew::eqBandCount; ++b)
                {
                    if (params[b].type <= tew::eqTypeOff)
                        continue;
                    tew::Biquad one[1] { bq[b] };
                    juce::Path path;
                    for (int i = 0; i < n; ++i)
                    {
                        const float x01 = (float) i / (float) (n - 1);
                        const float mag = tew::eqMagnitude (one, 1, sr(), xToHz (x01));
                        const float db = mag > 1.0e-8f ? 20.f * std::log10 (mag) : -80.f;
                        const juce::Point<float> pt (r.getX() + x01 * r.getWidth(),
                                                     r.getY() + dbToY (db) * r.getHeight());
                        if (i == 0)
                            path.startNewSubPath (pt);
                        else
                            path.lineTo (pt);
                    }
                    g.setColour (kEqBand[b].withAlpha (b == fx.eqBand ? 0.9f : 0.4f));
                    g.strokePath (path, juce::PathStrokeType (b == fx.eqBand ? 1.6f : 1.1f));
                }

                juce::Path sum;
                for (int i = 0; i < n; ++i)
                {
                    const float x01 = (float) i / (float) (n - 1);
                    const float mag = tew::eqMagnitude (bq, tew::eqBandCount, sr(), xToHz (x01));
                    const float db = mag > 1.0e-8f ? 20.f * std::log10 (mag) : -80.f;
                    const juce::Point<float> pt (r.getX() + x01 * r.getWidth(),
                                                 r.getY() + dbToY (db) * r.getHeight());
                    if (i == 0)
                        sum.startNewSubPath (pt);
                    else
                        sum.lineTo (pt);
                }
                g.setColour (kEqSum);
                g.strokePath (sum, juce::PathStrokeType (1.8f));

                for (int b = 0; b < tew::eqBandCount; ++b)
                {
                    const auto pt = bandPos (params[b], bq[b]);
                    const bool on = params[b].type > tew::eqTypeOff;
                    const bool sel = fx.eqBand == b;
                    g.setColour (kEqBand[b].withAlpha (on ? 1.f : 0.35f));
                    g.fillEllipse (pt.x - 4.5f, pt.y - 4.5f, 9.f, 9.f);
                    if (sel)
                    {
                        g.setColour (kCream);
                        g.drawEllipse (pt.x - 6.f, pt.y - 6.f, 12.f, 12.f, 1.2f);
                    }
                }
            }

            void mouseDown (const juce::MouseEvent& e) override
            {
                tew::EqBandParam params[tew::eqBandCount];
                tew::Biquad bq[tew::eqBandCount];
                readBands (params, bq);
                drag = hitBand (e.position, params, bq);
                if (drag < 0)
                    return;
                fx.eqBand = drag;
                fx.bindEq();
                if (e.getNumberOfClicks() >= 2)
                {
                    fx.resetEqBand (drag);
                    drag = -1;
                    return;
                }
                fx.beginEq (ParamID::fxEqFreq[drag]);
                fx.beginEq (ParamID::fxEqGain[drag]);
                gesturing = true;
            }

            void mouseDoubleClick (const juce::MouseEvent& e) override
            {
                tew::EqBandParam params[tew::eqBandCount];
                tew::Biquad bq[tew::eqBandCount];
                readBands (params, bq);
                const int b = hitBand (e.position, params, bq);
                if (b < 0)
                    return;
                fx.eqBand = b;
                fx.resetEqBand (b);
            }

            void mouseDrag (const juce::MouseEvent& e) override
            {
                if (drag < 0)
                    return;
                auto r = plot();
                const float x01 = r.getWidth() > 1.f ? (e.position.x - r.getX()) / r.getWidth() : 0.f;
                const float y01 = r.getHeight() > 1.f ? (e.position.y - r.getY()) / r.getHeight() : 0.f;
                fx.setEqParam (ParamID::fxEqFreq[drag], xToHz (x01));
                fx.setEqParam (ParamID::fxEqGain[drag], yToDb (y01));
                repaint();
            }

            void mouseUp (const juce::MouseEvent&) override
            {
                if (! gesturing)
                    return;
                fx.endEq (ParamID::fxEqFreq[drag]);
                fx.endEq (ParamID::fxEqGain[drag]);
                gesturing = false;
                drag = -1;
                fx.bindEq();
            }

            void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
            {
                tew::EqBandParam params[tew::eqBandCount];
                tew::Biquad bq[tew::eqBandCount];
                readBands (params, bq);
                int b = hitBand (e.position, params, bq);
                if (b < 0)
                    b = fx.eqBand;
                fx.eqBand = b;
                const char* id = ParamID::fxEqQ[b];
                fx.beginEq (id);
                fx.setEqParam (id, fx.loadEq (id) * (1.f + w.deltaY * 0.25f) + w.deltaY * 0.05f);
                fx.endEq (id);
                fx.bindEq();
            }

            FxRow& fx;
            int drag = -1;
            bool gesturing = false;
            std::vector<float> specDb;
        };

        struct Visual : public juce::Component
        {
            explicit Visual (FxRow& row) : fx (row) {}

            void paint (juce::Graphics& g) override
            {
                auto bounds = getLocalBounds().toFloat();
                g.setColour (juce::Colour (0xff12151a));
                g.fillRoundedRectangle (bounds, 4.f);
                auto r = bounds.reduced (8.f, 10.f);
                g.setColour (juce::Colour (0xff2a2e32));
                g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());

                switch (fx.type)
                {
                    case 0: case 6: case 7: paintSweep (g, r); break;
                    case 1: paintGr (g, r); break;
                    case 2: paintDelay (g, r); break;
                    case 3: paintDist (g, r); break;
                    case 5: paintFilter (g, r); break;
                    case 8: paintReverb (g, r); break;
                    default: break;
                }
            }

            void paintSweep (juce::Graphics& g, juce::Rectangle<float> r)
            {
                const bool flanger = fx.type == 6;
                const bool phaser = fx.type == 7;
                const char* rateId = phaser ? ParamID::fxPhaRate
                                   : flanger ? ParamID::fxFlaRate : ParamID::fxChoRate;
                const char* depthId = phaser ? ParamID::fxPhaDepth
                                    : flanger ? ParamID::fxFlaDepth : ParamID::fxChoDepth;
                const float depth = fx.load (depthId);
                const float rate = fx.load (rateId);
                const float phase = fx.owner.vizPhase * (0.4f + rate * 2.6f);
                juce::Path p;
                const int n = juce::jmax (2, (int) r.getWidth());
                for (int i = 0; i < n; ++i)
                {
                    const float x01 = (float) i / (float) (n - 1);
                    const float y = std::sin ((x01 * 4.f + phase) * 6.2831853f) * depth * r.getHeight() * 0.35f;
                    const juce::Point<float> pt (r.getX() + x01 * r.getWidth(), r.getCentreY() + y);
                    if (i == 0)
                        p.startNewSubPath (pt);
                    else
                        p.lineTo (pt);
                }
                g.setColour (kCream);
                g.strokePath (p, juce::PathStrokeType (1.6f));
            }

            void paintGr (juce::Graphics& g, juce::Rectangle<float> r)
            {
                const float gr = juce::jlimit (0.f, 24.f, fx.owner.proc.fxCompGrDb());
                const float w = r.getWidth() * (gr / 24.f);
                g.setColour (juce::Colour (0xff5a1808));
                g.fillRoundedRectangle (r.removeFromTop (18.f), 3.f);
                g.setColour (kLedOn);
                g.fillRoundedRectangle (r.withWidth (w).withHeight (18.f).translated (0.f, -18.f), 3.f);
                g.setColour (kCream);
                g.setFont (labelFont (11.f));
                g.drawText (juce::String (gr, 1) + " dB", r.translated (0.f, 8.f),
                            juce::Justification::centredLeft, false);
            }

            void paintDelay (juce::Graphics& g, juce::Rectangle<float> r)
            {
                const float fb = fx.load (ParamID::fxDlyFb);
                const bool ping = fx.load (ParamID::fxDlyPing) >= 0.5f;
                const int taps = 1 + juce::roundToInt (fb * 6.f);
                for (int i = 0; i < taps; ++i)
                {
                    const float x = r.getX() + r.getWidth() * ((float) (i + 1) / (float) (taps + 1));
                    const float h = r.getHeight() * (1.f - (float) i * 0.12f);
                    g.setColour (kCream.withAlpha (1.f - (float) i * 0.12f));
                    g.fillRect (x - 2.f, r.getBottom() - h, 4.f, h);
                    if (ping && (i % 2) == 1)
                    {
                        g.setColour (juce::Colour (0xff4c8dff));
                        g.fillEllipse (x - 3.f, r.getY() + 4.f, 6.f, 6.f);
                    }
                }
            }

            void paintDist (juce::Graphics& g, juce::Rectangle<float> r)
            {
                const int kind = juce::roundToInt (fx.load (ParamID::fxDstType));
                const float drive = 1.f + fx.load (ParamID::fxDstDrive) * 12.f;
                const float bias = fx.load (ParamID::fxDstBias) * 0.5f;
                juce::Path p;
                const int n = juce::jmax (2, (int) r.getWidth());
                for (int i = 0; i < n; ++i)
                {
                    const float x01 = (float) i / (float) (n - 1);
                    const float x = (x01 * 2.f - 1.f);
                    float y = 0.f;
                    float v = (x + bias) * drive;
                    if (kind == 1)
                        y = juce::jlimit (-1.f, 1.f, v);
                    else if (kind == 2)
                    {
                        y = v;
                        while (y > 1.f || y < -1.f)
                        {
                            if (y > 1.f) y = 2.f - y;
                            if (y < -1.f) y = -2.f - y;
                        }
                    }
                    else if (kind == 3)
                        y = std::abs (std::tanh (v));
                    else
                        y = std::tanh (v);
                    const juce::Point<float> pt (r.getX() + x01 * r.getWidth(),
                                                 r.getCentreY() - y * r.getHeight() * 0.42f);
                    if (i == 0)
                        p.startNewSubPath (pt);
                    else
                        p.lineTo (pt);
                }
                g.setColour (kCream);
                g.strokePath (p, juce::PathStrokeType (1.6f));
            }

            void paintFilter (juce::Graphics& g, juce::Rectangle<float> r)
            {
                const int kind = juce::roundToInt (fx.load (ParamID::fxFltType));
                const float cut = fx.load (ParamID::fxFltCut, 2000.f);
                const float res = 0.1f + fx.load (ParamID::fxFltRes) * 8.f;
                const bool steep = juce::roundToInt (fx.load (ParamID::fxFltSlope)) >= 1;
                const int eqt = kind == 1 ? tew::eqTypePeak : (kind == 2 ? tew::eqTypeHP : tew::eqTypeLP);
                const float gain = kind == 1 ? juce::jlimit (0.f, 12.f, (res - 0.1f) * 1.4f) : 0.f;
                const float q = juce::jlimit (0.1f, 10.f, res);
                auto bq = tew::eqBiquad (eqt, 44100.f, cut, q, gain);
                juce::Path path;
                const int n = juce::jmax (2, (int) r.getWidth());
                for (int i = 0; i < n; ++i)
                {
                    const float x01 = (float) i / (float) (n - 1);
                    const float hz = 20.f * std::pow (10.f, x01 * 3.f);
                    tew::Biquad one[1] { bq };
                    float mag = tew::eqMagnitude (one, 1, 44100.f, hz);
                    if (steep)
                        mag *= mag;
                    const float db = mag > 1.0e-8f ? 20.f * std::log10 (mag) : -60.f;
                    const float y01 = 1.f - juce::jlimit (0.f, 1.f, (db + 24.f) / 36.f);
                    const juce::Point<float> pt (r.getX() + x01 * r.getWidth(), r.getY() + y01 * r.getHeight());
                    if (i == 0)
                        path.startNewSubPath (pt);
                    else
                        path.lineTo (pt);
                }
                g.setColour (kCream);
                g.strokePath (path, juce::PathStrokeType (1.6f));
            }

            void paintReverb (juce::Graphics& g, juce::Rectangle<float> r)
            {
                const float size = fx.load (ParamID::fxRevSize);
                const float damp = fx.load (ParamID::fxRevDamp);
                const float pre = fx.load (ParamID::fxRevPre) / 200.f;
                juce::Path p;
                const int n = juce::jmax (2, (int) r.getWidth());
                for (int i = 0; i < n; ++i)
                {
                    const float x01 = (float) i / (float) (n - 1);
                    const float t = juce::jmax (0.f, x01 - pre * 0.25f);
                    const float env = t <= 0.f ? 0.f : std::exp (-t * (1.4f + damp * 4.f) / (0.15f + size));
                    const juce::Point<float> pt (r.getX() + x01 * r.getWidth(),
                                                 r.getBottom() - env * r.getHeight() * 0.85f);
                    if (i == 0)
                        p.startNewSubPath (pt);
                    else
                        p.lineTo (pt);
                }
                g.setColour (kCream);
                g.strokePath (p, juce::PathStrokeType (1.6f));
            }

            FxRow& fx;
        };

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

        int knobRows() const
        {
            int vis = 0;
            for (auto* k : knobs)
                if (k->isVisible())
                    ++vis;
            const int cols = juce::jmax (1, juce::jmin (vis, 6));
            return vis <= 0 ? 1 : (vis + cols - 1) / cols;
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (8, 6);
            auto vizR = r.removeFromTop (kVizH);
            if (type == 4 && eqPlot != nullptr)
                eqPlot->setBounds (vizR);
            else
                viz.setBounds (vizR);
            r.removeFromTop (6);
            int vis = 0;
            for (auto* k : knobs)
                if (k->isVisible())
                    ++vis;
            if (vis <= 0)
                return;
            const int cols = juce::jmin (vis, 6);
            const int rowsN = (vis + cols - 1) / cols;
            const int rowH = juce::jmax (1, r.getHeight() / rowsN);
            const int colW = juce::jmax (1, r.getWidth() / cols);
            int i = 0;
            for (auto* k : knobs)
            {
                if (! k->isVisible())
                    continue;
                const int row = i / cols;
                const int col = i % cols;
                const int colsThis = (row == rowsN - 1) ? (vis - row * cols) : cols;
                const int w = row == rowsN - 1 ? r.getWidth() / juce::jmax (1, colsThis) : colW;
                k->setBounds (r.getX() + col * w, r.getY() + row * rowH, w, rowH);
                ++i;
            }
        }

        EffectsPage& owner;
        int type = 0;
        int eqBand = 0;
        Visual viz;
        std::unique_ptr<EqPlot> eqPlot;
        juce::OwnedArray<FxKnob> knobs;
    };

    struct RailButton : public juce::Component,
                       public juce::DragAndDropTarget
    {
        RailButton (EffectsPage& page, int t) : owner (page), type (t)
        {
            on.setButtonText ("On");
            on.setComponentID ("led");
            addAndMakeVisible (on);
            onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                owner.proc.apvts, ParamID::fxOnIds[type], on);
        }

        void bindClick()
        {
            on.onClick = [this]
            {
                if (on.getToggleState())
                    owner.select (type);
                owner.refresh();
            };
        }

        void setOn (bool v)
        {
            if (lit == v)
                return;
            lit = v;
            repaint();
        }

        juce::Rectangle<int> gripBounds() const
        {
            return { 0, 0, kGripW, getHeight() };
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            g.setColour (lit ? juce::Colour (0xff3a3a3a) : kKnob);
            g.fillRoundedRectangle (r, 4.f);
            g.setColour (owner.selected == type ? kCream : (lit ? kInk : kInk.withAlpha (0.45f)));
            g.drawRoundedRectangle (r.reduced (0.5f), 4.f, owner.selected == type ? 1.8f : 1.f);

            auto grip = juce::Rectangle<float> ((float) kGripW, r.getHeight()).reduced (3.f, 6.f);
            g.setColour (kCream.withAlpha (0.55f));
            for (int row = 0; row < 3; ++row)
                for (int col = 0; col < 2; ++col)
                    g.fillEllipse (grip.getX() + (float) col * 5.f,
                                   grip.getY() + (float) row * (grip.getHeight() / 3.f),
                                   2.4f, 2.4f);

            drawIcon (g, juce::Rectangle<float> (r.getRight() - 26.f, r.getY() + 4.f, 20.f, r.getHeight() - 8.f));

            g.setColour (kCream);
            g.setFont (labelFont (10.f));
            g.drawFittedText (ParamID::fxNames[type],
                              juce::Rectangle<int> (kGripW + 2, (int) r.getY() + 2,
                                                    (int) r.getWidth() - kGripW - 48, (int) r.getHeight() / 2),
                              juce::Justification::centredLeft, 2);
        }

        void drawIcon (juce::Graphics& g, juce::Rectangle<float> box)
        {
            g.setColour (kCream.withAlpha (lit ? 1.f : 0.65f));
            juce::Path p;
            const float x = box.getX(), y = box.getY(), w = box.getWidth(), h = box.getHeight();
            switch (type)
            {
                case 0:
                    g.drawEllipse (x, y + h * 0.25f, w * 0.55f, h * 0.5f, 1.4f);
                    g.drawEllipse (x + w * 0.22f, y + h * 0.25f, w * 0.55f, h * 0.5f, 1.4f);
                    g.drawEllipse (x + w * 0.44f, y + h * 0.25f, w * 0.55f, h * 0.5f, 1.4f);
                    break;
                case 1:
                    for (int i = 0; i < 4; ++i)
                    {
                        const float bh = h * (0.25f + 0.15f * (float) (i % 3));
                        g.fillRect (x + 3.f * (float) i, y + (h - bh) * 0.5f, 2.2f, bh);
                    }
                    break;
                case 2:
                    for (int i = 0; i < 4; ++i)
                        g.fillRect (x + 4.f * (float) i, y + h * 0.2f, 2.f, h * (0.6f - 0.12f * (float) i));
                    break;
                case 3:
                    p.addTriangle (x + w * 0.5f, y + 2.f, x + w - 2.f, y + h * 0.5f, x + w * 0.5f, y + h - 2.f);
                    p.addTriangle (x + w * 0.5f, y + 2.f, x + 2.f, y + h * 0.5f, x + w * 0.5f, y + h - 2.f);
                    g.strokePath (p, juce::PathStrokeType (1.3f));
                    break;
                case 4:
                    g.drawLine (x, y + h * 0.6f, x + w, y + h * 0.45f, 1.3f);
                    g.fillEllipse (x + w * 0.2f - 2.f, y + h * 0.55f - 2.f, 4.f, 4.f);
                    g.fillEllipse (x + w * 0.55f - 2.f, y + h * 0.4f - 2.f, 4.f, 4.f);
                    break;
                case 5:
                    p.startNewSubPath (x, y + h * 0.75f);
                    p.quadraticTo (x + w * 0.45f, y + 2.f, x + w, y + h * 0.7f);
                    g.strokePath (p, juce::PathStrokeType (1.4f));
                    break;
                case 6:
                    for (int i = 0; i < 5; ++i)
                        g.drawLine (x + 3.5f * (float) i, y + h * 0.2f, x + 3.5f * (float) i, y + h * 0.8f, 1.2f);
                    break;
                case 7:
                    p.startNewSubPath (x, y + h * 0.5f);
                    p.quadraticTo (x + w * 0.25f, y + 2.f, x + w * 0.5f, y + h * 0.5f);
                    p.quadraticTo (x + w * 0.75f, y + h - 2.f, x + w, y + h * 0.5f);
                    g.strokePath (p, juce::PathStrokeType (1.4f));
                    break;
                case 8:
                    g.drawRect (x + 3.f, y + h * 0.3f, w * 0.55f, h * 0.5f, 1.2f);
                    g.drawLine (x + 3.f, y + h * 0.3f, x + 8.f, y + h * 0.15f, 1.2f);
                    g.drawLine (x + 3.f + w * 0.55f, y + h * 0.3f, x + 8.f + w * 0.55f, y + h * 0.15f, 1.2f);
                    g.drawLine (x + 8.f, y + h * 0.15f, x + 8.f + w * 0.55f, y + h * 0.15f, 1.2f);
                    break;
                default:
                    break;
            }
        }

        void resized() override
        {
            auto r = getLocalBounds();
            r.removeFromLeft (kGripW);
            r.removeFromRight (24);
            on.setBounds (r.removeFromBottom (18).reduced (2, 1));
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            gripDown = gripBounds().contains (e.getPosition());
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (gripDown && e.getDistanceFromDragStart() > 6)
                if (auto* c = juce::DragAndDropContainer::findParentDragContainerFor (this))
                    c->startDragging ("fx:" + juce::String (type), this);
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            const bool wasGrip = gripDown;
            gripDown = false;
            if (! e.mouseWasClicked())
                return;
            if (! wasGrip)
                owner.select (type);
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
        bool lit = false;
        bool gripDown = false;
        juce::ToggleButton on;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
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

    void layoutDetail()
    {
        const int w = juce::jmax (1, viewport.getWidth());
        for (int t = 0; t < tew::fxCount; ++t)
            rows[t]->setVisible (t == selected);
        if (selected < 0)
        {
            stack.setSize (w, 1);
            return;
        }
        const int h = kVizH + rows[selected]->knobRows() * kKnobRowH + 16;
        stack.setSize (w, juce::jmax (viewport.getHeight(), h));
        rows[selected]->setBounds (0, 0, w, stack.getHeight());
        rows[selected]->toFront (false);
        rows[selected]->syncDelay();
    }

    TEW03AudioProcessor& proc;
    juce::Viewport viewport;
    juce::Component stack;
    juce::OwnedArray<RailButton> rails;
    juce::OwnedArray<FxRow> rows;
    juce::Rectangle<int> railArea;
    juce::Image logo;
    int railOrder[tew::fxCount] {};
    int nRail = 0;
    int selected = -1;
    float vizPhase = 0.f;
};

inline const juce::Colour EffectsPage::kInk { 0xff111111 };
inline const juce::Colour EffectsPage::kCream { 0xfff4ead0 };
inline const juce::Colour EffectsPage::kKnob { 0xff1a1a1a };
inline const juce::Colour EffectsPage::kLedOn { 0xffff2200 };
inline const juce::Colour EffectsPage::kLedOff { 0xff5a1808 };
inline const juce::Colour EffectsPage::kLaneBlack { 0xff2a2a2a };
inline const juce::Colour EffectsPage::kEqSum { 0xffe8b84a };
inline const juce::Colour EffectsPage::kEqBand[tew::eqBandCount] {
    juce::Colour (0xffe07a2e),
    juce::Colour (0xff4aa3ff),
    juce::Colour (0xffc86ad8),
    juce::Colour (0xff3dcc8c),
    juce::Colour (0xffe8c200)
};
