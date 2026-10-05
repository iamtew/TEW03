#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "midi/MidiHandler.h"
#include "parameters/Library.h"

#include <cstring>

namespace
{
constexpr int kMidiBytes = 512;
constexpr int kMaxEvents = 32;
constexpr int kPlayKeyboard = 0;
constexpr int kPlayPattern = 1;
constexpr int kPlayKey = 2;

const juce::Identifier kBanks { "BANKS" };
const juce::Identifier kBank { "BANK" };
const juce::Identifier kPattern { "PATTERN" };
const juce::Identifier kStep { "STEP" };
const juce::Identifier kIndex { "index" };
const juce::Identifier kNote { "note" };
const juce::Identifier kAccent { "accent" };
const juce::Identifier kSlide { "slide" };
const juce::Identifier kPatchName { "PATCH_NAME" };
const juce::Identifier kBankName { "BANK_NAME" };
constexpr const char* kInitPatch = "Init Patch";
constexpr const char* kInitBank = "Init Bank";
constexpr const char* kPatchExt = ".tew3p";
constexpr const char* kBankExt = ".tew3b";

juce::File libraryRoot()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Stupid Systems LLC")
        .getChildFile ("TEW03");
}

juce::File ensureDir (juce::File dir)
{
    dir.createDirectory();
    return dir;
}

juce::StringArray listStemNames (const juce::File& dir, const char* wildcard)
{
    juce::StringArray names;
    if (! dir.isDirectory())
        return names;

    for (const auto& entry : juce::RangedDirectoryIterator (dir, false, wildcard))
        names.add (entry.getFile().getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

juce::String legalStem (const juce::String& name)
{
    auto stem = juce::File::createLegalFileName (name.trim());
    if (stem.endsWithIgnoreCase (kPatchExt))
        stem = stem.dropLastCharacters ((int) std::strlen (kPatchExt));
    if (stem.endsWithIgnoreCase (kBankExt))
        stem = stem.dropLastCharacters ((int) std::strlen (kBankExt));
    return stem;
}

bool writeText (const juce::File& dest, const std::string& text)
{
    dest.getParentDirectory().createDirectory();
    return dest.replaceWithText (text);
}

std::string readText (const juce::File& src)
{
    return src.loadFileAsString().toStdString();
}

tew::Sequencer::Step stepFromTree (const juce::ValueTree& t)
{
    return { (int) t.getProperty (kNote, 36),
             (bool) t.getProperty (kAccent, false),
             (bool) t.getProperty (kSlide, false) };
}

juce::ValueTree makeStepTree (int index, tew::Sequencer::Step s)
{
    juce::ValueTree t (kStep);
    t.setProperty (kIndex, index, nullptr);
    t.setProperty (kNote, s.note, nullptr);
    t.setProperty (kAccent, s.accent, nullptr);
    t.setProperty (kSlide, s.slide, nullptr);
    return t;
}

juce::ValueTree findChildByIndex (juce::ValueTree parent, const juce::Identifier& type, int index)
{
    for (int i = 0; i < parent.getNumChildren(); ++i)
    {
        auto child = parent.getChild (i);
        if (child.hasType (type) && (int) child.getProperty (kIndex, -1) == index)
            return child;
    }
    return {};
}

juce::ValueTree findStepChild (juce::ValueTree pattern, int index)
{
    return findChildByIndex (pattern, kStep, index);
}

bool slotHasNote (juce::ValueTree pattern)
{
    for (int i = 0; i < tew::Sequencer::maxSteps; ++i)
    {
        auto child = findStepChild (pattern, i);
        if (child.isValid() && (int) child.getProperty (kNote, -1) >= 0)
            return true;
    }
    return false;
}
} // namespace

TEW03AudioProcessor::TEW03AudioProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    for (int b = 0; b < tew::Sequencer::numBanks; ++b)
        for (int p = 0; p < tew::Sequencer::patternsPerBank; ++p)
        {
            tew::Sequencer::Step slot[tew::Sequencer::maxSteps];
            tew::Sequencer::fillSlot (slot, b, p);
            for (int s = 0; s < tew::Sequencer::maxSteps; ++s)
                patterns[b][p][s].store (tew::Sequencer::pack (slot[s]), std::memory_order_relaxed);
        }

    apvts.addParameterListener (ParamID::seqBank, this);
    apvts.addParameterListener (ParamID::seqPattern, this);
    loadBanksFromState();

    if (! apvts.state.hasProperty (kPatchName))
        apvts.state.setProperty (kPatchName, kInitPatch, nullptr);
    if (! apvts.state.hasProperty (kBankName))
        apvts.state.setProperty (kBankName, kInitBank, nullptr);
}

