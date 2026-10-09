#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr int kEditorW = 900;
constexpr int kEditorH = 628;
constexpr int kLfoH = 140;
constexpr int kLockH = 22;
constexpr int kPad = 6;
constexpr int kTitleH = 36;
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
const juce::Colour kLfoCol[2] { juce::Colour (0xff3dcc8c), juce::Colour (0xffff6a00) };
const juce::Colour kLfoIdle { 0xff7a7a7a };

juce::Colour lfoCol (int src)
{
    return kLfoCol[src == 2 ? 1 : 0];
}

bool lfoAssigned (TEW03AudioProcessor& proc, int index)
{
    const int src = index + 1;
    for (int d = 0; d < tew::destCount; ++d)
        if (juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::destLfoIds[d])->load()) == src)
            return true;
    return false;
}

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
    return tew::inScale (note, root, minor);
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

// +1 top (higher pitches), -1 bottom, 0 not on overlay
int keyOverlayDir (const RollGeom& geo, juce::Point<int> p)
{
    if (p.x >= (int) geo.grid.getX() || p.x < (int) geo.keys.getX())
        return 0;
    const float y = (float) p.y;
    if (y < geo.keys.getY() || y >= geo.keys.getBottom())
        return 0;
    if (y < geo.keys.getY() + geo.rowH)
        return 1;
    if (y >= geo.keys.getBottom() - geo.rowH)
        return -1;
    return 0;
}

void paintKeyOverlay (juce::Graphics& g, juce::Rectangle<float> band, bool up, bool held)
{
    g.setColour (juce::Colours::black.withAlpha (held ? 0.5f : 0.35f));
    g.fillRect (band);

    juce::Path chev;
    const float cx = band.getCentreX();
    const float cy = band.getCentreY();
    if (up)
    {
        chev.startNewSubPath (cx - 7.f, cy + 3.f);
        chev.lineTo (cx, cy - 4.f);
        chev.lineTo (cx + 7.f, cy + 3.f);
    }
    else
    {
        chev.startNewSubPath (cx - 7.f, cy - 3.f);
        chev.lineTo (cx, cy + 4.f);
        chev.lineTo (cx + 7.f, cy - 3.f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.strokePath (chev, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
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
    if (id == ParamID::nasty)       return "Nasty";
    if (id == ParamID::volume)      return "Volume";
    if (id == ParamID::waveform)    return "Waveform";
    if (id == ParamID::glide)       return "Glide";
    if (id == ParamID::seqPlay)     return "Run";
    if (id == ParamID::playMode)    return "Play";
    if (id == ParamID::seqTempo)    return "Tempo";
    return id;
}

juce::String uiButton (const juce::String& id)
{
    return uiLabel (id);
}

} // namespace

TEW03AudioProcessorEditor::PianoRoll::PianoRoll (TEW03AudioProcessor& p)
    : proc (p)
{
}

TEW03AudioProcessorEditor::PianoRoll::~PianoRoll()
{
    stopTimer();
}

void TEW03AudioProcessorEditor::PianoRoll::stopHold()
{
    if (holdDir == 0)
        return;
    holdDir = 0;
    stopTimer();
    repaint();
}

void TEW03AudioProcessorEditor::PianoRoll::timerCallback()
{
    if (holdDir == 0)
    {
        stopTimer();
        return;
    }
    if (getTimerInterval() != 80)
        startTimer (80);
    scrollBy (holdDir);
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

void TEW03AudioProcessorEditor::PianoRoll::scrollBy (int steps)
{
    if (steps == 0)
        return;

    int next = viewLow;
    if (locked)
    {
        const int dir = steps > 0 ? 1 : -1;
        const int n = steps > 0 ? steps : -steps;
        for (int i = 0; i < n; ++i)
            next = stepScale (next, dir, keyRoot, minor);
        next = clampViewLow (next, true, keyRoot, minor);
    }
    else
    {
        next = clampViewLow (viewLow + steps, false, keyRoot, minor);
    }

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

    paintKeyOverlay (g, geo.keys.withHeight (geo.rowH), true, holdDir > 0);
    paintKeyOverlay (g, geo.keys.withY (geo.keys.getBottom() - geo.rowH).withHeight (geo.rowH),
                     false, holdDir < 0);

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

    // LFO overlay sits on the grid. Y is 0-1 of the LFO, not pitch — scale scroll leaves it.
    {
        auto clip = juce::Rectangle<float> (geo.grid.getX(), geo.grid.getY(),
                                            geo.grid.getWidth(),
                                            geo.rowH * (float) geo.rows).toNearestInt();
        g.saveState();
        g.reduceClipRegion (clip);
        const float bpm = proc.tempoBpm();
        for (int li = 0; li < tew::numLfos; ++li)
        {
            if (! lfoAssigned (proc, li))
                continue;
            const bool sync = proc.apvts.getRawParameterValue (ParamID::lfoSyncIds[li])->load() >= 0.5f;
            const int div = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::lfoDivIds[li])->load());
            const float rate = proc.apvts.getRawParameterValue (ParamID::lfoRateIds[li])->load();
            const float cycles = tew::cyclesPerBar (sync, div, rate, bpm);
            const auto shape = proc.getLfoShape (li);
            juce::Path path;
            constexpr int kPts = 96;
            for (int i = 0; i <= kPts; ++i)
            {
                const float x01 = (float) i / (float) kPts;
                float phase = x01 * cycles;
                phase -= std::floor (phase);
                const float y01 = shape.lookup (phase);
                const float x = geo.grid.getX() + x01 * geo.grid.getWidth();
                const float y = geo.grid.getY() + (1.f - y01) * geo.rowH * (float) geo.rows;
                if (i == 0)
                    path.startNewSubPath (x, y);
                else
                    path.lineTo (x, y);
            }
            g.setColour (kLfoCol[li].withAlpha (0.55f));
            g.strokePath (path, juce::PathStrokeType (1.4f));
        }
        g.restoreState();
    }
}

void TEW03AudioProcessorEditor::PianoRoll::mouseDown (const juce::MouseEvent& e)
{
    const RollView v { locked, keyRoot, minor, viewLow };
    auto used = getLocalBounds();
    used.setHeight (visibleRows (v) * kRowH);
    const auto geo = makeGeom (used, visibleRows (v), gridSteps (proc));

    const int overlay = keyOverlayDir (geo, e.getPosition());
    if (overlay != 0)
    {
        gutterDrag = false;
        dragStep = -1;
        dragged = false;
        holdDir = overlay;
        scrollBy (holdDir);
        startTimer (350);
        repaint();
        return;
    }

    gutterDrag = e.x < (int) geo.grid.getX();
    gutterStartY = e.y;
    gutterStartView = viewLow;
    dragStep = gutterDrag ? -1 : stepAtX (geo, (float) e.x);
    dragged = false;
}

void TEW03AudioProcessorEditor::PianoRoll::mouseDrag (const juce::MouseEvent& e)
{
    if (holdDir != 0)
        return;

    if (gutterDrag)
    {
        const int delta = (gutterStartY - e.y) / kRowH;
        viewLow = gutterStartView;
        if (delta != 0)
            scrollBy (delta);
        else
            repaint();
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
    if (holdDir != 0)
    {
        stopHold();
        return;
    }

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
                                                            juce::Slider& slider)
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

    const int src = (int) slider.getProperties().getWithDefault ("lfoSrc", 0);
    if (src > 0)
    {
        const float amt = std::abs ((float) slider.getProperties().getWithDefault ("lfoAmt", 0.f));
        g.setColour (lfoCol (src).withAlpha (0.45f + 0.5f * amt));
        g.drawEllipse (rc.expanded (2.f), 3.f);
    }
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

    if (b.getComponentID() == "plain")
    {
        g.setColour (kCream);
        g.drawText (b.getButtonText(), bounds.toNearestInt(), juce::Justification::centred, false);
        return;
    }

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

TEW03AudioProcessorEditor::ParamCell::ParamCell (TEW03AudioProcessorEditor& ed,
                                                 juce::RangedAudioParameter& param)
    : editor (ed)
{
    const auto id = param.getParameterID();
    paramId = id;
    dest = ParamID::destIndexForId (id.toRawUTF8());
    if (dest >= 0)
        label.setInterceptsMouseClicks (false, false);
    label.setText (uiLabel (id), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, kInk);
    label.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (label);

    isBool = dynamic_cast<juce::AudioParameterBool*> (&param) != nullptr;
    isChoice = dynamic_cast<juce::AudioParameterChoice*> (&param) != nullptr;
    isFlip = isBool && id == ParamID::waveform;
    auto& state = editor.proc.apvts;

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
        buttonAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, id, button);
    }
    else if (isChoice)
    {
        auto* choice = dynamic_cast<juce::AudioParameterChoice*> (&param);
        addAndMakeVisible (combo);
        combo.setColour (juce::ComboBox::textColourId, kCream);
        combo.setColour (juce::ComboBox::arrowColourId, kCream);
        if (choice != nullptr)
            for (int i = 0; i < choice->choices.size(); ++i)
                combo.addItem (choice->choices[i], i + 1);
        if (id == ParamID::playMode)
            combo.setTooltip ("Keyboard plays notes. Pattern loops the selected slot. Key holds C1-B3 to play bank patterns.");
        comboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, id, combo);
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
        slider.addMouseListener (this, false);
    }
}

