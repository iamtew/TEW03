#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr int kEditorW = 900;
constexpr int kEditorH = 480;
constexpr int kLockH = 22;
constexpr int kPad = 6;
constexpr int kTitleH = 22;
constexpr int kSeqW = 236;
constexpr int kSectionH = 14;
constexpr int kLedH = 10;
constexpr int kToggleH = 18;
constexpr int kStripH = kLedH + kToggleH * 2 + 4;
constexpr int kLabelH = 11;
constexpr int kNoteMin = 24;
constexpr int kNoteMax = 60;
constexpr int kOctave = 12;
constexpr int kKeyW = 54;
constexpr int kPianoW = 8;
constexpr int kRowH = 16;
constexpr int kRollH = kOctave * kRowH;

const juce::Colour kChassis { 0xffe8c200 };
const juce::Colour kChassisDark { 0xffc9a400 };
const juce::Colour kCream { 0xfff4ead0 };
const juce::Colour kInk { 0xff111111 };
const juce::Colour kKnob { 0xff1a1a1a };
const juce::Colour kChrome { 0xffd4d8dc };
const juce::Colour kChromeHi { 0xfff2f4f6 };
const juce::Colour kLedOn { 0xffff2200 };
const juce::Colour kLedOff { 0xff5a1808 };
const juce::Colour kLaneWhite { 0xff3c3c3c };
const juce::Colour kLaneBlack { 0xff2a2a2a };

juce::String noteLetter (int note)
{
    return juce::MidiMessage::getMidiNoteName (note, true, false, 4);
}

int noteOctave (int note)
{
    return (note / 12) - 1; // middle C (60) = C4
}

int nudgeNote (int note, int delta)
{
    const int from = note < 0 ? 36 : note;
    return juce::jlimit (kNoteMin, kNoteMax, from + delta);
}

bool inScale (int note, int root, bool minor)
{
    const int pc = ((note % 12) - root + 12) % 12;
    if (minor)
        return pc == 0 || pc == 2 || pc == 3 || pc == 5 || pc == 7 || pc == 8 || pc == 10;
    return pc == 0 || pc == 2 || pc == 4 || pc == 5 || pc == 7 || pc == 9 || pc == 11;
}

int snapScale (int note, int root, bool minor)
{
    note = juce::jlimit (kNoteMin, kNoteMax, note);
    if (inScale (note, root, minor))
        return note;

    for (int d = 1; d <= 6; ++d)
    {
        const int up = note + d;
        if (up <= kNoteMax && inScale (up, root, minor))
            return up;
        const int down = note - d;
        if (down >= kNoteMin && inScale (down, root, minor))
            return down;
    }
    return note;
}

int stepScale (int note, int dir, int root, bool minor)
{
    int n = note;
    for (int i = 0; i < 12; ++i)
    {
        n = nudgeNote (n, dir);
        if (n == note)
            break;
        if (inScale (n, root, minor))
            return n;
    }
    return note;
}

bool isBlackKey (int note)
{
    switch (note % 12)
    {
        case 1: case 3: case 6: case 8: case 10: return true;
        default: return false;
    }
}

struct ScaleList
{
    int notes[40] {};
    int n = 0;
};

ScaleList scaleNotes (int root, bool minor)
{
    ScaleList s;
    for (int note = kNoteMin; note <= kNoteMax; ++note)
        if (inScale (note, root, minor))
            s.notes[s.n++] = note;
    return s;
}

int lockedStart (int viewLow, const ScaleList& s)
{
    int i = 0;
    while (i < s.n && s.notes[i] < viewLow)
        ++i;
    const int maxStart = juce::jmax (0, s.n - kOctave);
    return juce::jlimit (0, maxStart, i);
}

int clampViewLow (int viewLow, bool locked, int root, bool minor)
{
    if (! locked)
        return juce::jlimit (kNoteMin, kNoteMax - kOctave + 1, viewLow);

    const auto s = scaleNotes (root, minor);
    if (s.n <= 0)
        return kNoteMin;

    const int maxStartNote = s.notes[juce::jmax (0, s.n - kOctave)];
    if (! inScale (viewLow, root, minor))
        viewLow = snapScale (viewLow, root, minor);
    return juce::jlimit (s.notes[0], maxStartNote, viewLow);
}

struct RollView
{
    bool locked = false;
    int root = 0;
    bool minor = false;
    int viewLow = 36;
};

