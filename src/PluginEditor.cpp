#include "PluginEditor.h"

namespace
{
constexpr int kEditorW = 900;
constexpr int kEditorH = 258;
constexpr int kPad = 6;
constexpr int kTitleH = 22;
constexpr int kSeqW = 236;
constexpr int kSectionH = 14;
constexpr int kLedH = 10;
constexpr int kPitchH = 28;
constexpr int kToggleH = 18;
constexpr int kStripH = kLedH + kPitchH + kToggleH * 2 + 4;
constexpr int kLabelH = 14;
constexpr int kNoteMin = 24;
constexpr int kNoteMax = 60;

const juce::Colour kChassis { 0xffe8c200 };
const juce::Colour kChassisDark { 0xffc9a400 };
const juce::Colour kCream { 0xfff4ead0 };
const juce::Colour kInk { 0xff111111 };
const juce::Colour kKnob { 0xff1a1a1a };
const juce::Colour kChrome { 0xffd4d8dc };
const juce::Colour kChromeHi { 0xfff2f4f6 };
const juce::Colour kLedOn { 0xffff2200 };
const juce::Colour kLedOff { 0xff5a1808 };

juce::String noteText (int note)
{
    if (note < 0)
        return "--";
    return juce::MidiMessage::getMidiNoteName (note, true, true, 4);
}

int nudgeNote (int note, int delta)
{
    const int from = note < 0 ? 36 : note;
    return juce::jlimit (kNoteMin, kNoteMax, from + delta);
}

juce::Font boldFont (float h)
{
    return juce::Font (juce::FontOptions (h).withStyle ("Bold"));
}

juce::String uiLabel (const juce::String& id)
{
    if (id == ParamID::cutoff)      return "Cutoff";
    if (id == ParamID::resonance)   return "Resonance";
    if (id == ParamID::decay)       return "Decay";
    if (id == ParamID::envMod)      return "Env Mod";
    if (id == ParamID::accent)      return "Accent";
    if (id == ParamID::drive)       return "Drive";
    if (id == ParamID::volume)      return "Volume";
    if (id == ParamID::waveform)    return "Waveform";
    if (id == ParamID::glide)       return "Glide";
    if (id == ParamID::seqPlay)     return "Sequencer";
    if (id == ParamID::seqTempo)    return "Tempo";
    return id;
}

juce::String uiButton (const juce::String& id)
{
    if (id == ParamID::seqPlay)     return "Run";
    return uiLabel (id);
}

} // namespace

TEW03AudioProcessorEditor::PitchCell::PitchCell (TEW03AudioProcessor& p, int stepIndex)
    : proc (p), index (stepIndex)
{
    const auto s = proc.getSequencer().getStep (index);
    lastNote = s.note >= 0 ? s.note : 36;
}

void TEW03AudioProcessorEditor::PitchCell::paint (juce::Graphics& g)
{
    const auto note = proc.getSequencer().getStep (index).note;
    auto r = getLocalBounds().toFloat().reduced (1.f);
    g.setColour (note < 0 ? kChassisDark : kCream);
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (kInk.withAlpha (0.35f));
    g.drawRoundedRectangle (r, 3.f, 1.f);
    g.setColour (kInk);
    g.setFont (boldFont (11.f));
    g.drawFittedText (noteText (note), getLocalBounds(), juce::Justification::centred, 1);
}

void TEW03AudioProcessorEditor::PitchCell::refresh()
{
    const auto s = proc.getSequencer().getStep (index);
    if (s.note >= 0)
        lastNote = s.note;
    repaint();
}

void TEW03AudioProcessorEditor::PitchCell::mouseDown (const juce::MouseEvent& e)
{
    auto s = proc.getSequencer().getStep (index);
    dragStartY = e.y;
    dragStartNote = s.note < 0 ? lastNote : s.note;
    dragged = false;
}

void TEW03AudioProcessorEditor::PitchCell::mouseDrag (const juce::MouseEvent& e)
{
    const int delta = (dragStartY - e.y) / 6;
    if (delta == 0)
        return;

    dragged = true;
    auto s = proc.getSequencer().getStep (index);
    s.note = nudgeNote (dragStartNote, delta);
    lastNote = s.note;
    proc.setPatternStep (index, s);
    repaint();
}