void TEW03AudioProcessorEditor::ParamCell::resized()
{
    auto r = getLocalBounds().reduced (2, 0);
    label.setBounds (r.removeFromTop (kLabelH));
    if (isBool)
        button.setBounds ((isFlip ? r : r.removeFromTop (24)).reduced (2, 2));
    else if (isChoice)
        combo.setBounds (r.removeFromTop (24).reduced (2, 2));
    else
        slider.setBounds (r);
}

juce::Rectangle<int> TEW03AudioProcessorEditor::ParamCell::badgeBounds() const
{
    return { getWidth() - 16, 0, 14, kLabelH };
}

void TEW03AudioProcessorEditor::ParamCell::refreshMod()
{
    if (dest < 0)
        return;
    lfoSrc = juce::roundToInt (editor.proc.apvts.getRawParameterValue (ParamID::destLfoIds[dest])->load());
    lfoAmt = editor.proc.apvts.getRawParameterValue (ParamID::destAmtIds[dest])->load();
    slider.getProperties().set ("lfoSrc", lfoSrc);
    slider.getProperties().set ("lfoAmt", lfoAmt);
    slider.repaint();
    repaint();
}

void TEW03AudioProcessorEditor::ParamCell::paintOverChildren (juce::Graphics& g)
{
    if (dest < 0 || lfoSrc <= 0)
        return;
    auto b = badgeBounds().toFloat();
    const auto col = lfoCol (lfoSrc);
    g.setColour (kKnob);
    g.fillRoundedRectangle (b, 3.f);
    g.setColour (col);
    g.drawRoundedRectangle (b, 3.f, 1.2f);
    g.setFont (boldFont (10.f));
    g.setColour (col);
    g.drawText (juce::String (lfoSrc), b.toNearestInt(), juce::Justification::centred, false);
}

void TEW03AudioProcessorEditor::ParamCell::mouseDown (const juce::MouseEvent& e)
{
    if (dest < 0)
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
    m.addItem (1, "LFO 1", true, lfoSrc == 1);
    m.addItem (2, "LFO 2", true, lfoSrc == 2);
    m.addItem (3, "None", true, lfoSrc == 0);
    juce::Component::SafePointer<ParamCell> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                     [safe] (int result)
                     {
                         if (safe == nullptr || result <= 0)
                             return;
                         safe->editor.assignDest (safe->dest, result == 3 ? 0 : result);
                     });
}

void TEW03AudioProcessorEditor::ParamCell::mouseDrag (const juce::MouseEvent& e)
{
    if (! amtDragging || dest < 0)
        return;
    const auto pos = e.getEventRelativeTo (this).getPosition();
    const float next = juce::jlimit (-1.f, 1.f, amtDragStart - (float) (pos.y - amtDragY) / 80.f);
    editor.setDestAmt (dest, next);
}

void TEW03AudioProcessorEditor::ParamCell::mouseUp (const juce::MouseEvent&)
{
    amtDragging = false;
}

bool TEW03AudioProcessorEditor::ParamCell::isInterestedInDragSource (const SourceDetails& d)
{
    return dest >= 0 && d.description.toString().startsWith ("lfo:");
}

void TEW03AudioProcessorEditor::ParamCell::itemDropped (const SourceDetails& d)
{
    const int src = d.description.toString().fromFirstOccurrenceOf (":", false, false).getIntValue();
    if (src >= 1 && src <= tew::numLfos)
        editor.assignDest (dest, src);
}

TEW03AudioProcessorEditor::VolumeStrip::VolumeStrip (TEW03AudioProcessorEditor& ed)
    : editor (ed)
{
    slider.setLookAndFeel (&lnf);
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    slider.setOpaque (false);
    slider.setColour (juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::trackColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::thumbColourId, kInk);
    auto* p = dynamic_cast<juce::RangedAudioParameter*> (editor.proc.apvts.getParameter (ParamID::volume));
    if (p != nullptr)
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    sliderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        editor.proc.apvts, ParamID::volume, slider);
    slider.addMouseListener (this, false);
    addAndMakeVisible (slider);
    setTooltip ("Volume");
}

TEW03AudioProcessorEditor::VolumeStrip::~VolumeStrip()
{
    slider.setLookAndFeel (nullptr);
}

juce::Rectangle<float> TEW03AudioProcessorEditor::VolumeStrip::faderArea() const
{
    auto r = getLocalBounds().toFloat();
    return r.removeFromLeft (r.getWidth() * 5.f / 9.f);
}

juce::Rectangle<float> TEW03AudioProcessorEditor::VolumeStrip::scopeArea() const
{
    auto r = getLocalBounds().toFloat();
    r.removeFromLeft (r.getWidth() * 5.f / 9.f + 4.f);
    return r;
}

void TEW03AudioProcessorEditor::VolumeStrip::resized()
{
    slider.setBounds (faderArea().toNearestInt());
}