TEW03AudioProcessor::~TEW03AudioProcessor()
{
    apvts.removeParameterListener (ParamID::seqBank, this);
    apvts.removeParameterListener (ParamID::seqPattern, this);
}

float TEW03AudioProcessor::raw (const char* id) const
{
    return apvts.getRawParameterValue (id)->load();
}

float TEW03AudioProcessor::hostTempoBpm() const
{
    if (wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        return 0.f;

    if (auto* head = getPlayHead())
    {
        if (const auto pos = head->getPosition())
        {
            if (const auto bpm = pos->getBpm())
            {
                if (*bpm > 0.0)
                    return (float) *bpm;
            }
        }
    }

    return 0.f;
}

bool TEW03AudioProcessor::usesHostTempo() const
{
    return hostTempoBpm() > 0.f;
}

float TEW03AudioProcessor::tempoBpm() const
{
    // Standalone has no DAW clock. The Seq Tempo knob drives it.
    // A plugin follows the host BPM, and falls back to the knob if the host omits it.
    const float host = hostTempoBpm();
    return host > 0.f ? host : raw (ParamID::seqTempo);
}

bool TEW03AudioProcessor::patternShouldRun() const
{
    if (raw (ParamID::seqPlay) < 0.5f)
        return false;

    // Standalone, or a host that never handed us a playhead: Run is the clock.
    if (wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        return true;

    if (auto* head = getPlayHead())
    {
        if (const auto pos = head->getPosition())
            return pos->getIsPlaying();
    }

    return true;
}

std::atomic<uint32_t>* TEW03AudioProcessor::slotPacked (int bank, int pat)
{
    return patterns[bank][pat];
}

void TEW03AudioProcessor::loadLiveFromSlot()
{
    sequencer.loadPacked (slotPacked (currentBank(), currentPattern()));
}

void TEW03AudioProcessor::storeLiveToSlot()
{
    sequencer.storePacked (slotPacked (currentBank(), currentPattern()));
}

void TEW03AudioProcessor::selectSlot (int bank, int pat)
{
    bank = juce::jlimit (0, tew::Sequencer::numBanks - 1, bank);
    pat = juce::jlimit (0, tew::Sequencer::patternsPerBank - 1, pat);
    if (bank == currentBank() && pat == currentPattern())
        return;

    storeLiveToSlot();
    curBank.store (bank, std::memory_order_relaxed);
    curPattern.store (pat, std::memory_order_relaxed);
    loadLiveFromSlot();
}

void TEW03AudioProcessor::parameterChanged (const juce::String& parameterID, float)
{
    if (parameterID != ParamID::seqBank && parameterID != ParamID::seqPattern)
        return;

    selectSlot ((int) raw (ParamID::seqBank),
                (int) raw (ParamID::seqPattern));
}

void TEW03AudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    sequencer.prepare (sampleRate);
}

void TEW03AudioProcessor::releaseResources() {}

bool TEW03AudioProcessor::handleKeyTriggers (const juce::MidiBuffer& midi)
{
    int held = heldKey.load (std::memory_order_relaxed);

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn() && msg.getFloatVelocity() > 0.f)
        {
            int bank = 0, pat = 0;
            if (! tew::Sequencer::mapKeyToSlot (msg.getNoteNumber(), bank, pat))
                continue;
            held = msg.getNoteNumber();
            selectSlot (bank, pat);
        }
        else if (msg.isNoteOff() || (msg.isNoteOn() && msg.getFloatVelocity() <= 0.f))
        {
            if (msg.getNoteNumber() == held)
                held = -1;
        }
    }

    heldKey.store (held, std::memory_order_relaxed);
    return held >= 0;
}