int visibleRows (const RollView& v)
{
    if (! v.locked)
        return kOctave;

    return juce::jmin (kOctave, juce::jmax (1, scaleNotes (v.root, v.minor).n));
}

int noteForRow (int row, const RollView& v)
{
    if (! v.locked)
        return v.viewLow + kOctave - 1 - juce::jlimit (0, kOctave - 1, row);

    const auto s = scaleNotes (v.root, v.minor);
    const int start = lockedStart (v.viewLow, s);
    const int count = juce::jmin (kOctave, s.n - start);
    const int idx = start + count - 1 - juce::jlimit (0, count - 1, row);
    return s.notes[idx];
}

int rowForNote (int note, const RollView& v)
{
    if (! v.locked)
    {
        if (note < v.viewLow || note > v.viewLow + kOctave - 1)
            return -1;
        return v.viewLow + kOctave - 1 - note;
    }

    const auto s = scaleNotes (v.root, v.minor);
    const int start = lockedStart (v.viewLow, s);
    const int count = juce::jmin (kOctave, s.n - start);
    for (int i = 0; i < count; ++i)
        if (s.notes[start + i] == note)
            return count - 1 - i;
    return -1;
}

juce::Colour pitchColour (int note)
{
    const float t = juce::jlimit (0.f, 1.f,
                                  (float) (kNoteMax - note) / (float) (kNoteMax - kNoteMin));
    juce::ColourGradient grad (juce::Colour (0xff156878), 0.f, 0.f,
                               juce::Colour (0xff2a4a08), 0.f, 1.f, false);
    grad.addColour (0.28, juce::Colour (0xff1a9a58));
    grad.addColour (0.52, juce::Colour (0xff28b040));
    grad.addColour (0.78, juce::Colour (0xff3a8a18));
    return grad.getColourAtPosition ((double) t);
}

struct RollGeom
{
    juce::Rectangle<float> keys;
    juce::Rectangle<float> grid;
    float colW = 1.f;
    float rowH = 1.f;
    int rows = kOctave;
    int cols = tew::Sequencer::numSteps;
};

int gridSteps (TEW03AudioProcessor& proc)
{
    return proc.apvts.getRawParameterValue (ParamID::seq2x)->load() >= 0.5f
         ? tew::Sequencer::maxSteps : tew::Sequencer::numSteps;
}

RollGeom makeGeom (juce::Rectangle<int> bounds, int rows, int cols)
{
    auto f = bounds.toFloat();
    RollGeom g;
    g.keys = f.removeFromLeft ((float) kKeyW);
    g.grid = f;
    g.cols = juce::jmax (1, cols);
    g.colW = g.grid.getWidth() / (float) g.cols;
    g.rows = juce::jmax (1, rows);
    g.rowH = (float) kRowH;
    return g;
}

int noteAtY (const RollGeom& g, float y, const RollView& v)
{
    const int row = juce::jlimit (0, g.rows - 1,
                                  (int) std::floor ((y - g.grid.getY()) / g.rowH));
    return noteForRow (row, v);
}

int stepAtX (const RollGeom& g, float x)
{
    if (x < g.grid.getX())
        return -1;
    return juce::jlimit (0, g.cols - 1,
                         (int) std::floor ((x - g.grid.getX()) / g.colW));
}

juce::Rectangle<float> cellRect (const RollGeom& g, int step, int note, const RollView& v)
{
    const int row = rowForNote (note, v);
    if (row < 0)
        return {};
    return { g.grid.getX() + (float) step * g.colW,
             g.grid.getY() + (float) row * g.rowH,
             g.colW, g.rowH };
}