juce::Rectangle<int> TEW03AudioProcessorEditor::VolumeStrip::badgeBounds() const
{
    auto f = faderArea().toNearestInt();
    return { f.getRight() - 16, f.getY(), 14, 12 };
}

void TEW03AudioProcessorEditor::VolumeStrip::LnF::drawLinearSlider (juce::Graphics& g, int, int y, int, int h,
                                                                   float pos, float, float,
                                                                   juce::Slider::SliderStyle, juce::Slider&)
{
    const float by = (float) (y + h);
    juce::Path p;
    p.addTriangle (pos - 5.f, by, pos + 5.f, by, pos, by - 7.f);
    g.setColour (kInk);
    g.fillPath (p);
    g.setColour (kCream);
    g.strokePath (p, juce::PathStrokeType (1.f));
}

void TEW03AudioProcessorEditor::VolumeStrip::paint (juce::Graphics& g)
{
    auto fader = faderArea();
    auto scope = scopeArea();

    g.setColour (kLaneBlack);
    g.fillRoundedRectangle (fader, 3.f);
    g.fillRoundedRectangle (scope, 3.f);

    const float pos = (float) slider.valueToProportionOfLength (slider.getValue());
    auto fill = fader.withWidth (std::max (2.f, fader.getWidth() * pos));
    g.setColour (clipped ? kLedOn : kChassisDark);
    g.fillRoundedRectangle (fill, 3.f);

    g.setColour (kCream.withAlpha (0.35f));
    for (int i = 1; i < 8; ++i)
    {
        const float x = fader.getX() + fader.getWidth() * (float) i / 8.f;
        const float tickH = (i == 4) ? 6.f : 3.f;
        g.drawLine (x, fader.getBottom() - 2.f, x, fader.getBottom() - 2.f - tickH, 1.f);
    }

    const int n = juce::jlimit (2, 256, (int) scope.getWidth());
    float specDb[256];
    editor.proc.eqAnalyser().copyLogDb (specDb, n);
    juce::Path spec;
    spec.startNewSubPath (scope.getX(), scope.getBottom());
    for (int i = 0; i < n; ++i)
    {
        const float x01 = (float) i / (float) (n - 1);
        // Same floor as the EQ page: 0 dB at the top, -72 at the bottom.
        const float y01 = 1.f - juce::jlimit (0.f, 1.f, (specDb[i] + 72.f) / 72.f);
        spec.lineTo (scope.getX() + x01 * scope.getWidth(),
                     scope.getY() + y01 * scope.getHeight());
    }
    spec.lineTo (scope.getRight(), scope.getBottom());
    spec.closeSubPath();
    g.setColour ((clipped ? kLedOn : kCream).withAlpha (0.7f));
    g.fillPath (spec);
}

void TEW03AudioProcessorEditor::VolumeStrip::paintOverChildren (juce::Graphics& g)
{
    if (lfoSrc <= 0)
        return;
    auto b = badgeBounds().toFloat();
    const auto col = lfoCol (lfoSrc);
    g.setColour (kKnob);
    g.fillRoundedRectangle (b, 3.f);
    g.setColour (col);
    g.drawRoundedRectangle (b, 3.f, 1.2f);
    g.setFont (boldFont (10.f));
    g.setColour (col);
    g.drawText (juce::String (lfoSrc), b.toNearestInt(), juce::Justification::centred, false);
}

void TEW03AudioProcessorEditor::VolumeStrip::refresh()
{
    if (editor.proc.outputMeter().takeClip())
        clipUntil = juce::Time::getMillisecondCounter() + 800;
    clipped = juce::Time::getMillisecondCounter() < clipUntil;

    lfoSrc = juce::roundToInt (editor.proc.apvts.getRawParameterValue (ParamID::destLfoIds[dest])->load());
    lfoAmt = editor.proc.apvts.getRawParameterValue (ParamID::destAmtIds[dest])->load();

    const int pct = juce::roundToInt (slider.valueToProportionOfLength (slider.getValue()) * 100.f);
    setTooltip ("Volume " + juce::String (pct) + "%");
    slider.setTooltip (getTooltip());
    repaint();
}

void TEW03AudioProcessorEditor::VolumeStrip::mouseDown (const juce::MouseEvent& e)
{
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
    m.addItem (1, "LFO 1", true, lfoSrc == 1);
    m.addItem (2, "LFO 2", true, lfoSrc == 2);
    m.addItem (3, "None", true, lfoSrc == 0);
    juce::Component::SafePointer<VolumeStrip> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                     [safe] (int result)
                     {
                         if (safe == nullptr || result <= 0)
                             return;
                         safe->editor.assignDest (safe->dest, result == 3 ? 0 : result);
                     });
}

void TEW03AudioProcessorEditor::VolumeStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (! amtDragging)
        return;
    const auto pos = e.getEventRelativeTo (this).getPosition();
    const float next = juce::jlimit (-1.f, 1.f, amtDragStart - (float) (pos.y - amtDragY) / 80.f);
    editor.setDestAmt (dest, next);
}

void TEW03AudioProcessorEditor::VolumeStrip::mouseUp (const juce::MouseEvent&)
{
    amtDragging = false;
}

bool TEW03AudioProcessorEditor::VolumeStrip::isInterestedInDragSource (const SourceDetails& d)
{
    return d.description.toString().startsWith ("lfo:");
}

void TEW03AudioProcessorEditor::VolumeStrip::itemDropped (const SourceDetails& d)
{
    const int src = d.description.toString().fromFirstOccurrenceOf (":", false, false).getIntValue();
    if (src >= 1 && src <= tew::numLfos)
        editor.assignDest (dest, src);
}

void TEW03AudioProcessorEditor::SlotBar::setText (const juce::String& t)
{
    if (text == t)
        return;
    text = t;
    repaint();
}

void TEW03AudioProcessorEditor::SlotBar::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff2e2e2e));
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (kInk.withAlpha (0.45f));
    g.drawRoundedRectangle (r.reduced (0.5f), 4.f, 1.f);

    auto left = r.removeFromLeft (18.f);
    auto right = r.removeFromRight (18.f);

    auto chev = [] (juce::Rectangle<float> box, bool back)
    {
        juce::Path p;
        const float cx = box.getCentreX();
        const float cy = box.getCentreY();
        if (back)
            p.addTriangle (cx + 3.f, cy - 5.f, cx - 4.f, cy, cx + 3.f, cy + 5.f);
        else
            p.addTriangle (cx - 3.f, cy - 5.f, cx + 4.f, cy, cx - 3.f, cy + 5.f);
        return p;
    };

    g.setColour (kCream);
    g.fillPath (chev (left, true));
    g.fillPath (chev (right, false));
    g.setFont (boldFont (12.f));
    g.drawText (text, r.toNearestInt(), juce::Justification::centred, false);
}

void TEW03AudioProcessorEditor::SlotBar::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasClicked())
        return;
    if (e.mods.isPopupMenu())
    {
        if (onPopup)
            onPopup();
        return;
    }
    const int w = getWidth();
    if (e.x < 20)
    {
        if (onStep)
            onStep (-1);
    }
    else if (e.x > w - 20)
    {
        if (onStep)
            onStep (1);
    }
    else if (onOpen)
        onOpen();
}

