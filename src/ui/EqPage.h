#pragma once

#include "../PluginProcessor.h"
#include "../dsp/EqBiquad.h"

#include <JuceHeader.h>

class EqPage : public juce::Component,
               private juce::Timer
{
public:
    explicit EqPage (TEW03AudioProcessor& p) : proc (p), curve (*this), inspector (*this)
    {
        preRow.setup (*this, false);
        postRow.setup (*this, true);
        addAndMakeVisible (preRow);
        addAndMakeVisible (postRow);
        preOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            proc.apvts, ParamID::preEqOn, preRow.on);
        postOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            proc.apvts, ParamID::postEqOn, postRow.on);

        resetBtn.setButtonText ("Reset");
        resetBtn.setComponentID ("plain");
        resetBtn.setClickingTogglesState (false);
        resetBtn.onClick = [this] { resetEq(); };
        addAndMakeVisible (resetBtn);

        addAndMakeVisible (curve);
        addAndMakeVisible (inspector);
        inspector.bind();
        startTimerHz (24);
    }

    ~EqPage() override { stopTimer(); }

    void resized() override
    {
        auto r = getLocalBounds();
        auto rail = r.removeFromLeft (102);
        preRow.setBounds (rail.removeFromTop (28).reduced (2, 1));
        postRow.setBounds (rail.removeFromTop (28).reduced (2, 1));
        resetBtn.setBounds (rail.removeFromTop (22).reduced (4, 1));
        inspector.setBounds (rail);
        curve.setBounds (r);
    }

    TEW03AudioProcessor& proc;