void TEW03AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // First callback may grow the host buffer. Later blocks stay at this size.
    midi.ensureSize (kMidiBytes);

    engine.setTone (raw (ParamID::decay),
                    raw (ParamID::accent),
                    raw (ParamID::glide),
                    raw (ParamID::volume),
                    raw (ParamID::waveform) >= 0.5f);
    engine.setFilter (raw (ParamID::cutoff),
                      raw (ParamID::resonance),
                      raw (ParamID::drive),
                      raw (ParamID::envMod));

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    // Wheel still works while the sequencer owns the notes.
    tew::MidiHandler::applyPitchBend (midi, engine.getVoice());

    sequencer.setLength (raw (ParamID::seq2x) >= 0.5f ? tew::Sequencer::maxSteps
                                                     : tew::Sequencer::numSteps);

    const int mode = juce::roundToInt (raw (ParamID::playMode));
    const float bpm = tempoBpm();

    if (mode != kPlayKey)
        heldKey.store (-1, std::memory_order_relaxed);

    bool playing = false;
    if (mode == kPlayPattern)
        playing = patternShouldRun();
    else if (mode == kPlayKey)
        playing = handleKeyTriggers (midi) && raw (ParamID::seqPlay) >= 0.5f;

    if (mode != kPlayKeyboard)
        midi.clear();

    if (! playing)
    {
        tew::SeqEvent stopped[1];
        const int nStop = sequencer.advance (numSamples, bpm, false, stopped, 1);
        for (int i = 0; i < nStop; ++i)
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, stopped[i].note), stopped[i].offset);
            engine.getVoice().noteOff (stopped[i].note);
        }

        if (mode == kPlayKeyboard)
            tew::MidiHandler::applyNotes (midi, engine.getVoice());
        engine.render (buffer, 0, numSamples);
        return;
    }

    tew::SeqEvent events[kMaxEvents];
    const int nEvents = sequencer.advance (numSamples, bpm, true, events, kMaxEvents);

    int rendered = 0;
    for (int i = 0; i < nEvents; ++i)
    {
        const int at = events[i].offset;
        if (at > rendered)
        {
            engine.render (buffer, rendered, at - rendered);
            rendered = at;
        }

        if (events[i].on)
        {
            const auto velocity = events[i].accent ? (juce::uint8) 127 : (juce::uint8) 100;
            midi.addEvent (juce::MidiMessage::noteOn (1, events[i].note, velocity), at);
            engine.getVoice().noteOn (events[i].note, events[i].accent, events[i].slide);
        }
        else
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, events[i].note), at);
            engine.getVoice().noteOff (events[i].note);
        }
    }

    if (rendered < numSamples)
        engine.render (buffer, rendered, numSamples - rendered);
}

juce::AudioProcessorEditor* TEW03AudioProcessor::createEditor()
{
    return new TEW03AudioProcessorEditor (*this);
}

void TEW03AudioProcessor::loadBanksFromState()
{
    auto root = apvts.state;
    auto banks = root.getChildWithName (kBanks);
    const auto oldPattern = root.getChildWithName (kPattern);

    if (! banks.isValid())
    {
        banks = juce::ValueTree (kBanks);
        root.appendChild (banks, nullptr);
    }

    tew::Sequencer::Step factory[tew::Sequencer::maxSteps];

    for (int b = 0; b < tew::Sequencer::numBanks; ++b)
    {
        auto bankTree = findChildByIndex (banks, kBank, b);
        if (! bankTree.isValid())
        {
            bankTree = juce::ValueTree (kBank);
            bankTree.setProperty (kIndex, b, nullptr);
            banks.appendChild (bankTree, nullptr);
        }

        for (int p = 0; p < tew::Sequencer::patternsPerBank; ++p)
        {
            auto patTree = findChildByIndex (bankTree, kPattern, p);
            if (! patTree.isValid())
            {
                patTree = juce::ValueTree (kPattern);
                patTree.setProperty (kIndex, p, nullptr);
                bankTree.appendChild (patTree, nullptr);
            }

            tew::Sequencer::fillSlot (factory, b, p);
            const bool migrate = (b == 0 && p == 0 && oldPattern.isValid());
            const bool useFactory = ! migrate && ! slotHasNote (patTree);
            const auto src = migrate ? oldPattern : patTree;

            for (int i = 0; i < tew::Sequencer::maxSteps; ++i)
            {
                auto child = findStepChild (src, i);
                const auto step = (! useFactory && child.isValid()) ? stepFromTree (child) : factory[i];
                auto dest = findStepChild (patTree, i);
                if (! dest.isValid())
                    patTree.appendChild (makeStepTree (i, step), nullptr);
                else if (migrate || useFactory)
                {
                    dest.setProperty (kNote, step.note, nullptr);
                    dest.setProperty (kAccent, step.accent, nullptr);
                    dest.setProperty (kSlide, step.slide, nullptr);
                }
                patterns[b][p][i].store (tew::Sequencer::pack (step), std::memory_order_relaxed);
            }
        }
    }

    if (oldPattern.isValid())
        root.removeChild (oldPattern, nullptr);

    curBank.store (juce::jlimit (0, tew::Sequencer::numBanks - 1,
                                 (int) raw (ParamID::seqBank)),
                   std::memory_order_relaxed);
    curPattern.store (juce::jlimit (0, tew::Sequencer::patternsPerBank - 1,
                                    (int) raw (ParamID::seqPattern)),
                     std::memory_order_relaxed);
    sequencer.setLength (raw (ParamID::seq2x) >= 0.5f ? tew::Sequencer::maxSteps
                                                     : tew::Sequencer::numSteps);
    loadLiveFromSlot();
}