void TEW03AudioProcessorEditor::PitchCell::mouseUp (const juce::MouseEvent&)
{
    if (dragged)
        return;

    auto s = proc.getSequencer().getStep (index);
    if (s.note < 0)
        s.note = lastNote;
    else
    {
        lastNote = s.note;
        s.note = -1;
    }
    proc.setPatternStep (index, s);
    repaint();
}

void TEW03AudioProcessorEditor::PitchCell::mouseWheelMove (const juce::MouseEvent&,
                                                           const juce::MouseWheelDetails& w)
{
    auto s = proc.getSequencer().getStep (index);
    const int delta = w.deltaY > 0 ? 1 : -1;
    s.note = nudgeNote (s.note, delta);
    lastNote = s.note;
    proc.setPatternStep (index, s);
    repaint();
}

TEW03AudioProcessorEditor::StepColumn::StepColumn (TEW03AudioProcessor& p, int stepIndex)
    : pitch (p, stepIndex), index (stepIndex)
{
    addAndMakeVisible (pitch);
    addAndMakeVisible (accent);
    addAndMakeVisible (slide);

    const auto s = p.getSequencer().getStep (index);
    accent.setToggleState (s.accent, juce::dontSendNotification);
    slide.setToggleState (s.slide, juce::dontSendNotification);
    accent.setComponentID ("led");
    slide.setComponentID ("led");
    accent.setTooltip ("Step accent");
    slide.setTooltip ("Step slide");

    accent.onClick = [this, &p]
    {
        auto step = p.getSequencer().getStep (index);
        step.accent = accent.getToggleState();
        p.setPatternStep (index, step);
    };
    slide.onClick = [this, &p]
    {
        auto step = p.getSequencer().getStep (index);
        step.slide = slide.getToggleState();
        p.setPatternStep (index, step);
    };
}

void TEW03AudioProcessorEditor::StepColumn::paint (juce::Graphics& g)
{
    auto led = getLocalBounds().removeFromTop (kLedH).toFloat();
    g.setColour (pitch.lit ? kLedOn : kLedOff);
    g.fillEllipse (led.withSizeKeepingCentre (8.f, 8.f));
}

void TEW03AudioProcessorEditor::StepColumn::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (kLedH);
    pitch.setBounds (r.removeFromTop (kPitchH).reduced (1, 0));
    accent.setBounds (r.removeFromTop (kToggleH));
    slide.setBounds (r.removeFromTop (kToggleH));
}

void TEW03AudioProcessorEditor::StepColumn::refreshPitch()
{
    pitch.refresh();
}

void TEW03AudioProcessorEditor::StepColumn::setLit (bool on)
{
    pitch.lit = on;
    pitch.repaint();
    repaint();
}

void TEW03AudioProcessorEditor::PanelLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                                            float pos, float startAngle, float endAngle,
                                                            juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.f);
    const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto rc = bounds.withSizeKeepingCentre (d, d);

    g.setColour (kKnob);
    g.fillEllipse (rc);

    auto cap = rc.reduced (d * 0.16f);
    g.setColour (kChrome);
    g.fillEllipse (cap);
    g.setColour (kChromeHi);
    g.fillEllipse (cap.translated (-d * 0.06f, -d * 0.08f).withSizeKeepingCentre (cap.getWidth() * 0.45f,
                                                                                 cap.getHeight() * 0.35f));

    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto c = cap.getCentre();
    const auto tip = c.getPointOnCircumference (cap.getWidth() * 0.42f, angle);
    g.setColour (kInk);
    g.drawLine (c.x, c.y, tip.x, tip.y, 2.2f);
    g.fillEllipse (c.x - 2.5f, c.y - 2.5f, 5.f, 5.f);
}