// Y of a pitch even when scrolled off the window, so slide lines can keep going.
float pitchY (const RollGeom& g, int note, const RollView& v)
{
    const int row = rowForNote (note, v);
    if (row >= 0)
        return g.grid.getY() + ((float) row + 0.5f) * g.rowH;

    const int top = noteForRow (0, v);
    const int bot = noteForRow (g.rows - 1, v);
    int steps = 0;
    if (note > top)
    {
        for (int n = top + 1; n <= note; ++n)
            if (! v.locked || inScale (n, v.root, v.minor))
                ++steps;
        return g.grid.getY() + 0.5f * g.rowH - (float) steps * g.rowH;
    }

    for (int n = bot - 1; n >= note; --n)
        if (! v.locked || inScale (n, v.root, v.minor))
            ++steps;
    return g.grid.getY() + ((float) g.rows - 0.5f) * g.rowH + (float) steps * g.rowH;
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

TEW03AudioProcessorEditor::PianoRoll::PianoRoll (TEW03AudioProcessor& p)
    : proc (p)
{
}

void TEW03AudioProcessorEditor::PianoRoll::setPlayhead (int step)
{
    if (playhead == step)
        return;
    playhead = step;
    repaint();
}

int TEW03AudioProcessorEditor::PianoRoll::lockNote (int note) const
{
    return locked ? snapScale (note, keyRoot, minor) : juce::jlimit (kNoteMin, kNoteMax, note);
}

void TEW03AudioProcessorEditor::PianoRoll::scrollBy (int semitones)
{
    const int next = clampViewLow (viewLow + semitones, locked, keyRoot, minor);
    if (next == viewLow)
        return;
    viewLow = next;
    repaint();
}

void TEW03AudioProcessorEditor::PianoRoll::paint (juce::Graphics& g)
{
    const RollView v { locked, keyRoot, minor, viewLow };
    auto used = getLocalBounds();
    used.setHeight (visibleRows (v) * kRowH);
    const auto geo = makeGeom (used, visibleRows (v), gridSteps (proc));

    for (int row = 0; row < geo.rows; ++row)
    {
        const int note = noteForRow (row, v);
        const float y = geo.keys.getY() + (float) row * geo.rowH;
        auto lane = juce::Rectangle<float> (geo.grid.getX(), y, geo.grid.getWidth(), geo.rowH);
        auto key = juce::Rectangle<float> (geo.keys.getX(), y, geo.keys.getWidth(), geo.rowH);

        g.setColour (isBlackKey (note) ? kLaneBlack : kLaneWhite);
        g.fillRect (lane);
        g.setColour (pitchColour (note));
        g.fillRect (key);
    }

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (int row = 0; row <= geo.rows; ++row)
        g.drawHorizontalLine ((int) std::round (geo.keys.getY() + (float) row * geo.rowH),
                              geo.keys.getX(), geo.keys.getRight());

    const float nameH = juce::jlimit (9.f, 13.f, geo.rowH - 3.f);
    g.setFont (boldFont (nameH));

    for (int row = 0; row < geo.rows; ++row)
    {
        const int note = noteForRow (row, v);
        auto key = juce::Rectangle<float> (geo.keys.getX(),
                                           geo.keys.getY() + (float) row * geo.rowH,
                                           geo.keys.getWidth(), geo.rowH);

        if (! locked)
        {
            auto piano = key.removeFromRight ((float) kPianoW);
            g.setColour (isBlackKey (note) ? juce::Colour (0xff111111) : kCream);
            g.fillRect (piano.reduced (0.f, 0.5f));
        }

        auto label = key.toNearestInt().reduced (3, 0);
        g.setColour (kCream);
        g.drawText (noteLetter (note), label, juce::Justification::centredLeft, false);
        if (note % 12 == 0)
            g.drawText (juce::String (noteOctave (note)), label,
                        juce::Justification::centredRight, false);
    }

    if (playhead >= 0 && playhead < geo.cols)
    {
        auto col = juce::Rectangle<float> (geo.grid.getX() + (float) playhead * geo.colW,
                                           geo.grid.getY(), geo.colW, geo.rowH * (float) geo.rows);
        g.setColour (kLedOn.withAlpha (0.18f));
        g.fillRect (col);
    }

    g.setColour (kCream.withAlpha (0.12f));
    const float gridBottom = geo.grid.getY() + geo.rowH * (float) geo.rows;
    for (int i = 0; i <= geo.cols; ++i)
        g.drawVerticalLine ((int) std::round (geo.grid.getX() + (float) i * geo.colW),
                            geo.grid.getY(), gridBottom);
    for (int row = 0; row <= geo.rows; ++row)
        g.drawHorizontalLine ((int) std::round (geo.grid.getY() + (float) row * geo.rowH),
                              geo.grid.getX(), geo.grid.getRight());

    auto& seq = proc.getSequencer();

    for (int i = 0; i < geo.cols; ++i)
    {
        const auto s = seq.getStep (i);
        if (s.note < 0)
            continue;

        auto cell = cellRect (geo, i, s.note, v).reduced (1.f, 1.f);
        if (! cell.isEmpty())
        {
            g.setColour (s.accent ? kLedOn : kCream);
            g.fillRoundedRectangle (cell, 2.f);
            g.setColour (s.accent ? kCream.withAlpha (0.7f) : juce::Colour (0xff1a1a1a).withAlpha (0.55f));
            g.drawRoundedRectangle (cell, 2.f, 1.f);
        }

        if (! s.slide)
            continue;

        const float y0 = cell.isEmpty() ? pitchY (geo, s.note, v) : cell.getCentreY();
        if (! cell.isEmpty())
        {
            const float tipX = cell.getRight() + juce::jmin (6.f, geo.colW * 0.2f);
            juce::Path chev;
            chev.addTriangle (cell.getRight() - 2.f, y0 - 3.5f,
                              tipX, y0,
                              cell.getRight() - 2.f, y0 + 3.5f);
            g.setColour (s.accent ? kCream : juce::Colour (0xff1a1a1a));
            g.fillPath (chev);
        }

        const int next = i + 1;
        if (next >= geo.cols)
            continue;

        const auto ns = seq.getStep (next);
        if (ns.note < 0)
            continue;

        const float x0 = geo.grid.getX() + (float) (i + 1) * geo.colW;
        const float x1 = geo.grid.getX() + (float) next * geo.colW + 1.f;
        const float y1 = pitchY (geo, ns.note, v);
        g.saveState();
        g.reduceClipRegion (juce::Rectangle<float> (geo.grid.getX(), geo.grid.getY(),
                                                    geo.grid.getWidth(),
                                                    geo.rowH * (float) geo.rows).toNearestInt());
        g.setColour ((s.accent ? kLedOn : kCream).withAlpha (0.55f));
        g.drawLine (x0, y0, x1, y1, 1.6f);
        g.restoreState();
    }
}

void TEW03AudioProcessorEditor::PianoRoll::mouseDown (const juce::MouseEvent& e)
{
    const RollView v { locked, keyRoot, minor, viewLow };
    auto used = getLocalBounds();
    used.setHeight (visibleRows (v) * kRowH);
    const auto geo = makeGeom (used, visibleRows (v), gridSteps (proc));

    gutterDrag = e.x < (int) geo.grid.getX();
    gutterStartY = e.y;
    gutterStartView = viewLow;
    dragStep = gutterDrag ? -1 : stepAtX (geo, (float) e.x);
    dragged = false;
}

void TEW03AudioProcessorEditor::PianoRoll::mouseDrag (const juce::MouseEvent& e)
{
    if (gutterDrag)
    {
        const int delta = (gutterStartY - e.y) / kRowH;
        scrollBy ((gutterStartView + delta) - viewLow);
        return;
    }

    if (dragStep < 0)
        return;

    const RollView v { locked, keyRoot, minor, viewLow };
    auto used = getLocalBounds();
    used.setHeight (visibleRows (v) * kRowH);
    const auto geo = makeGeom (used, visibleRows (v), gridSteps (proc));
    const int note = lockNote (noteAtY (geo, (float) e.y, v));
    auto s = proc.getSequencer().getStep (dragStep);
    if (s.note == note)
        return;

    dragged = true;
    s.note = note;
    proc.setPatternStep (dragStep, s);
    repaint();
}

void TEW03AudioProcessorEditor::PianoRoll::mouseUp (const juce::MouseEvent& e)
{
    if (gutterDrag)
    {
        gutterDrag = false;
        return;
    }

    if (dragged || dragStep < 0)
        return;

    const RollView v { locked, keyRoot, minor, viewLow };
    auto used = getLocalBounds();
    used.setHeight (visibleRows (v) * kRowH);
    const auto geo = makeGeom (used, visibleRows (v), gridSteps (proc));
    if (e.x < (int) geo.grid.getX())
        return;

    const int note = lockNote (noteAtY (geo, (float) e.y, v));
    auto s = proc.getSequencer().getStep (dragStep);
    s.note = (s.note == note) ? -1 : note;
    proc.setPatternStep (dragStep, s);
    repaint();
}

void TEW03AudioProcessorEditor::PianoRoll::mouseWheelMove (const juce::MouseEvent& e,
                                                           const juce::MouseWheelDetails& w)
{
    const RollView v { locked, keyRoot, minor, viewLow };
    auto used = getLocalBounds();
    used.setHeight (visibleRows (v) * kRowH);
    const auto geo = makeGeom (used, visibleRows (v), gridSteps (proc));

    if (e.x < (int) geo.grid.getX())
    {
        scrollBy (w.deltaY > 0 ? 1 : -1);
        return;
    }

    const int step = stepAtX (geo, (float) e.x);
    if (step < 0)
        return;

    auto s = proc.getSequencer().getStep (step);
    if (s.note < 0)
        return;

    const int dir = w.deltaY > 0 ? 1 : -1;
    s.note = locked ? stepScale (s.note, dir, keyRoot, minor) : nudgeNote (s.note, dir);
    proc.setPatternStep (step, s);
    repaint();
}

TEW03AudioProcessorEditor::StepColumn::StepColumn (TEW03AudioProcessor& p, int stepIndex, juce::Component& rollToRepaint)
    : roll (rollToRepaint), index (stepIndex)
{
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
        roll.repaint();
    };
    slide.onClick = [this, &p]
    {
        auto step = p.getSequencer().getStep (index);
        step.slide = slide.getToggleState();
        p.setPatternStep (index, step);
        roll.repaint();
    };
}

