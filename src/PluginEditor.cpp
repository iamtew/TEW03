#include "PluginEditor.h"

namespace
{
constexpr int kColW = 40;
constexpr int kPitchH = 36;
constexpr int kToggleH = 22;
constexpr int kStripH = kPitchH + kToggleH * 2;
constexpr int kRowH = 28;
constexpr int kNoteMin = 24;
constexpr int kNoteMax = 60;

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
} // namespace

TEW03AudioProcessorEditor::PitchCell::PitchCell (TEW03AudioProcessor& p, int stepIndex)
    : proc (p), index (stepIndex)
{
    const auto s = proc.getSequencer().getStep (index);
    lastNote = s.note >= 0 ? s.note : 36;
}

void TEW03AudioProcessorEditor::PitchCell::paint (juce::Graphics& g)
{
    g.fillAll (lit ? juce::Colours::darkorange : juce::Colours::darkslategrey);
    g.setColour (juce::Colours::white);
    g.setFont (13.f);
    g.drawFittedText (noteText (proc.getSequencer().getStep (index).note),
                      getLocalBounds(), juce::Justification::centred, 1);
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

void TEW03AudioProcessorEditor::StepColumn::resized()
{
    auto r = getLocalBounds();
    pitch.setBounds (r.removeFromTop (kPitchH).reduced (1));
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
}

TEW03AudioProcessorEditor::ParamRow::ParamRow (juce::AudioProcessorValueTreeState& state,
                                               juce::RangedAudioParameter& param)
{
    label.setText (param.getName (32), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);

    isBool = dynamic_cast<juce::AudioParameterBool*> (&param) != nullptr;
    const auto id = param.getParameterID();

    if (isBool)
    {
        addAndMakeVisible (button);
        buttonAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, id, button);
    }
    else
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 18);
        addAndMakeVisible (slider);
        sliderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id, slider);
    }
}

void TEW03AudioProcessorEditor::ParamRow::resized()
{
    auto r = getLocalBounds().reduced (4, 2);
    label.setBounds (r.removeFromLeft (88));
    if (isBool)
        button.setBounds (r.removeFromLeft (80));
    else
        slider.setBounds (r);
}

TEW03AudioProcessorEditor::TEW03AudioProcessorEditor (TEW03AudioProcessor& p)
    : juce::AudioProcessorEditor (p), proc (p)
{
    for (int i = 0; i < tew::Sequencer::numSteps; ++i)
    {
        auto* col = steps.add (new StepColumn (proc, i));
        addAndMakeVisible (col);
    }

    for (auto* param : proc.getParameters())
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param);
        if (ranged == nullptr)
            continue;
        auto* row = rows.add (new ParamRow (proc.apvts, *ranged));
        addAndMakeVisible (row);
    }

    setSize (tew::Sequencer::numSteps * kColW, kStripH + 8 + rows.size() * kRowH);
    startTimerHz (15);
}

TEW03AudioProcessorEditor::~TEW03AudioProcessorEditor()
{
    stopTimer();
}

void TEW03AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void TEW03AudioProcessorEditor::resized()
{
    auto r = getLocalBounds();
    auto strip = r.removeFromTop (kStripH);
    const int colW = strip.getWidth() / tew::Sequencer::numSteps;
    for (int i = 0; i < steps.size(); ++i)
        steps[i]->setBounds (strip.removeFromLeft (i == steps.size() - 1 ? strip.getWidth() : colW));

    r.removeFromTop (8);
    for (auto* row : rows)
        row->setBounds (r.removeFromTop (kRowH));
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