void TEW03AudioProcessorEditor::PanelLnF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                                            bool, bool)
{
    auto bounds = b.getLocalBounds().toFloat().reduced (2.f);

    if (b.getComponentID() == "wave")
    {
        g.setColour (kKnob);
        g.fillRoundedRectangle (bounds, 4.f);
        const float half = bounds.getWidth() * 0.5f;
        auto knob = bounds.withWidth (half);
        if (b.getToggleState())
            knob = knob.translated (half, 0.f);
        g.setColour (kCream);
        g.fillRoundedRectangle (knob.reduced (2.f), 3.f);
        g.setFont (boldFont (11.f));
        g.setColour (b.getToggleState() ? kCream : kInk);
        g.drawText ("SAW", bounds.removeFromLeft (half).toNearestInt(), juce::Justification::centred, false);
        g.setColour (b.getToggleState() ? kInk : kCream);
        g.drawText ("SQR", bounds.toNearestInt(), juce::Justification::centred, false);
        return;
    }

    g.setColour (kKnob);
    g.fillRoundedRectangle (bounds, 3.f);
    g.setFont (boldFont (10.f));

    if (b.getButtonText().length() <= 1)
    {
        g.setColour (b.getToggleState() ? kLedOn : kCream);
        g.drawText (b.getButtonText(), bounds.toNearestInt(), juce::Justification::centred, false);
        return;
    }

    const float led = juce::jmin (10.f, bounds.getHeight() - 4.f);
    g.setColour (b.getToggleState() ? kLedOn : kLedOff);
    g.fillEllipse (bounds.getX() + 4.f, bounds.getCentreY() - led * 0.5f, led, led);
    g.setColour (kCream);
    g.drawText (b.getButtonText(), bounds.reduced (led + 6.f, 0.f).toNearestInt(),
                juce::Justification::centredLeft, false);
}

juce::Font TEW03AudioProcessorEditor::PanelLnF::getLabelFont (juce::Label&)
{
    return boldFont (11.f);
}

TEW03AudioProcessorEditor::ParamCell::ParamCell (juce::AudioProcessorValueTreeState& state,
                                                 juce::RangedAudioParameter& param)
{
    const auto id = param.getParameterID();
    label.setText (uiLabel (id), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, kInk);
    label.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (label);

    isBool = dynamic_cast<juce::AudioParameterBool*> (&param) != nullptr;
    isFlip = isBool && id == ParamID::waveform;

    if (isBool)
    {
        addAndMakeVisible (button);
        button.setButtonText (uiButton (id));
        if (isFlip)
        {
            button.setComponentID ("wave");
            button.setButtonText ({});
        }
        else
        {
            button.setComponentID ("led");
        }
        if (id == ParamID::seqPlay)
            button.setTooltip ("Run or stop the internal sequencer");
        buttonAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, id, button);
    }
    else
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        const bool tempo = id == ParamID::seqTempo;
        slider.setTextBoxStyle (tempo ? juce::Slider::TextBoxBelow : juce::Slider::NoTextBox,
                                false, 56, 14);
        slider.setColour (juce::Slider::textBoxTextColourId, kInk);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setDoubleClickReturnValue (true, param.convertFrom0to1 (param.getDefaultValue()));
        addAndMakeVisible (slider);
        sliderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id, slider);
    }
}

void TEW03AudioProcessorEditor::ParamCell::resized()
{
    auto r = getLocalBounds().reduced (2);
    label.setBounds (r.removeFromTop (kLabelH));
    if (isBool)
        button.setBounds (r.removeFromTop (isFlip ? 28 : 24).reduced (2, 2));
    else
        slider.setBounds (r);
}

TEW03AudioProcessorEditor::TEW03AudioProcessorEditor (TEW03AudioProcessor& p)
    : juce::AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&panelLnF);
    setOpaque (true);

    static constexpr const char* kSeq[] = {
        ParamID::seqPlay, ParamID::seqTempo, ParamID::waveform
    };
    static constexpr const char* kFilter[] = {
        ParamID::cutoff, ParamID::resonance, ParamID::envMod, ParamID::decay, ParamID::accent
    };
    static constexpr const char* kMaster[] = {
        ParamID::drive, ParamID::glide, ParamID::volume
    };

    auto add = [this] (juce::OwnedArray<ParamCell>& dest, const char* id)
    {
        auto* p = dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter (id));
        if (p == nullptr)
            return;
        addAndMakeVisible (dest.add (new ParamCell (proc.apvts, *p)));
    };

    for (auto* id : kSeq)
        add (seqCells, id);
    for (auto* id : kFilter)
        add (filterCells, id);
    for (auto* id : kMaster)
        add (masterCells, id);

    for (int i = 0; i < tew::Sequencer::numSteps; ++i)
    {
        auto* col = steps.add (new StepColumn (proc, i));
        addAndMakeVisible (col);
    }

    setSize (kEditorW, kEditorH);
    startTimerHz (15);
}