private:
    static const juce::Colour kCream;
    static const juce::Colour kGraph;
    static const juce::Colour kSum;
    static const juce::Colour kBandCol[tew::eqBandCount];

    static juce::Font labelFont (float h)
    {
        return juce::Font (juce::FontOptions (h).withStyle ("Bold"));
    }

    void timerCallback() override
    {
        inspector.sync();
        curve.repaint();
    }

    void setEditPost (bool post)
    {
        if (editPostOn == post)
            return;
        editPostOn = post;
        preRow.repaint();
        postRow.repaint();
        inspector.bind();
        curve.repaint();
    }

    void selectBand (int b)
    {
        selBand = juce::jlimit (0, tew::eqBandCount - 1, b);
        inspector.bind();
        repaint();
    }

    const char* typeId (int b) const
    {
        return editPostOn ? ParamID::postEqType[b] : ParamID::preEqType[b];
    }
    const char* freqId (int b) const
    {
        return editPostOn ? ParamID::postEqFreq[b] : ParamID::preEqFreq[b];
    }
    const char* gainId (int b) const
    {
        return editPostOn ? ParamID::postEqGain[b] : ParamID::preEqGain[b];
    }
    const char* qId (int b) const
    {
        return editPostOn ? ParamID::postEqQ[b] : ParamID::preEqQ[b];
    }
    const char* onId() const
    {
        return editPostOn ? ParamID::postEqOn : ParamID::preEqOn;
    }

    void setParam (const char* id, float v)
    {
        auto* p = dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter (id));
        if (p == nullptr)
            return;
        p->setValueNotifyingHost (p->convertTo0to1 (p->getNormalisableRange().snapToLegalValue (v)));
    }

    void beginParam (const char* id)
    {
        if (auto* p = proc.apvts.getParameter (id))
            p->beginChangeGesture();
    }
    void endParam (const char* id)
    {
        if (auto* p = proc.apvts.getParameter (id))
            p->endChangeGesture();
    }

    float loadId (const char* id) const
    {
        auto* v = proc.apvts.getRawParameterValue (id);
        return v != nullptr ? v->load() : 0.f;
    }

    void setType (int b, int type)
    {
        auto* p = proc.apvts.getParameter (typeId (b));
        if (p == nullptr)
            return;
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) type));
        p->endChangeGesture();
    }

    void resetParamId (const char* id)
    {
        auto* p = proc.apvts.getParameter (id);
        if (p == nullptr)
            return;
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getDefaultValue());
        p->endChangeGesture();
    }

    void resetBand (int b)
    {
        resetParamId (typeId (b));
        resetParamId (freqId (b));
        resetParamId (gainId (b));
        resetParamId (qId (b));
        inspector.bind();
        curve.repaint();
    }

    // Currently edited stack only (Pre or Post), not both.
    void resetEq()
    {
        resetParamId (onId());
        for (int b = 0; b < tew::eqBandCount; ++b)
        {
            resetParamId (typeId (b));
            resetParamId (freqId (b));
            resetParamId (gainId (b));
            resetParamId (qId (b));
        }
        inspector.bind();
        curve.repaint();
    }

    // LED enables this stack; click the name to edit it. Selecting does not turn it on.
    struct StackRow : public juce::Component
    {
        void setup (EqPage& page, bool post)
        {
            owner = &page;
            isPost = post;
            // LnF only draws the LED blob when the label is 2+ chars; spaces keep it LED-only.
            on.setButtonText ("  ");
            addAndMakeVisible (on);
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            const bool sel = owner != nullptr && owner->editPostOn == isPost;
            g.setColour (sel ? juce::Colour (0xff2a2a2a) : juce::Colour (0xff151515));
            g.fillRoundedRectangle (r, 4.f);
            g.setColour (sel ? kCream : kCream.withAlpha (0.45f));
            if (sel)
                g.drawRoundedRectangle (r.reduced (0.8f), 4.f, 1.6f);
            g.setFont (labelFont (14.f));
            g.drawText (isPost ? "POST" : "PRE",
                        getLocalBounds().withTrimmedLeft (24),
                        juce::Justification::centredLeft, false);
        }

        void resized() override
        {
            on.setBounds (getLocalBounds().removeFromLeft (22).reduced (1, 3));
        }

        void mouseDown (const juce::MouseEvent&) override
        {
            if (owner != nullptr)
                owner->setEditPost (isPost);
        }

        EqPage* owner = nullptr;
        bool isPost = false;
        juce::ToggleButton on;
    };

    struct BandKnob : public juce::Component
    {
        BandKnob()
        {
            label.setJustificationType (juce::Justification::centred);
            label.setFont (labelFont (11.f));
            label.setColour (juce::Label::textColourId, kCream);
            label.setBorderSize ({});
            addAndMakeVisible (label);
            value.setJustificationType (juce::Justification::centred);
            value.setFont (labelFont (11.f));
            value.setColour (juce::Label::textColourId, kCream);
            value.setBorderSize ({});
            addAndMakeVisible (value);
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            slider.onValueChange = [this] { refreshValue(); };
            addAndMakeVisible (slider);
        }

        void refreshValue()
        {
            const auto v = slider.getValue();
            value.setText (label.getText() == "GAIN" ? juce::String (v, 2)
                                                     : slider.getTextFromValue (v),
                           juce::dontSendNotification);
        }

        void bind (TEW03AudioProcessor& p, const char* id, const juce::String& name)
        {
            label.setText (name, juce::dontSendNotification);
            att.reset();
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (id)))
                slider.setDoubleClickReturnValue (true, ranged->convertFrom0to1 (ranged->getDefaultValue()));
            att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                p.apvts, id, slider);
            refreshValue();
        }

        void resized() override
        {
            auto r = getLocalBounds();
            const int ks = juce::jmin (r.getWidth() - 2, 56);
            slider.setBounds (r.removeFromTop (ks).withSizeKeepingCentre (ks, ks));
            label.setBounds (r.removeFromTop (12));
            value.setBounds (r.removeFromTop (12));
        }

        juce::Label label, value;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    static void drawEqTypePath (juce::Graphics& g, juce::Rectangle<float> r, int type, juce::Colour c)
    {
        r = r.reduced (1.2f);
        if (r.getWidth() < 4.f || r.getHeight() < 4.f)
            return;

        const float x0 = r.getX(), x1 = r.getRight();
        const float y0 = r.getY(), y1 = r.getBottom();
        const float xm = r.getCentreX(), ym = r.getCentreY();
        juce::Path p;

        switch (type)
        {
            case tew::eqTypePeak:
                p.startNewSubPath (x0, ym);
                p.quadraticTo (x0 + r.getWidth() * 0.32f, ym, xm, y0);
                p.quadraticTo (x1 - r.getWidth() * 0.32f, ym, x1, ym);
                break;
            case tew::eqTypeLowShelf:
                p.startNewSubPath (x0, y0 + r.getHeight() * 0.22f);
                p.lineTo (xm - 1.5f, y0 + r.getHeight() * 0.22f);
                p.lineTo (xm + 1.5f, y1 - r.getHeight() * 0.22f);
                p.lineTo (x1, y1 - r.getHeight() * 0.22f);
                break;
            case tew::eqTypeHighShelf:
                p.startNewSubPath (x0, y1 - r.getHeight() * 0.22f);
                p.lineTo (xm - 1.5f, y1 - r.getHeight() * 0.22f);
                p.lineTo (xm + 1.5f, y0 + r.getHeight() * 0.22f);
                p.lineTo (x1, y0 + r.getHeight() * 0.22f);
                break;
            case tew::eqTypeHP:
                p.startNewSubPath (x0, y1);
                p.lineTo (xm - 2.f, y1);
                p.quadraticTo (xm, y1, xm, ym);
                p.lineTo (xm, y0 + 1.5f);
                p.lineTo (x1, y0 + 1.5f);
                break;
            case tew::eqTypeLP:
                p.startNewSubPath (x0, y0 + 1.5f);
                p.lineTo (xm, y0 + 1.5f);
                p.lineTo (xm, ym);
                p.quadraticTo (xm, y1, xm + 2.f, y1);
                p.lineTo (x1, y1);
                break;
            default: // Off
                p.startNewSubPath (x0 + r.getWidth() * 0.18f, ym);
                p.lineTo (x1 - r.getWidth() * 0.18f, ym);
                break;
        }

        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    struct TypeBtn : public juce::Button
    {
        int type = 0;

        TypeBtn() : juce::Button ({})
        {
            setClickingTogglesState (true);
            setRadioGroupId (2);
        }

        void paintButton (juce::Graphics& g, bool, bool) override
        {
            auto b = getLocalBounds().toFloat().reduced (1.f);
            const bool on = getToggleState();
            g.setColour (on ? kSum.withAlpha (0.85f) : juce::Colour (0xff151515));
            g.fillRoundedRectangle (b, 3.f);
            drawEqTypePath (g, b.reduced (2.f), type, on ? juce::Colours::white : kCream);
        }
    };

    struct Inspector : public juce::Component
    {
        explicit Inspector (EqPage& page) : owner (page)
        {
            prev.setButtonText ("<");
            next.setButtonText (">");
            prev.setClickingTogglesState (false);
            next.setClickingTogglesState (false);
            prev.onClick = [this] { owner.selectBand (owner.selBand - 1); };
            next.onClick = [this] { owner.selectBand (owner.selBand + 1); };
            index.setJustificationType (juce::Justification::centred);
            index.setFont (labelFont (16.f));
            index.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (prev);
            addAndMakeVisible (index);
            addAndMakeVisible (next);
            static const char* kTips[] = {
                "Off", "Peak", "Low Shelf", "High Shelf", "High Pass", "Low Pass"
            };
            for (int i = 0; i < 6; ++i)
            {
                types[i].type = i;
                types[i].setTooltip (kTips[i]);
                types[i].onClick = [this, i]
                {
                    if (types[i].getToggleState())
                        owner.setType (owner.selBand, i);
                    sync();
                };
                addAndMakeVisible (types[i]);
            }
            addAndMakeVisible (freq);
            addAndMakeVisible (gain);
            addAndMakeVisible (q);
        }

        void bind()
        {
            const int b = owner.selBand;
            index.setText (juce::String (b + 1), juce::dontSendNotification);
            index.setColour (juce::Label::textColourId, kBandCol[b]);
            freq.bind (owner.proc, owner.freqId (b), "FREQ");
            gain.bind (owner.proc, owner.gainId (b), "GAIN");
            q.bind (owner.proc, owner.qId (b), "Q");
            sync();
            repaint();
        }

        void sync()
        {
            const int t = juce::roundToInt (owner.loadId (owner.typeId (owner.selBand)));
            for (int i = 0; i < 6; ++i)
                types[i].setToggleState (i == t, juce::dontSendNotification);
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            g.setColour (juce::Colour (0xee1a1a1a));
            g.fillRoundedRectangle (r, 6.f);
            g.setColour (kBandCol[owner.selBand]);
            g.drawRoundedRectangle (r.reduced (0.8f), 6.f, 1.6f);
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (4, 4);
            auto row = r.removeFromTop (22);
            prev.setBounds (row.removeFromLeft (22).reduced (1));
            next.setBounds (row.removeFromRight (22).reduced (1));
            index.setBounds (row);
            auto pad = r.removeFromTop (54);
            const int cw = juce::jmax (1, pad.getWidth() / 2);
            const int ch = juce::jmax (1, pad.getHeight() / 3);
            for (int i = 0; i < 6; ++i)
                types[i].setBounds (pad.getX() + (i % 2) * cw,
                                    pad.getY() + (i / 2) * ch,
                                    cw, ch);
            // Fixed cells — don't stretch into the leftover rail (that parked the labels miles below each knob).
            const int knobH = 80; // 56 rotary + 12 + 12
            freq.setBounds (r.removeFromTop (knobH));
            gain.setBounds (r.removeFromTop (knobH));
            q.setBounds (r.removeFromTop (knobH));
        }

        EqPage& owner;
        juce::ToggleButton prev, next;
        juce::Label index;
        TypeBtn types[6];
        BandKnob freq, gain, q;
    };

    struct Curve : public juce::Component
    {
        explicit Curve (EqPage& page) : owner (page) { setMouseCursor (juce::MouseCursor::CrosshairCursor); }

        float sr() const
        {
            const double s = owner.proc.getSampleRate();
            return s > 1.0 ? (float) s : 44100.f;
        }

        juce::Rectangle<float> plot() const
        {
            return getLocalBounds().toFloat().reduced (28.f, 18.f);
        }

        static float xToHz (float x01)
        {
            return 20.f * std::pow (10.f, juce::jlimit (0.f, 1.f, x01) * 3.f);
        }
        static float hzToX (float hz)
        {
            hz = juce::jlimit (20.f, 20000.f, hz);
            return std::log10 (hz / 20.f) / 3.f;
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
            if (p.type == tew::eqTypeHP || p.type == tew::eqTypeLP)
            {
                tew::Biquad one[1] { bq };
                const float mag = tew::eqMagnitude (one, 1, sr(), p.hz);
                db = mag > 1.0e-8f ? 20.f * std::log10 (mag) : -18.f;
            }
            return { r.getX() + hzToX (p.hz) * r.getWidth(),
                     r.getY() + dbToY (db) * r.getHeight() };
        }

        int hitBand (juce::Point<float> pos, const tew::EqBandParam* params, const tew::Biquad* bq) const
        {
            int best = -1;
            float bestD = 16.f * 16.f;
            for (int b = 0; b < tew::eqBandCount; ++b)
            {
                if (params[b].type <= tew::eqTypeOff)
                    continue;
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

        void paint (juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            g.setColour (kGraph);
            g.fillRoundedRectangle (bounds, 4.f);

            auto r = plot();
            g.setColour (juce::Colour (0xff2a2e32));
            const float zeroY = r.getY() + dbToY (0.f) * r.getHeight();
            g.drawHorizontalLine ((int) zeroY, r.getX(), r.getRight());
            for (int db : { 12, 6, -6, -12 })
                g.drawHorizontalLine ((int) (r.getY() + dbToY ((float) db) * r.getHeight()),
                                      r.getX(), r.getRight());
            const float gridHz[] = { 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f };
            for (float hz : gridHz)
            {
                const float x = r.getX() + hzToX (hz) * r.getWidth();
                g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
            }

            const int n = juce::jmax (2, (int) r.getWidth());
            if ((int) specDb.size() != n)
                specDb.resize ((size_t) n);
            owner.proc.eqAnalyser().copyLogDb (specDb.data(), n);
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

            tew::EqBandParam params[tew::eqBandCount];
            tew::readEqBands (owner.proc.apvts, owner.editPostOn, params);
            tew::Biquad bq[tew::eqBandCount];
            for (int b = 0; b < tew::eqBandCount; ++b)
                bq[b] = tew::eqBiquad (params[b].type, sr(), params[b].hz, params[b].q, params[b].gainDb);

            auto strokeBand = [&] (int b, float alpha, float thick)
            {
                if (params[b].type <= tew::eqTypeOff)
                    return;
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
                g.setColour (kBandCol[b].withAlpha (alpha));
                g.strokePath (path, juce::PathStrokeType (thick));
            };
            for (int b = 0; b < tew::eqBandCount; ++b)
                strokeBand (b, b == owner.selBand ? 0.85f : 0.4f, b == owner.selBand ? 1.8f : 1.2f);

            juce::Path sum;
            juce::Path fill;
            const bool on = tew::eqEnabled (owner.proc.apvts, owner.editPostOn);
            for (int i = 0; i < n; ++i)
            {
                const float x01 = (float) i / (float) (n - 1);
                const float mag = tew::eqMagnitude (bq, tew::eqBandCount, sr(), xToHz (x01));
                const float db = mag > 1.0e-8f ? 20.f * std::log10 (mag) : -80.f;
                const juce::Point<float> pt (r.getX() + x01 * r.getWidth(),
                                             r.getY() + dbToY (db) * r.getHeight());
                if (i == 0)
                {
                    sum.startNewSubPath (pt);
                    fill.startNewSubPath (pt.x, zeroY);
                    fill.lineTo (pt);
                }
                else
                {
                    sum.lineTo (pt);
                    fill.lineTo (pt);
                }
            }
            fill.lineTo (r.getRight(), zeroY);
            fill.closeSubPath();
            g.setColour (kSum.withAlpha (on ? 0.16f : 0.06f));
            g.fillPath (fill);
            g.setColour (kSum.withAlpha (on ? 1.f : 0.35f));
            g.strokePath (sum, juce::PathStrokeType (2.f));

            if (hoverX >= 0.f)
            {
                g.setColour (juce::Colour (0x66c8d0d8));
                g.drawLine (hoverX, r.getY(), hoverX, r.getBottom(), 1.f);
            }

            g.setFont (labelFont (10.f));
            for (int b = 0; b < tew::eqBandCount; ++b)
            {
                if (params[b].type <= tew::eqTypeOff)
                    continue;
                const auto pt = bandPos (params[b], bq[b]);
                const bool sel = owner.selBand == b;
                g.setColour (kBandCol[b]);
                g.fillEllipse (pt.x - (sel ? 6.f : 5.f), pt.y - (sel ? 6.f : 5.f),
                               sel ? 12.f : 10.f, sel ? 12.f : 10.f);
                if (sel)
                {
                    g.setColour (kCream);
                    g.drawEllipse (pt.x - 7.f, pt.y - 7.f, 14.f, 14.f, 1.4f);
                }
                g.setColour (juce::Colour (0xff151515));
                g.setFont (labelFont (sel ? 10.f : 9.f));
                g.drawText (juce::String (b + 1),
                            (int) pt.x - 6, (int) pt.y - 6, 12, 12,
                            juce::Justification::centred, false);
            }

            g.setColour (juce::Colour (0xff8a9098));
            g.setFont (labelFont (10.f));
            auto freqLabel = [&] (float hz, const char* text)
            {
                g.drawText (text, (int) (r.getX() + hzToX (hz) * r.getWidth()) - 14,
                            (int) r.getBottom() + 2, 28, 12, juce::Justification::centred, false);
            };
            freqLabel (20.f, "20");
            freqLabel (100.f, "100");
            freqLabel (1000.f, "1k");
            freqLabel (10000.f, "10k");
            freqLabel (20000.f, "20k");
            g.drawText ("+12", (int) r.getRight() + 2, (int) (r.getY() + dbToY (12.f) * r.getHeight()) - 6,
                        24, 12, juce::Justification::centredLeft, false);
            g.drawText ("0", (int) r.getRight() + 2, (int) zeroY - 6,
                        24, 12, juce::Justification::centredLeft, false);
            g.drawText ("-12", (int) r.getRight() + 2, (int) (r.getY() + dbToY (-12.f) * r.getHeight()) - 6,
                        24, 12, juce::Justification::centredLeft, false);
        }

        void mouseMove (const juce::MouseEvent& e) override
        {
            hoverX = e.position.x;
            repaint();
        }
        void mouseExit (const juce::MouseEvent&) override
        {
            hoverX = -1.f;
            repaint();
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            tew::EqBandParam params[tew::eqBandCount];
            tew::readEqBands (owner.proc.apvts, owner.editPostOn, params);
            tew::Biquad bq[tew::eqBandCount];
            for (int b = 0; b < tew::eqBandCount; ++b)
                bq[b] = tew::eqBiquad (params[b].type, sr(), params[b].hz, params[b].q, params[b].gainDb);
            drag = hitBand (e.position, params, bq);
            if (drag < 0)
                return;
            owner.selectBand (drag);
            if (e.getNumberOfClicks() >= 2)
            {
                owner.resetBand (drag);
                drag = -1;
                return;
            }
            owner.beginParam (owner.freqId (drag));
            owner.beginParam (owner.gainId (drag));
            gesturing = true;
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            hoverX = e.position.x;
            if (drag < 0)
                return;
            auto r = plot();
            const float x01 = r.getWidth() > 1.f ? (e.position.x - r.getX()) / r.getWidth() : 0.f;
            const float y01 = r.getHeight() > 1.f ? (e.position.y - r.getY()) / r.getHeight() : 0.f;
            owner.setParam (owner.freqId (drag), xToHz (x01));
            owner.setParam (owner.gainId (drag), yToDb (y01));
            repaint();
        }

        void mouseUp (const juce::MouseEvent&) override
        {
            if (! gesturing)
                return;
            owner.endParam (owner.freqId (drag));
            owner.endParam (owner.gainId (drag));
            gesturing = false;
            drag = -1;
            owner.inspector.bind();
        }

        void mouseDoubleClick (const juce::MouseEvent& e) override
        {
            tew::EqBandParam params[tew::eqBandCount];
            tew::readEqBands (owner.proc.apvts, owner.editPostOn, params);
            tew::Biquad bq[tew::eqBandCount];
            for (int b = 0; b < tew::eqBandCount; ++b)
                bq[b] = tew::eqBiquad (params[b].type, sr(), params[b].hz, params[b].q, params[b].gainDb);
            const int b = hitBand (e.position, params, bq);
            if (b < 0)
                return;
            owner.resetBand (b);
        }

        void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
        {
            tew::EqBandParam params[tew::eqBandCount];
            tew::readEqBands (owner.proc.apvts, owner.editPostOn, params);
            tew::Biquad bq[tew::eqBandCount];
            for (int b = 0; b < tew::eqBandCount; ++b)
                bq[b] = tew::eqBiquad (params[b].type, sr(), params[b].hz, params[b].q, params[b].gainDb);
            int b = hitBand (e.position, params, bq);
            if (b < 0)
                b = owner.selBand;
            const char* id = owner.qId (b);
            owner.beginParam (id);
            owner.setParam (id, owner.loadId (id) * (1.f + w.deltaY * 0.25f) + w.deltaY * 0.05f);
            owner.endParam (id);
            owner.selectBand (b);
        }

        EqPage& owner;
        int drag = -1;
        bool gesturing = false;
        float hoverX = -1.f;
        std::vector<float> specDb;
    };

    StackRow preRow, postRow;
    juce::ToggleButton resetBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preOnAtt, postOnAtt;
    Curve curve;
    Inspector inspector;
    bool editPostOn = false;
    int selBand = 0;
};

inline const juce::Colour EqPage::kCream { 0xfff4ead0 };
inline const juce::Colour EqPage::kGraph { 0xff12151a };
inline const juce::Colour EqPage::kSum { 0xffe8b84a };
inline const juce::Colour EqPage::kBandCol[tew::eqBandCount] {
    juce::Colour (0xffe07a2e),
    juce::Colour (0xff4aa3ff),
    juce::Colour (0xffc86ad8),
    juce::Colour (0xff3dcc8c),
    juce::Colour (0xffe8c200)
};