TEW03AudioProcessorEditor::LfoHandle::LfoHandle (TEW03AudioProcessorEditor& ed, int i)
    : editor (ed), index (i)
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

void TEW03AudioProcessorEditor::LfoHandle::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.f);
    const bool on = lfoAssigned (editor.proc, index);
    const auto col = on ? kLfoCol[index] : kLfoIdle;
    g.setColour (kLaneBlack);
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (col);
    g.drawRoundedRectangle (r, 4.f, on ? 1.5f : 1.f);
    g.setFont (boldFont (11.f));
    g.drawText ("LFO " + juce::String (index + 1), r.toNearestInt(), juce::Justification::centred, false);
}

void TEW03AudioProcessorEditor::LfoHandle::mouseDown (const juce::MouseEvent&)
{
    dragging = false;
}

void TEW03AudioProcessorEditor::LfoHandle::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 4)
        return;
    dragging = true;
    editor.startDragging ("lfo:" + juce::String (index + 1), this);
}

TEW03AudioProcessorEditor::LfoShapeView::LfoShapeView (TEW03AudioProcessorEditor& ed, int i)
    : editor (ed), index (i)
{
}

juce::Point<float> TEW03AudioProcessorEditor::LfoShapeView::toScreenPt (const tew::LfoShape& s, int i) const
{
    auto r = getLocalBounds().toFloat().reduced (8.f, 6.f);
    return { r.getX() + s.x[i] * r.getWidth(),
             r.getBottom() - s.y[i] * r.getHeight() };
}

int TEW03AudioProcessorEditor::LfoShapeView::hitPoint (juce::Point<float> p) const
{
    const auto s = editor.proc.getLfoShape (index);
    for (int i = 0; i < s.n; ++i)
        if (p.getDistanceFrom (toScreenPt (s, i)) <= 8.f)
            return i;
    return -1;
}

int TEW03AudioProcessorEditor::LfoShapeView::sixteenths() const
{
    auto& p = editor.proc;
    const bool sync = p.apvts.getRawParameterValue (ParamID::lfoSyncIds[index])->load() >= 0.5f;
    const int div = juce::roundToInt (p.apvts.getRawParameterValue (ParamID::lfoDivIds[index])->load());
    const float rate = p.apvts.getRawParameterValue (ParamID::lfoRateIds[index])->load();
    return tew::sixteenthsPerCycle (sync, div, rate, p.tempoBpm());
}

void TEW03AudioProcessorEditor::LfoShapeView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (kLaneBlack);
    g.fillRoundedRectangle (bounds, 4.f);

    auto r = bounds.reduced (8.f, 6.f);
    const auto col = kLfoCol[index];
    const int n16 = sixteenths();
    for (int i = 1; i < n16; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) n16;
        g.setColour (i % 4 == 0 ? col.withAlpha (0.35f) : kCream.withAlpha (0.12f));
        g.drawVerticalLine ((int) std::round (x), r.getY(), r.getBottom());
    }
    g.setColour (kCream.withAlpha (0.12f));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * 0.25f * (float) i),
                              r.getX(), r.getRight());

    const auto s = editor.proc.getLfoShape (index);
    if (s.n < 2)
        return;

    juce::Path path;
    path.startNewSubPath (toScreenPt (s, 0));
    for (int i = 1; i < s.n; ++i)
        path.lineTo (toScreenPt (s, i));
    g.setColour (col);
    g.strokePath (path, juce::PathStrokeType (1.6f));

    juce::Path fill = path;
    fill.lineTo (r.getRight(), r.getBottom());
    fill.lineTo (r.getX(), r.getBottom());
    fill.closeSubPath();
    g.setColour (col.withAlpha (0.22f));
    g.fillPath (fill);

    for (int i = 0; i < s.n; ++i)
    {
        const auto c = toScreenPt (s, i);
        g.setColour (col);
        g.fillEllipse (c.x - 3.5f, c.y - 3.5f, 7.f, 7.f);
        g.setColour (kCream);
        g.drawEllipse (c.x - 3.5f, c.y - 3.5f, 7.f, 7.f, 1.f);
    }

    if (editor.proc.lfoPlayheadOn())
    {
        const float phase = editor.proc.lfoPhase (index);
        const float px = r.getX() + phase * r.getWidth();
        g.setColour (col.withAlpha (0.9f));
        g.drawLine (px, r.getY(), px, r.getBottom(), 1.f);
    }
}

void TEW03AudioProcessorEditor::LfoShapeView::mouseDown (const juce::MouseEvent& e)
{
    auto s = editor.proc.getLfoShape (index);
    const int hit = hitPoint (e.position);
    if (e.mods.isPopupMenu())
    {
        if (hit > 0 && hit < s.n - 1 && s.n > 2)
        {
            tew::LfoShape next;
            for (int i = 0; i < s.n; ++i)
                if (i != hit)
                    next.add (s.x[i], s.y[i]);
            editor.proc.setLfoShape (index, next);
            editor.refreshLfo();
        }
        return;
    }
    dragPt = hit;
}

void TEW03AudioProcessorEditor::LfoShapeView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragPt < 0)
        return;
    auto s = editor.proc.getLfoShape (index);
    auto r = getLocalBounds().toFloat().reduced (8.f, 6.f);
    float nx = juce::jlimit (0.f, 1.f, (e.position.x - r.getX()) / r.getWidth());
    float ny = juce::jlimit (0.f, 1.f, (r.getBottom() - e.position.y) / r.getHeight());
    if (dragPt == 0)
        nx = 0.f;
    else if (dragPt == s.n - 1)
        nx = 1.f;
    else
    {
        const float lo = s.x[dragPt - 1] + 0.01f;
        const float hi = s.x[dragPt + 1] - 0.01f;
        nx = juce::jlimit (lo, hi, nx);
    }
    s.x[dragPt] = nx;
    s.y[dragPt] = ny;
    editor.proc.setLfoShape (index, s);
    editor.pianoRoll.repaint();
    repaint();
}

void TEW03AudioProcessorEditor::LfoShapeView::mouseUp (const juce::MouseEvent&)
{
    dragPt = -1;
}

void TEW03AudioProcessorEditor::LfoShapeView::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (hitPoint (e.position) >= 0)
        return;
    auto s = editor.proc.getLfoShape (index);
    if (s.n >= tew::lfoMaxPoints)
        return;
    auto r = getLocalBounds().toFloat().reduced (8.f, 6.f);
    const float nx = juce::jlimit (0.02f, 0.98f, (e.position.x - r.getX()) / r.getWidth());
    const float ny = juce::jlimit (0.f, 1.f, (r.getBottom() - e.position.y) / r.getHeight());
    tew::LfoShape next;
    bool inserted = false;
    for (int i = 0; i < s.n; ++i)
    {
        if (! inserted && nx < s.x[i])
        {
            next.add (nx, ny);
            inserted = true;
        }
        next.add (s.x[i], s.y[i]);
    }
    if (! inserted)
        next.add (nx, ny);
    editor.proc.setLfoShape (index, next);
    editor.refreshLfo();
}