void TEW03AudioProcessorEditor::StepColumn::paint (juce::Graphics& g)
{
    auto led = getLocalBounds().removeFromTop (kLedH).toFloat();
    g.setColour (lit ? kLedOn : kLedOff);
    g.fillEllipse (led.withSizeKeepingCentre (8.f, 8.f));
}

void TEW03AudioProcessorEditor::StepColumn::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (kLedH);
    accent.setBounds (r.removeFromTop (kToggleH));
    slide.setBounds (r.removeFromTop (kToggleH));
}

void TEW03AudioProcessorEditor::StepColumn::setLit (bool on)
{
    if (lit == on)
        return;
    lit = on;
    repaint();
}

void TEW03AudioProcessorEditor::StepColumn::syncFrom (TEW03AudioProcessor& p)
{
    const auto s = p.getSequencer().getStep (index);
    accent.setToggleState (s.accent, juce::dontSendNotification);
    slide.setToggleState (s.slide, juce::dontSendNotification);
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
        const float half = bounds.getHeight() * 0.5f;
        auto knob = bounds.withHeight (half);
        if (b.getToggleState())
            knob = knob.translated (0.f, half);
        g.setColour (kCream);
        g.fillRoundedRectangle (knob.reduced (2.f), 3.f);
        g.setFont (boldFont (11.f));
        g.setColour (b.getToggleState() ? kCream : kInk);
        g.drawText ("Saw", bounds.removeFromTop (half).toNearestInt(), juce::Justification::centred, false);
        g.setColour (b.getToggleState() ? kInk : kCream);
        g.drawText ("Sqr", bounds.toNearestInt(), juce::Justification::centred, false);
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

void TEW03AudioProcessorEditor::PanelLnF::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                                        int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0.f, 0.f, (float) width, (float) height);
    g.setColour (juce::Colour (0xff2e2e2e));
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (kInk.withAlpha (0.45f));
    g.drawRoundedRectangle (r.reduced (0.5f), 3.f, 1.f);
}