void TEW03AudioProcessor::writeBanksToState()
{
    storeLiveToSlot();

    auto banks = apvts.state.getChildWithName (kBanks);
    if (! banks.isValid())
    {
        banks = juce::ValueTree (kBanks);
        apvts.state.appendChild (banks, nullptr);
    }

    for (int b = 0; b < tew::Sequencer::numBanks; ++b)
    {
        auto bankTree = findChildByIndex (banks, kBank, b);
        if (! bankTree.isValid())
        {
            bankTree = juce::ValueTree (kBank);
            bankTree.setProperty (kIndex, b, nullptr);
            banks.appendChild (bankTree, nullptr);
        }

        for (int p = 0; p < tew::Sequencer::patternsPerBank; ++p)
        {
            auto patTree = findChildByIndex (bankTree, kPattern, p);
            if (! patTree.isValid())
            {
                patTree = juce::ValueTree (kPattern);
                patTree.setProperty (kIndex, p, nullptr);
                bankTree.appendChild (patTree, nullptr);
            }

            for (int i = 0; i < tew::Sequencer::maxSteps; ++i)
            {
                const auto step = tew::Sequencer::unpack (
                    patterns[b][p][i].load (std::memory_order_relaxed));
                auto child = findStepChild (patTree, i);
                if (! child.isValid())
                    patTree.appendChild (makeStepTree (i, step), nullptr);
                else
                {
                    child.setProperty (kNote, step.note, nullptr);
                    child.setProperty (kAccent, step.accent, nullptr);
                    child.setProperty (kSlide, step.slide, nullptr);
                }
            }
        }
    }
}

void TEW03AudioProcessor::writeStepToState (int bank, int pat, int index, tew::Sequencer::Step step)
{
    auto banks = apvts.state.getChildWithName (kBanks);
    if (! banks.isValid())
    {
        writeBanksToState();
        banks = apvts.state.getChildWithName (kBanks);
    }

    auto bankTree = findChildByIndex (banks, kBank, bank);
    auto patTree = bankTree.isValid() ? findChildByIndex (bankTree, kPattern, pat) : juce::ValueTree();
    if (! patTree.isValid())
    {
        writeBanksToState();
        return;
    }

    auto child = findStepChild (patTree, index);
    if (! child.isValid())
    {
        patTree.appendChild (makeStepTree (index, step), nullptr);
        return;
    }

    child.setProperty (kNote, step.note, nullptr);
    child.setProperty (kAccent, step.accent, nullptr);
    child.setProperty (kSlide, step.slide, nullptr);
}

void TEW03AudioProcessor::setPatternStep (int index, tew::Sequencer::Step step)
{
    sequencer.setStep (index, step);
    const int bank = currentBank();
    const int pat = currentPattern();
    patterns[bank][pat][index].store (tew::Sequencer::pack (step), std::memory_order_relaxed);
    writeStepToState (bank, pat, index, step);
}

void TEW03AudioProcessor::syncSeqLength (bool doubled)
{
    sequencer.reshapeTo (doubled ? tew::Sequencer::maxSteps : tew::Sequencer::numSteps);
    storeLiveToSlot();
    const int bank = currentBank();
    const int pat = currentPattern();
    for (int i = 0; i < tew::Sequencer::maxSteps; ++i)
        writeStepToState (bank, pat, i, sequencer.getStep (i));
}

juce::File TEW03AudioProcessor::patchesDir() const
{
    return ensureDir (libraryRoot().getChildFile ("Patches"));
}