TEW03AudioProcessorEditor::LfoLane::LfoLane (TEW03AudioProcessorEditor& ed, int i)
    : editor (ed), index (i), handle (ed, i), view (ed, i)
{
    addAndMakeVisible (handle);
    addAndMakeVisible (view);
    nameBar.onStep = [this] (int d) { cyclePreset (d); };
    addAndMakeVisible (nameBar);

    auto lab = [this] (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, kCream);
        l.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (l);
    };
    lab (modeLabel, "MODE");
    lab (tempoLabel, "TEMPO");
    lab (smoothLabel, "SMOOTH");

    modeBox.addItem ("Free", 1);
    modeBox.addItem ("Trigger", 2);
    divBox.addItem ("1/16", 1);
    divBox.addItem ("1/8", 2);
    divBox.addItem ("1/4", 3);
    divBox.addItem ("1/2", 4);
    divBox.addItem ("1", 5);
    divBox.addItem ("2", 6);
    auto colourBox = [] (juce::ComboBox& b)
    {
        b.setColour (juce::ComboBox::textColourId, kCream);
        b.setColour (juce::ComboBox::arrowColourId, kCream);
    };
    colourBox (modeBox);
    colourBox (divBox);
    addAndMakeVisible (modeBox);
    addAndMakeVisible (divBox);

    syncBtn.setClickingTogglesState (true);
    syncBtn.setComponentID ("led");
    syncBtn.setTooltip ("Sync LFO rate to tempo");
    addAndMakeVisible (syncBtn);

    auto setupKnob = [this] (juce::Slider& s, int boxW)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, boxW, 12);
        s.setColour (juce::Slider::textBoxTextColourId, kCream);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (s);
    };
    setupKnob (rateSlider, 44);
    setupKnob (smoothSlider, 40);
    smoothSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 12);

    auto& st = editor.proc.apvts;
    modeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        st, ParamID::lfoModeIds[index], modeBox);
    divAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        st, ParamID::lfoDivIds[index], divBox);
    syncAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        st, ParamID::lfoSyncIds[index], syncBtn);
    rateAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        st, ParamID::lfoRateIds[index], rateSlider);
    smoothAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        st, ParamID::lfoSmoothIds[index], smoothSlider);

    syncBtn.onClick = [this] { refresh(); };
    rateSlider.onValueChange = [this] { view.repaint(); editor.pianoRoll.repaint(); };
    divBox.onChange = [this] { view.repaint(); editor.pianoRoll.repaint(); };
}

void TEW03AudioProcessorEditor::LfoLane::resized()
{
    auto r = getLocalBounds().reduced (4, 2);
    auto ctrls = r.removeFromLeft (108);
    auto modeR = ctrls.removeFromTop (36);
    modeLabel.setBounds (modeR.removeFromTop (12));
    modeBox.setBounds (modeR.reduced (1, 0));

    auto tempoR = ctrls.removeFromTop (52);
    tempoLabel.setBounds (tempoR.removeFromTop (12));
    syncBtn.setBounds (tempoR.removeFromLeft (40).reduced (1, 2));
    rateSlider.setBounds (tempoR);
    divBox.setBounds (rateSlider.getBounds());

    auto smoothR = ctrls;
    smoothSlider.setBounds (smoothR);
    smoothSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false,
                                  juce::jmax (40, smoothR.getWidth() - 40), 12);
    smoothLabel.setBounds (smoothR.withTrimmedLeft (40).removeFromTop (12));

    auto head = r.removeFromTop (22);
    handle.setBounds (head.removeFromLeft (52));
    head.removeFromLeft (4);
    nameBar.setBounds (head);
    r.removeFromTop (3);
    view.setBounds (r);
}

void TEW03AudioProcessorEditor::LfoLane::refresh()
{
    const bool sync = editor.proc.apvts.getRawParameterValue (ParamID::lfoSyncIds[index])->load() >= 0.5f;
    rateSlider.setVisible (! sync);
    divBox.setVisible (sync);
    const auto shape = editor.proc.getLfoShape (index);
    const int preset = shape.presetIndex();
    nameBar.setText (preset >= 0 ? tew::LfoShape::presetName (preset) : "Custom");
    handle.repaint();
    view.repaint();
}

void TEW03AudioProcessorEditor::LfoLane::cyclePreset (int delta)
{
    auto shape = editor.proc.getLfoShape (index);
    int idx = shape.presetIndex();
    if (idx < 0)
        idx = 0;
    const int n = tew::LfoShape::numPresets;
    idx = (idx + delta + n) % n;
    shape.applyPreset (idx);
    editor.proc.setLfoShape (index, shape);
    editor.refreshLfo();
}

