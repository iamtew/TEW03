#pragma once

#include "../PluginProcessor.h"
#include "../dsp/EqBiquad.h"

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
    static const juce::Colour kEqSum;
    static const juce::Colour kEqBand[tew::eqBandCount];

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
                    eqPlot = std::make_unique<EqPlot> (*this);
                    addAndMakeVisible (*eqPlot);
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
            if (type == 4 && eqPlot != nullptr)
            {
                auto knobArea = r.removeFromRight (280);
                eqPlot->setBounds (r.reduced (2, 2));
                r = knobArea;
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
        std::unique_ptr<EqPlot> eqPlot;
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
inline const juce::Colour EffectsPage::kEqSum { 0xffe8b84a };
inline const juce::Colour EffectsPage::kEqBand[tew::eqBandCount] {
    juce::Colour (0xffe07a2e),
    juce::Colour (0xff4aa3ff),
    juce::Colour (0xffc86ad8),
    juce::Colour (0xff3dcc8c),
    juce::Colour (0xffe8c200)
};