TEW03AudioProcessorEditor::~TEW03AudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TEW03AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kChassis);

    auto r = getLocalBounds().reduced (kPad);
    auto title = r.removeFromTop (kTitleH);
    g.setColour (kInk);
    g.setFont (boldFont (16.f));
    g.drawText ("TEW03", title.removeFromLeft (90), juce::Justification::centredLeft, false);
    g.setFont (boldFont (10.f));
    g.drawText ("STUPID SYSTEMS", title, juce::Justification::centredRight, false);

    auto header = [this, &g] (juce::Rectangle<int> area, const juce::String& name)
    {
        g.setColour (kInk.withAlpha (0.55f));
        g.setFont (boldFont (10.f));
        g.drawText (name, area.removeFromTop (kSectionH), juce::Justification::centred, false);
        g.setColour (kInk.withAlpha (0.22f));
        g.drawHorizontalLine (area.getY(), (float) area.getX() + 8.f, (float) area.getRight() - 8.f);
    };
    header (filterArea, "FILTER");
    header (masterArea, "MASTER");

    g.setColour (kInk.withAlpha (0.22f));
    if (! seqArea.isEmpty())
        g.drawLine ((float) seqArea.getRight(), (float) seqArea.getY() + 2.f,
                    (float) seqArea.getRight(), (float) seqArea.getBottom() - 2.f, 1.f);
    if (! filterArea.isEmpty())
        g.drawLine ((float) filterArea.getRight(), (float) filterArea.getY() + 2.f,
                    (float) filterArea.getRight(), (float) filterArea.getBottom() - 2.f, 1.f);

    auto bevel = getLocalBounds().toFloat().reduced (1.5f);
    g.setColour (kChassisDark);
    g.drawRoundedRectangle (bevel, 2.f, 2.f);
}

void TEW03AudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (kPad);
    r.removeFromTop (kTitleH);

    auto strip = r.removeFromBottom (kStripH);
    r.removeFromBottom (4);

    seqArea = r.removeFromLeft (kSeqW);
    const int masterW = r.getWidth() * 3 / 8;
    filterArea = r.removeFromLeft (r.getWidth() - masterW);
    masterArea = r;

    auto seq = seqArea.reduced (2, 0);
    if (seqCells.size() >= 3)
    {
        seqCells[0]->setBounds (seq.removeFromTop (36));
        auto row = seq;
        const int half = row.getWidth() / 2;
        seqCells[1]->setBounds (row.removeFromLeft (half));
        seqCells[2]->setBounds (row);
    }

    auto place = [] (juce::OwnedArray<ParamCell>& cells, juce::Rectangle<int> area)
    {
        area.removeFromTop (kSectionH);
        const int n = juce::jmax (1, cells.size());
        const int w = area.getWidth() / n;
        for (int i = 0; i < cells.size(); ++i)
            cells[i]->setBounds (area.removeFromLeft (i == cells.size() - 1 ? area.getWidth() : w));
    };
    place (filterCells, filterArea);
    place (masterCells, masterArea);

    const int stepW = strip.getWidth() / tew::Sequencer::numSteps;
    for (int i = 0; i < steps.size(); ++i)
        steps[i]->setBounds (strip.removeFromLeft (i == steps.size() - 1 ? strip.getWidth() : stepW));
}

void TEW03AudioProcessorEditor::timerCallback()
{
    refreshPlayhead();
}

void TEW03AudioProcessorEditor::refreshPlayhead()
{
    const int now = proc.getSequencer().playhead();
    if (now == lastPlayhead)
        return;

    if (lastPlayhead >= 0 && lastPlayhead < steps.size())
        steps[lastPlayhead]->setLit (false);
    if (now >= 0 && now < steps.size())
        steps[now]->setLit (true);
    lastPlayhead = now;
}