TEW03AudioProcessorEditor::TEW03AudioProcessorEditor (TEW03AudioProcessor& p)
    : juce::AudioProcessorEditor (p), proc (p), effectsPage (p), eqPage (p), pianoRoll (p)
{
    setLookAndFeel (&panelLnF);
    setOpaque (true);
    panelLnF.setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff2e2e2e));
    panelLnF.setColour (juce::PopupMenu::textColourId, kCream);
    panelLnF.setColour (juce::PopupMenu::highlightedBackgroundColourId, kLedOn);
    panelLnF.setColour (juce::PopupMenu::highlightedTextColourId, kCream);

    static constexpr const char* kSeq[] = {
        ParamID::playMode, ParamID::seqTempo, ParamID::waveform
    };
    static constexpr const char* kFilter[] = {
        ParamID::cutoff, ParamID::resonance, ParamID::envMod, ParamID::decay, ParamID::accent
    };
    static constexpr const char* kMaster[] = {
        ParamID::drive, ParamID::glide, ParamID::nasty
    };

    auto add = [this] (juce::OwnedArray<ParamCell>& dest, const char* id)
    {
        auto* p = dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter (id));
        if (p == nullptr)
            return;
        auto* cell = dest.add (new ParamCell (*this, *p));
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
    setupLabel (bankLabel, "Bank");
    setupLabel (patternLabel, "Pat");
    addAndMakeVisible (keyLabel);
    addAndMakeVisible (scaleLabel);
    addAndMakeVisible (bankLabel);
    addAndMakeVisible (patternLabel);

    lockBtn.setClickingTogglesState (true);
    lockBtn.setTooltip ("Lock sequencer notes to the selected key and scale");
    addAndMakeVisible (lockBtn);
    lockAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, ParamID::seqKeyLock, lockBtn);

    runBtn.setClickingTogglesState (true);
    runBtn.setComponentID ("led");
    runBtn.setTooltip ("Play or pause the sequencer");
    addAndMakeVisible (runBtn);
    runAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, ParamID::seqPlay, runBtn);

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

    clearBtn.setClickingTogglesState (false);
    clearBtn.setComponentID ("plain");
    clearBtn.setTooltip ("Clear the current pattern");
    addAndMakeVisible (clearBtn);
    clearBtn.onClick = [this]
    {
        proc.clearCurrentPattern();
        syncSteps();
    };

    static constexpr const char* kKeys[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    for (int i = 0; i < 12; ++i)
        keyBox.addItem (kKeys[i], i + 1);
    scaleBox.addItem ("Major", 1);
    scaleBox.addItem ("Minor", 2);

    auto colourBox = [] (juce::ComboBox& b)
    {
        b.setColour (juce::ComboBox::textColourId, kCream);
        b.setColour (juce::ComboBox::arrowColourId, kCream);
        b.setTooltip ("Scale used when key lock is on");
    };
    colourBox (keyBox);
    colourBox (scaleBox);
    colourBox (bankBox);
    bankBox.setTooltip ("Pattern bank (1-3)");

    for (int i = 1; i <= tew::Sequencer::numBanks; ++i)
        bankBox.addItem (juce::String (i), i);
    addAndMakeVisible (bankBox);
    bankAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, ParamID::seqBank, bankBox);

    patchBar.setTooltip ("Patch: sound knobs. Click for save / load.");
    bankLibBar.setTooltip ("Bank: all 36 patterns. Click for save / load.");
    patBar.setTooltip ("Pattern in the current bank (1-12)");
    pageBar.setTooltip ("Main synth view, Effects chain, or EQ");
    pageBar.setText ("Main");
    pageBar.onStep = [this] (int d) { cyclePage (d); };
    pageBar.onOpen = [this] { openPageMenu(); };
    patchBar.onStep = [this] (int d) { cyclePatch (d); };
    patchBar.onOpen = [this] { openPatchMenu(); };
    bankLibBar.onStep = [this] (int d) { cycleBank (d); };
    bankLibBar.onOpen = [this] { openBankMenu(); };
    patBar.onStep = [this] (int d) { cyclePattern (d); };
    patBar.onOpen = [this] { openPatternMenu(); };
    patBar.onPopup = [this] { openPatternClipMenu(); };
    addAndMakeVisible (pageBar);
    addAndMakeVisible (patchBar);
    addAndMakeVisible (bankLibBar);
    addAndMakeVisible (patBar);
    addAndMakeVisible (volumeStrip);
    addAndMakeVisible (effectsPage);
    effectsPage.setVisible (false);
    addAndMakeVisible (eqPage);
    eqPage.setVisible (false);
    refreshLibraryNames();
    patBar.setText (juce::String (proc.currentPattern() + 1));

    lockBtn.onClick = [this] { applyLock(); };
    keyBox.onChange = [this] { applyLock(); };
    scaleBox.onChange = [this] { applyLock(); };
    keyAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, ParamID::seqKey, keyBox);
    scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, ParamID::seqScale, scaleBox);
    addAndMakeVisible (keyBox);
    addAndMakeVisible (scaleBox);
    applyLock();

    for (int i = 0; i < tew::Sequencer::maxSteps; ++i)
    {
        auto* col = steps.add (new StepColumn (proc, i, pianoRoll));
        addAndMakeVisible (col);
    }

    addAndMakeVisible (lfoLane0);
    addAndMakeVisible (lfoLane1);
    refreshLfo();

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
    auto nameCol = title.removeFromLeft (90);
    g.setColour (kInk);
    g.setFont (boldFont (16.f));
    g.drawText ("TEW03", nameCol.removeFromTop (20), juce::Justification::centredLeft, false);
    g.setFont (boldFont (10.f));
    g.drawText ("v" JucePlugin_VersionString, nameCol, juce::Justification::centredLeft, false);

    if (editorPage != 0)
    {
        auto bevel = getLocalBounds().toFloat().reduced (1.5f);
        g.setColour (kChassisDark);
        g.drawRoundedRectangle (bevel, 2.f, 2.f);
        return;
    }

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

    if (! lfoArea.isEmpty())
    {
        g.setColour (kLaneBlack);
        g.fillRoundedRectangle (lfoArea.toFloat(), 4.f);
    }

    auto bevel = getLocalBounds().toFloat().reduced (1.5f);
    g.setColour (kChassisDark);
    g.drawRoundedRectangle (bevel, 2.f, 2.f);
}

void TEW03AudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (kPad);
    auto title = r.removeFromTop (kTitleH);
    title.removeFromLeft (90);
    volumeStrip.setBounds (title.removeFromRight (240).reduced (2, 4));
    pageBar.setBounds (title.removeFromLeft (80).reduced (4, 2));
    const int barW = title.getWidth() / 2;
    patchBar.setBounds (title.removeFromLeft (barW).reduced (6, 2));
    bankLibBar.setBounds (title.reduced (6, 2));

    const auto body = r;
    effectsPage.setBounds (body);
    eqPage.setBounds (body);

    lfoArea = r.removeFromBottom (kLfoH);
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
    clearBtn.setBounds (lock.removeFromRight (56).reduced (0, 1));
    x2Btn.setBounds (lock.removeFromRight (44).reduced (0, 1));

    const int clusterW = 36 + 48 + 28 + 88;
    auto mid = lock.withSizeKeepingCentre (juce::jmin (clusterW, lock.getWidth()), lock.getHeight());
    bankLabel.setBounds (mid.removeFromLeft (36));
    bankBox.setBounds (mid.removeFromLeft (48).reduced (0, 1));
    patternLabel.setBounds (mid.removeFromLeft (28));
    patBar.setBounds (mid.removeFromLeft (88).reduced (1, 1));

    seqArea = r.removeFromLeft (kSeqW);
    const int masterW = r.getWidth() * 3 / 8;
    filterArea = r.removeFromLeft (r.getWidth() - masterW);
    masterArea = r;

    auto seq = seqArea.reduced (2, 0);
    if (seqCells.size() >= 3)
    {
        auto playRow = seq.removeFromTop (36);
        auto run = playRow.removeFromLeft (68);
        run.removeFromTop (kLabelH);
        runBtn.setBounds (run.reduced (2, 2));
        seqCells[0]->setBounds (playRow);
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
    // Dice 4: Drive Glide / Nasty empty. Bottom-right reserved for a future knob.
    {
        auto area = masterArea;
        area.removeFromTop (kSectionH);
        const int colW = area.getWidth() / 2;
        const int rowH = area.getHeight() / 2;
        auto top = area.removeFromTop (rowH);
        if (masterCells.size() >= 1)
            masterCells[0]->setBounds (top.removeFromLeft (colW));
        if (masterCells.size() >= 2)
            masterCells[1]->setBounds (top);
        if (masterCells.size() >= 3)
            masterCells[2]->setBounds (area.removeFromLeft (colW));
    }

    strip.removeFromLeft (kKeyW);
    const int n = gridSteps (proc);
    for (int i = 0; i < steps.size(); ++i)
        steps[i]->setVisible (i < n);
    const int stepW = strip.getWidth() / n;
    for (int i = 0; i < n; ++i)
        steps[i]->setBounds (strip.removeFromLeft (i == n - 1 ? strip.getWidth() : stepW));

    auto lfo = lfoArea.reduced (4, 4);
    const int half = lfo.getWidth() / 2;
    lfoLane0.setBounds (lfo.removeFromLeft (half));
    lfoLane1.setBounds (lfo);
    applyPageVisibility();
}