juce::File TEW03AudioProcessor::banksDir() const
{
    return ensureDir (libraryRoot().getChildFile ("Banks"));
}

juce::StringArray TEW03AudioProcessor::patchNames() const
{
    return listStemNames (patchesDir(), "*.tew3p");
}

juce::StringArray TEW03AudioProcessor::bankNames() const
{
    return listStemNames (banksDir(), "*.tew3b");
}

juce::String TEW03AudioProcessor::patchName() const
{
    return apvts.state.getProperty (kPatchName, kInitPatch).toString();
}

juce::String TEW03AudioProcessor::bankName() const
{
    return apvts.state.getProperty (kBankName, kInitBank).toString();
}

void TEW03AudioProcessor::copyLiveBanks (tew::Sequencer::Step dest[tew::Sequencer::numBanks]
                                                               [tew::Sequencer::patternsPerBank]
                                                               [tew::Sequencer::maxSteps])
{
    storeLiveToSlot();
    for (int b = 0; b < tew::Sequencer::numBanks; ++b)
        for (int p = 0; p < tew::Sequencer::patternsPerBank; ++p)
            for (int i = 0; i < tew::Sequencer::maxSteps; ++i)
                dest[b][p][i] = tew::Sequencer::unpack (
                    patterns[b][p][i].load (std::memory_order_relaxed));
}

void TEW03AudioProcessor::applyBankSteps (const tew::Sequencer::Step src[tew::Sequencer::numBanks]
                                                                      [tew::Sequencer::patternsPerBank]
                                                                      [tew::Sequencer::maxSteps])
{
    for (int b = 0; b < tew::Sequencer::numBanks; ++b)
        for (int p = 0; p < tew::Sequencer::patternsPerBank; ++p)
            for (int i = 0; i < tew::Sequencer::maxSteps; ++i)
                patterns[b][p][i].store (tew::Sequencer::pack (src[b][p][i]), std::memory_order_relaxed);
    writeBanksToState();
    loadLiveFromSlot();
}

void TEW03AudioProcessor::applyPatchParams (const std::vector<tew::PatchParam>& params)
{
    for (const auto& p : params)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (p.id));
        if (param == nullptr || ! tew::storeInPatch (p.id.c_str()))
            continue;
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (p.value));
        param->endChangeGesture();
    }
    syncSeqLength (raw (ParamID::seq2x) >= 0.5f);
}

std::vector<tew::PatchParam> TEW03AudioProcessor::currentPatchParams() const
{
    std::vector<tew::PatchParam> params;
    for (auto* base : getParameters())
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*> (base);
        if (param == nullptr)
            continue;
        const auto id = param->getParameterID().toStdString();
        if (! tew::storeInPatch (id.c_str()))
            continue;
        params.push_back ({ id, param->convertFrom0to1 (param->getValue()) });
    }
    return params;
}

void TEW03AudioProcessor::initPatch()
{
    for (auto* base : getParameters())
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*> (base);
        if (param == nullptr || ! tew::storeInPatch (param->getParameterID().toStdString().c_str()))
            continue;
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->getDefaultValue());
        param->endChangeGesture();
    }
    syncSeqLength (raw (ParamID::seq2x) >= 0.5f);
    apvts.state.setProperty (kPatchName, kInitPatch, nullptr);
}

void TEW03AudioProcessor::initBank()
{
    tew::Sequencer::Step steps[tew::Sequencer::numBanks][tew::Sequencer::patternsPerBank][tew::Sequencer::maxSteps];
    for (int b = 0; b < tew::Sequencer::numBanks; ++b)
        for (int p = 0; p < tew::Sequencer::patternsPerBank; ++p)
            tew::Sequencer::fillSlot (steps[b][p], b, p);
    applyBankSteps (steps);
    apvts.state.setProperty (kBankName, kInitBank, nullptr);
}

bool TEW03AudioProcessor::loadPatchFile (const juce::File& src, const juce::String& shownName)
{
    std::vector<tew::PatchParam> params;
    if (! tew::readPatchXml (readText (src), params))
        return false;
    applyPatchParams (params);
    apvts.state.setProperty (kPatchName, shownName, nullptr);
    return true;
}