juce::Font TEW03AudioProcessorEditor::PanelLnF::getComboBoxFont (juce::ComboBox&)
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
        const int boxW = (id == ParamID::cutoff) ? 88 : 56;
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, boxW, 14);
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
    auto r = getLocalBounds().reduced (2, 0);
    label.setBounds (r.removeFromTop (kLabelH));
    if (isBool)
        button.setBounds ((isFlip ? r : r.removeFromTop (24)).reduced (2, 2));
    else
        slider.setBounds (r);
}

TEW03AudioProcessorEditor::TEW03AudioProcessorEditor (TEW03AudioProcessor& p)
    : juce::AudioProcessorEditor (p), proc (p), pianoRoll (p)
{
    setLookAndFeel (&panelLnF);
    setOpaque (true);
    panelLnF.setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff2e2e2e));
    panelLnF.setColour (juce::PopupMenu::textColourId, kCream);
    panelLnF.setColour (juce::PopupMenu::highlightedBackgroundColourId, kLedOn);
    panelLnF.setColour (juce::PopupMenu::highlightedTextColourId, kCream);

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
        auto* cell = dest.add (new ParamCell (proc.apvts, *p));
        addAndMakeVisible (cell);
        if (id == ParamID::seqTempo)
            tempoCell = cell;
    };

    for (auto* id : kSeq)
        add (seqCells, id);
    for (auto* id : kFilter)
        add (filterCells, id);
    for (auto* id : kMaster)
        add (masterCells, id);

    addAndMakeVisible (pianoRoll);

    auto setupLabel = [] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredRight);
        l.setColour (juce::Label::textColourId, kInk);
        l.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    };
    setupLabel (keyLabel, "Key");
    setupLabel (scaleLabel, "Scale");
    addAndMakeVisible (keyLabel);
    addAndMakeVisible (scaleLabel);

    lockBtn.setClickingTogglesState (true);
    lockBtn.setToggleState (true, juce::dontSendNotification);
    lockBtn.setTooltip ("Lock sequencer notes to the selected key and scale");
    addAndMakeVisible (lockBtn);

    x2Btn.setClickingTogglesState (true);
    x2Btn.setTooltip ("Split each step in half (32nds)");
    addAndMakeVisible (x2Btn);
    x2Att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, ParamID::seq2x, x2Btn);
    x2Btn.onClick = [this]
    {
        proc.syncSeqLength (x2Btn.getToggleState());
        for (int i = 0; i < steps.size(); ++i)
            steps[i]->syncFrom (proc);
        resized();
        pianoRoll.repaint();
    };

    static constexpr const char* kKeys[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    for (int i = 0; i < 12; ++i)
        keyBox.addItem (kKeys[i], i + 1);
    keyBox.setSelectedId (1, juce::dontSendNotification);
    scaleBox.addItem ("Major", 1);
    scaleBox.addItem ("Minor", 2);
    scaleBox.setSelectedId (1, juce::dontSendNotification);

    auto colourBox = [] (juce::ComboBox& b)
    {
        b.setColour (juce::ComboBox::textColourId, kCream);
        b.setColour (juce::ComboBox::arrowColourId, kCream);
        b.setTooltip ("Scale used when key lock is on");
    };
    colourBox (keyBox);
    colourBox (scaleBox);

    auto applyLock = [this]
    {
        pianoRoll.locked = lockBtn.getToggleState();
        pianoRoll.keyRoot = keyBox.getSelectedId() - 1;
        pianoRoll.minor = scaleBox.getSelectedId() == 2;
        pianoRoll.repaint();
    };
    lockBtn.onClick = applyLock;
    keyBox.onChange = applyLock;
    scaleBox.onChange = applyLock;
    addAndMakeVisible (keyBox);
    addAndMakeVisible (scaleBox);

    for (int i = 0; i < tew::Sequencer::maxSteps; ++i)
    {
        auto* col = steps.add (new StepColumn (proc, i, pianoRoll));
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
    pianoRoll.setBounds (r.removeFromBottom (kRollH));
    auto lock = r.removeFromBottom (kLockH);
    r.removeFromBottom (4);

    lockBtn.setBounds (lock.removeFromLeft (56).reduced (0, 1));
    keyLabel.setBounds (lock.removeFromLeft (28));
    keyBox.setBounds (lock.removeFromLeft (72).reduced (0, 1));
    lock.removeFromLeft (10);
    scaleLabel.setBounds (lock.removeFromLeft (40));
    scaleBox.setBounds (lock.removeFromLeft (88).reduced (0, 1));
    x2Btn.setBounds (lock.removeFromRight (44).reduced (0, 1));

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

    strip.removeFromLeft (kKeyW);
    const int n = gridSteps (proc);
    for (int i = 0; i < steps.size(); ++i)
        steps[i]->setVisible (i < n);
    const int stepW = strip.getWidth() / n;
    for (int i = 0; i < n; ++i)
        steps[i]->setBounds (strip.removeFromLeft (i == n - 1 ? strip.getWidth() : stepW));
}

void TEW03AudioProcessorEditor::timerCallback()
{
    refreshPlayhead();
    refreshHostTempo();
}

void TEW03AudioProcessorEditor::refreshHostTempo()
{
    if (tempoCell == nullptr)
        return;

    const bool host = proc.usesHostTempo();
    tempoCell->slider.setEnabled (! host);
    if (! host)
        return;

    auto* param = dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter (ParamID::seqTempo));
    if (param == nullptr)
        return;

    const float bpm = proc.tempoBpm();
    const float shown = param->convertFrom0to1 (param->getValue());
    if (std::abs (shown - bpm) <= 0.05f)
        return;

    param->setValueNotifyingHost (param->convertTo0to1 (bpm));
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
    pianoRoll.setPlayhead (now);
    lastPlayhead = now;
}