void TEW03AudioProcessorEditor::applyLock()
{
    const bool lock = lockBtn.getToggleState();
    const int root = juce::jlimit (0, 11, keyBox.getSelectedId() - 1);
    const bool min = scaleBox.getSelectedId() == 2;
    if (pianoRoll.locked == lock && pianoRoll.keyRoot == root && pianoRoll.minor == min)
        return;
    pianoRoll.locked = lock;
    pianoRoll.keyRoot = root;
    pianoRoll.minor = min;
    pianoRoll.repaint();
}

void TEW03AudioProcessorEditor::timerCallback()
{
    applyLock();
    refreshPlayhead();
    refreshHostTempo();
    refreshSlot();
    refreshLibraryNames();
    refreshLfo();
    volumeStrip.refresh();
    if (editorPage == 1)
        effectsPage.refresh();
    else if (editorPage == 2)
        eqPage.repaint();
}

void TEW03AudioProcessorEditor::syncSteps()
{
    for (int i = 0; i < steps.size(); ++i)
        steps[i]->syncFrom (proc);
    pianoRoll.repaint();
}

void TEW03AudioProcessorEditor::refreshSlot()
{
    const int bank = proc.currentBank();
    const int pat = proc.currentPattern();
    if (bank == lastBank && pat == lastPattern)
        return;

    lastBank = bank;
    lastPattern = pat;

    auto setInt = [this] (const char* id, int shown)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter (id));
        if (param == nullptr)
            return;
        const int cur = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
        if (cur == shown)
            return;
        param->setValueNotifyingHost (param->convertTo0to1 ((float) shown));
    };
    setInt (ParamID::seqBank, bank);
    setInt (ParamID::seqPattern, pat);
    patBar.setText (juce::String (pat + 1));
    syncSteps();
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
    if (now != lastPlayhead)
    {
        if (lastPlayhead >= 0 && lastPlayhead < steps.size())
            steps[lastPlayhead]->setLit (false);
        if (now >= 0 && now < steps.size())
            steps[now]->setLit (true);
        pianoRoll.setPlayhead (now);
        lastPlayhead = now;
    }
    lfoLane0.view.repaint();
    lfoLane1.view.repaint();
}

void TEW03AudioProcessorEditor::refreshLfo()
{
    lfoLane0.refresh();
    lfoLane1.refresh();
    pianoRoll.repaint();

    auto refreshCells = [] (juce::OwnedArray<ParamCell>& cells)
    {
        for (auto* c : cells)
            c->refreshMod();
    };
    refreshCells (seqCells);
    refreshCells (filterCells);
    refreshCells (masterCells);
}

void TEW03AudioProcessorEditor::assignDest (int dest, int lfoIndexSrc)
{
    if (dest < 0 || dest >= tew::destCount)
        return;
    auto* src = proc.apvts.getParameter (ParamID::destLfoIds[dest]);
    auto* amt = proc.apvts.getParameter (ParamID::destAmtIds[dest]);
    if (src == nullptr || amt == nullptr)
        return;
    src->beginChangeGesture();
    src->setValueNotifyingHost (src->convertTo0to1 ((float) juce::jlimit (0, tew::numLfos, lfoIndexSrc)));
    src->endChangeGesture();
    if (lfoIndexSrc > 0 && std::abs (proc.apvts.getRawParameterValue (ParamID::destAmtIds[dest])->load()) < 0.001f)
    {
        amt->beginChangeGesture();
        amt->setValueNotifyingHost (amt->convertTo0to1 (0.5f));
        amt->endChangeGesture();
    }
    refreshLfo();
}

void TEW03AudioProcessorEditor::setDestAmt (int dest, float amount)
{
    if (dest < 0 || dest >= tew::destCount)
        return;
    auto* amt = proc.apvts.getParameter (ParamID::destAmtIds[dest]);
    if (amt == nullptr)
        return;
    amt->setValueNotifyingHost (amt->convertTo0to1 (juce::jlimit (-1.f, 1.f, amount)));
    refreshLfo();
}

void TEW03AudioProcessorEditor::refreshLibraryNames()
{
    patchBar.setText (proc.patchName());
    bankLibBar.setText (proc.bankName());
}

void TEW03AudioProcessorEditor::cyclePattern (int delta)
{
    auto* p = proc.apvts.getParameter (ParamID::seqPattern);
    if (p == nullptr)
        return;
    const int n = tew::Sequencer::patternsPerBank;
    const int cur = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::seqPattern)->load());
    const int next = (cur + delta + n) % n;
    p->beginChangeGesture();
    p->setValueNotifyingHost (p->convertTo0to1 ((float) next));
    p->endChangeGesture();
}

void TEW03AudioProcessorEditor::cyclePatch (int delta)
{
    const auto names = proc.patchNames();
    if (names.isEmpty())
        return;
    const auto cur = proc.patchName();
    int idx = names.indexOf (cur);
    if (idx < 0)
        idx = delta > 0 ? 0 : names.size() - 1;
    else
        idx = (idx + delta + names.size()) % names.size();
    proc.loadPatchByName (names[idx]);
    refreshLibraryNames();
    syncSteps();
    resized();
}

void TEW03AudioProcessorEditor::cycleBank (int delta)
{
    const auto names = proc.bankNames();
    if (names.isEmpty())
        return;
    const auto cur = proc.bankName();
    int idx = names.indexOf (cur);
    if (idx < 0)
        idx = delta > 0 ? 0 : names.size() - 1;
    else
        idx = (idx + delta + names.size()) % names.size();
    proc.loadBankByName (names[idx]);
    refreshLibraryNames();
    syncSteps();
}

void TEW03AudioProcessorEditor::openPatternMenu()
{
    juce::PopupMenu m;
    const int cur = proc.currentPattern();
    for (int i = 0; i < tew::Sequencer::patternsPerBank; ++i)
        m.addItem (i + 1, juce::String (i + 1), true, i == cur);

    juce::Component::SafePointer<TEW03AudioProcessorEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (patBar),
                     [safe] (int result)
                     {
                         if (safe == nullptr || result <= 0)
                             return;
                         auto* p = safe->proc.apvts.getParameter (ParamID::seqPattern);
                         if (p == nullptr)
                             return;
                         p->beginChangeGesture();
                         p->setValueNotifyingHost (p->convertTo0to1 ((float) (result - 1)));
                         p->endChangeGesture();
                     });
}

void TEW03AudioProcessorEditor::openPatternClipMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Copy");
    m.addItem (2, "Paste", proc.hasPatternClip());

    juce::Component::SafePointer<TEW03AudioProcessorEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (patBar),
                     [safe] (int result)
                     {
                         if (safe == nullptr || result <= 0)
                             return;
                         if (result == 1)
                             safe->proc.copyCurrentPattern();
                         else if (result == 2)
                         {
                             safe->proc.pasteCurrentPattern();
                             safe->syncSteps();
                         }
                     });
}

namespace
{
enum
{
    kMenuInit = 1,
    kMenuSave,
    kMenuSaveAs,
    kMenuExport,
    kMenuImport,
    kMenuFiles = 100
};

void fillLibraryMenu (juce::PopupMenu& m, const juce::StringArray& names,
                      const juce::String& current, const juce::String& initLabel)
{
    m.addItem (kMenuInit, initLabel, true, current == initLabel);
    m.addSeparator();
    for (int i = 0; i < names.size(); ++i)
        m.addItem (kMenuFiles + i, names[i], true, names[i] == current);
    m.addSeparator();
    m.addItem (kMenuSave, "Save");
    m.addItem (kMenuSaveAs, "Save As...");
    m.addItem (kMenuExport, "Export...");
    m.addItem (kMenuImport, "Import...");
}
} // namespace