bool TEW03AudioProcessor::loadBankFile (const juce::File& src, const juce::String& shownName)
{
    tew::Sequencer::Step steps[tew::Sequencer::numBanks][tew::Sequencer::patternsPerBank][tew::Sequencer::maxSteps];
    if (! tew::readBankXml (readText (src), steps))
        return false;
    applyBankSteps (steps);
    apvts.state.setProperty (kBankName, shownName, nullptr);
    return true;
}

bool TEW03AudioProcessor::loadPatchByName (const juce::String& name)
{
    const auto stem = legalStem (name);
    if (stem.isEmpty() || stem == kInitPatch)
    {
        initPatch();
        return true;
    }
    return loadPatchFile (patchesDir().getChildFile (stem + kPatchExt), stem);
}

bool TEW03AudioProcessor::loadBankByName (const juce::String& name)
{
    const auto stem = legalStem (name);
    if (stem.isEmpty() || stem == kInitBank)
    {
        initBank();
        return true;
    }
    return loadBankFile (banksDir().getChildFile (stem + kBankExt), stem);
}

bool TEW03AudioProcessor::savePatchAs (const juce::String& name)
{
    const auto stem = legalStem (name);
    if (stem.isEmpty() || stem == kInitPatch)
        return false;
    if (! writeText (patchesDir().getChildFile (stem + kPatchExt), tew::writePatchXml (currentPatchParams())))
        return false;
    apvts.state.setProperty (kPatchName, stem, nullptr);
    return true;
}

bool TEW03AudioProcessor::saveBankAs (const juce::String& name)
{
    const auto stem = legalStem (name);
    if (stem.isEmpty() || stem == kInitBank)
        return false;
    tew::Sequencer::Step steps[tew::Sequencer::numBanks][tew::Sequencer::patternsPerBank][tew::Sequencer::maxSteps];
    copyLiveBanks (steps);
    if (! writeText (banksDir().getChildFile (stem + kBankExt), tew::writeBankXml (steps)))
        return false;
    apvts.state.setProperty (kBankName, stem, nullptr);
    return true;
}

bool TEW03AudioProcessor::savePatch()
{
    const auto name = patchName();
    if (name.isEmpty() || name == kInitPatch)
        return false;
    return savePatchAs (name);
}

bool TEW03AudioProcessor::saveBank()
{
    const auto name = bankName();
    if (name.isEmpty() || name == kInitBank)
        return false;
    return saveBankAs (name);
}

bool TEW03AudioProcessor::exportPatch (const juce::File& dest)
{
    auto file = dest;
    if (! file.hasFileExtension ("tew3p"))
        file = file.withFileExtension ("tew3p");
    return writeText (file, tew::writePatchXml (currentPatchParams()));
}

bool TEW03AudioProcessor::exportBank (const juce::File& dest)
{
    auto file = dest;
    if (! file.hasFileExtension ("tew3b"))
        file = file.withFileExtension ("tew3b");
    tew::Sequencer::Step steps[tew::Sequencer::numBanks][tew::Sequencer::patternsPerBank][tew::Sequencer::maxSteps];
    copyLiveBanks (steps);
    return writeText (file, tew::writeBankXml (steps));
}

bool TEW03AudioProcessor::importPatch (const juce::File& src)
{
    const auto stem = legalStem (src.getFileNameWithoutExtension());
    if (stem.isEmpty())
        return false;
    const auto dest = patchesDir().getChildFile (stem + kPatchExt);
    if (src.getFullPathName() != dest.getFullPathName())
    {
        dest.deleteFile();
        if (! src.copyFileTo (dest))
            return false;
    }
    return loadPatchFile (dest, stem);
}

bool TEW03AudioProcessor::importBank (const juce::File& src)
{
    const auto stem = legalStem (src.getFileNameWithoutExtension());
    if (stem.isEmpty())
        return false;
    const auto dest = banksDir().getChildFile (stem + kBankExt);
    if (src.getFullPathName() != dest.getFullPathName())
    {
        dest.deleteFile();
        if (! src.copyFileTo (dest))
            return false;
    }
    return loadBankFile (dest, stem);
}

void TEW03AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    writeBanksToState();

    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void TEW03AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            loadBanksFromState();
            if (! apvts.state.hasProperty (kPatchName))
                apvts.state.setProperty (kPatchName, kInitPatch, nullptr);
            if (! apvts.state.hasProperty (kBankName))
                apvts.state.setProperty (kBankName, kInitBank, nullptr);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TEW03AudioProcessor();
}