void TEW03AudioProcessorEditor::setEditorPage (int page)
{
    editorPage = juce::jlimit (0, 2, page);
    pageBar.setText (editorPage == 0 ? "Main" : (editorPage == 1 ? "Effects" : "EQ"));
    applyPageVisibility();
    resized();
    repaint();
}

void TEW03AudioProcessorEditor::cyclePage (int delta)
{
    setEditorPage ((editorPage + delta + 3) % 3);
}

void TEW03AudioProcessorEditor::openPageMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Main", true, editorPage == 0);
    m.addItem (2, "Effects", true, editorPage == 1);
    m.addItem (3, "EQ", true, editorPage == 2);
    juce::Component::SafePointer<TEW03AudioProcessorEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (pageBar),
                     [safe] (int result)
                     {
                         if (safe == nullptr || result <= 0)
                             return;
                         safe->setEditorPage (result - 1);
                     });
}

void TEW03AudioProcessorEditor::applyPageVisibility()
{
    const bool main = editorPage == 0;
    effectsPage.setVisible (editorPage == 1);
    eqPage.setVisible (editorPage == 2);
    if (editorPage == 1)
        effectsPage.refresh();

    auto vis = [main] (juce::Component& c) { c.setVisible (main); };
    vis (pianoRoll);
    vis (lockBtn);
    vis (keyLabel);
    vis (scaleLabel);
    vis (bankLabel);
    vis (patternLabel);
    vis (keyBox);
    vis (scaleBox);
    vis (bankBox);
    vis (x2Btn);
    vis (clearBtn);
    vis (runBtn);
    vis (patBar);
    vis (lfoLane0);
    vis (lfoLane1);
    for (auto* c : seqCells)
        vis (*c);
    for (auto* c : filterCells)
        vis (*c);
    for (auto* c : masterCells)
        vis (*c);
    const int n = gridSteps (proc);
    for (int i = 0; i < steps.size(); ++i)
        steps[i]->setVisible (main && i < n);
}

void TEW03AudioProcessorEditor::openPatchMenu()
{
    juce::PopupMenu m;
    const auto names = proc.patchNames();
    fillLibraryMenu (m, names, proc.patchName(), "Init Patch");

    juce::Component::SafePointer<TEW03AudioProcessorEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (patchBar),
                     [safe, names] (int result)
                     {
                         if (safe == nullptr || result <= 0)
                             return;
                         if (result == kMenuInit)
                             safe->proc.initPatch();
                         else if (result == kMenuSave)
                         {
                             if (! safe->proc.savePatch())
                                 safe->chooseSaveAs (true);
                         }
                         else if (result == kMenuSaveAs)
                             safe->chooseSaveAs (true);
                         else if (result == kMenuExport)
                             safe->chooseExport (true);
                         else if (result == kMenuImport)
                             safe->chooseImport (true);
                         else if (result >= kMenuFiles)
                         {
                             const int i = result - kMenuFiles;
                             if (juce::isPositiveAndBelow (i, names.size()))
                                 safe->proc.loadPatchByName (names[i]);
                         }
                         safe->refreshLibraryNames();
                         safe->syncSteps();
                         safe->resized();
                     });
}

void TEW03AudioProcessorEditor::openBankMenu()
{
    juce::PopupMenu m;
    const auto names = proc.bankNames();
    fillLibraryMenu (m, names, proc.bankName(), "Init Bank");

    juce::Component::SafePointer<TEW03AudioProcessorEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (bankLibBar),
                     [safe, names] (int result)
                     {
                         if (safe == nullptr || result <= 0)
                             return;
                         if (result == kMenuInit)
                             safe->proc.initBank();
                         else if (result == kMenuSave)
                         {
                             if (! safe->proc.saveBank())
                                 safe->chooseSaveAs (false);
                         }
                         else if (result == kMenuSaveAs)
                             safe->chooseSaveAs (false);
                         else if (result == kMenuExport)
                             safe->chooseExport (false);
                         else if (result == kMenuImport)
                             safe->chooseImport (false);
                         else if (result >= kMenuFiles)
                         {
                             const int i = result - kMenuFiles;
                             if (juce::isPositiveAndBelow (i, names.size()))
                                 safe->proc.loadBankByName (names[i]);
                         }
                         safe->refreshLibraryNames();
                         safe->syncSteps();
                     });
}

void TEW03AudioProcessorEditor::chooseSaveAs (bool patch)
{
    const auto dir = patch ? proc.patchesDir() : proc.banksDir();
    const char* ext = patch ? "*.tew3p" : "*.tew3b";
    const auto start = dir.getChildFile (patch ? "Patch.tew3p" : "Bank.tew3b");
    chooser = std::make_unique<juce::FileChooser> (patch ? "Save Patch" : "Save Bank", start, ext);

    juce::Component::SafePointer<TEW03AudioProcessorEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe, patch] (const juce::FileChooser& c)
                          {
                              if (safe == nullptr)
                                  return;
                              auto file = c.getResult();
                              if (file.getFullPathName().isEmpty())
                                  return;
                              const auto name = file.getFileNameWithoutExtension();
                              if (patch)
                                  safe->proc.savePatchAs (name);
                              else
                                  safe->proc.saveBankAs (name);
                              safe->refreshLibraryNames();
                          });
}

void TEW03AudioProcessorEditor::chooseExport (bool patch)
{
    const char* ext = patch ? "*.tew3p" : "*.tew3b";
    const auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                           .getChildFile (patch ? "TEW03 Patch.tew3p" : "TEW03 Bank.tew3b");
    chooser = std::make_unique<juce::FileChooser> (patch ? "Export Patch" : "Export Bank", start, ext);

    juce::Component::SafePointer<TEW03AudioProcessorEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe, patch] (const juce::FileChooser& c)
                          {
                              if (safe == nullptr)
                                  return;
                              auto file = c.getResult();
                              if (file.getFullPathName().isEmpty())
                                  return;
                              if (patch)
                                  safe->proc.exportPatch (file);
                              else
                                  safe->proc.exportBank (file);
                          });
}

void TEW03AudioProcessorEditor::chooseImport (bool patch)
{
    const char* ext = patch ? "*.tew3p" : "*.tew3b";
    chooser = std::make_unique<juce::FileChooser> (
        patch ? "Import Patch" : "Import Bank",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        ext);

    juce::Component::SafePointer<TEW03AudioProcessorEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
                          [safe, patch] (const juce::FileChooser& c)
                          {
                              if (safe == nullptr)
                                  return;
                              auto file = c.getResult();
                              if (! file.existsAsFile())
                                  return;
                              if (patch)
                                  safe->proc.importPatch (file);
                              else
                                  safe->proc.importBank (file);
                              safe->refreshLibraryNames();
                              safe->syncSteps();
                              if (patch)
                                  safe->resized();
                          });
}
