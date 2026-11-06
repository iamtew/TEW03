#pragma once

#include "Compressor.h"
#include "ParametricEq.h"
#include "Lfo.h"
#include "../parameters/ParameterIDs.h"

#include <JuceHeader.h>
#include <atomic>
#include <cmath>

namespace tew
{

struct FxChain
{
    void prepare (double sampleRate, int samplesPerBlock, int numChannels)
    {
        sr = sampleRate > 1.0 ? sampleRate : 44100.0;
        const int ch = juce::jmax (1, numChannels);
        const int n = juce::jmax (samplesPerBlock, 64);
        juce::dsp::ProcessSpec spec { sr, (juce::uint32) n, (juce::uint32) ch };

        chorus.prepare (spec);
        flanger.prepare (spec);
        reverb.prepare (spec);
        svf.prepare (spec);
        svf2.prepare (spec);
        delay.prepare (spec);
        delay.setMaximumDelayInSamples ((int) std::ceil (sr * 2.0) + 8);
        delay.reset();
        choWidthDly.prepare (spec);
        choWidthDly.setMaximumDelayInSamples ((int) std::ceil (sr * 0.03) + 8);
        choWidthDly.reset();
        flaWidthDly.prepare (spec);
        flaWidthDly.setMaximumDelayInSamples ((int) std::ceil (sr * 0.03) + 8);
        flaWidthDly.reset();
        revPreDly.prepare (spec);
        revPreDly.setMaximumDelayInSamples ((int) std::ceil (sr * 0.25) + 8);
        revPreDly.reset();
        lp.prepare (spec);
        insertEq.prepare (sampleRate, samplesPerBlock, ch);
        scratch.setSize (ch, n);
        cmpGr = 0.f;
        cmpHpZ[0] = cmpHpZ[1] = 0.f;
        dlyHpZ[0] = dlyHpZ[1] = 0.f;
        dstLpZ[0] = dstLpZ[1] = 0.f;
        revHpZ[0] = revHpZ[1] = 0.f;
        for (int c = 0; c < 2; ++c)
        {
            phaFbZ[c] = 0.f;
            for (int s = 0; s < 8; ++s)
                phaZ[c][s] = 0.f;
        }
        phaPhase = 0.f;
        prepared = true;
    }

    float gainReductionDb() const { return grDb.load (std::memory_order_relaxed); }

    // lfoY is the block peek already advanced by the voice. Do not step it here.
    void process (juce::AudioBuffer<float>& buffer, float bpm,
                  juce::AudioProcessorValueTreeState& apvts, std::uint64_t packedOrder,
                  const float* lfoY)
    {
        if (! prepared || buffer.getNumSamples() <= 0)
            return;

        blockLfo = lfoY;

        int order[fxCount];
        int n = unpackFxOrder (packedOrder, order);
        bool used[fxCount] {};
        auto on = [&] (int t) -> bool
        {
            auto* pOn = apvts.getRawParameterValue (ParamID::fxOnIds[t]);
            return pOn != nullptr && pOn->load() >= 0.5f;
        };
        for (int i = 0; i < n; ++i)
        {
            used[order[i]] = true;
            if (on (order[i]))
                processOne (order[i], buffer, bpm, apvts);
        }
        for (int t = 0; t < fxCount; ++t)
            if (! used[t] && on (t))
                processOne (t, buffer, bpm, apvts);
        blockLfo = nullptr;
    }

private:
    float p (juce::AudioProcessorValueTreeState& apvts, const char* id) const
    {
        auto* v = apvts.getRawParameterValue (id);
        float raw = v != nullptr ? v->load() : 0.f;
        if (blockLfo == nullptr)
            return raw;

        const auto* slot = ParamID::fxModFor (id);
        if (slot == nullptr)
            return raw;
        auto* srcA = apvts.getRawParameterValue (slot->lfo);
        auto* amtA = apvts.getRawParameterValue (slot->amt);
        if (srcA == nullptr || amtA == nullptr)
            return raw;
        const int src = juce::roundToInt (srcA->load());
        if (src <= 0 || src > numLfos)
            return raw;

        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id));
        if (ranged == nullptr)
            return raw;
        const float bipolar = blockLfo[src - 1] * 2.f - 1.f;
        const float n = juce::jlimit (0.f, 1.f, ranged->convertTo0to1 (raw) + amtA->load() * bipolar);
        return ranged->convertFrom0to1 (n);
    }

    static float onePoleCoeff (float hz, double sampleRate)
    {
        const float ny = (float) sampleRate * 0.45f;
        hz = juce::jlimit (1.f, ny, hz);
        return 1.f - std::exp (-2.f * 3.14159265f * hz / (float) sampleRate);
    }

    static float hp1 (float x, float& z, float a)
    {
        z += a * (x - z);
        return x - z;
    }

    static float lp1 (float x, float& z, float a)
    {
        z += a * (x - z);
        return z;
    }

    static float shapeDist (float x, int type)
    {
        switch (type)
        {
            case 1: return juce::jlimit (-1.f, 1.f, x);
            case 2:
            {
                while (x > 1.f || x < -1.f)
                {
                    if (x > 1.f)
                        x = 2.f - x;
                    if (x < -1.f)
                        x = -2.f - x;
                }
                return x;
            }
            case 3: return std::abs (std::tanh (x));
            default: return std::tanh (x);
        }
    }

    void applyRDelay (juce::AudioBuffer<float>& buffer,
                      juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>& line,
                      float ms)
    {
        if (buffer.getNumChannels() < 2)
            return;
        const float samp = juce::jlimit (1.f, (float) line.getMaximumDelayInSamples() - 1.f,
                                         ms * 0.001f * (float) sr);
        line.setDelay (samp);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
        {
            const float in = buffer.getSample (1, i);
            buffer.setSample (1, i, line.popSample (1));
            line.pushSample (1, in);
        }
    }

    void processOne (int type, juce::AudioBuffer<float>& buffer, float bpm,
                     juce::AudioProcessorValueTreeState& apvts)
    {
        auto block = juce::dsp::AudioBlock<float> (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);

        switch (type)
        {
            case 0:
            {
                chorus.setRate (0.05f + p (apvts, ParamID::fxChoRate) * 7.95f);
                chorus.setDepth (p (apvts, ParamID::fxChoDepth));
                chorus.setFeedback (p (apvts, ParamID::fxChoFb) * 0.9f);
                chorus.setMix (p (apvts, ParamID::fxChoMix));
                chorus.setCentreDelay (p (apvts, ParamID::fxChoCentre));
                chorus.process (ctx);
                applyRDelay (buffer, choWidthDly, p (apvts, ParamID::fxChoWidth) * 12.f);
                break;
            }
            case 1:
                processComp (buffer, apvts);
                break;
            case 2:
                processDelay (buffer, bpm, apvts);
                break;
            case 3:
                processDist (buffer, apvts);
                break;
            case 4:
            {
                EqBandParam bands[eqBandCount];
                for (int b = 0; b < eqBandCount; ++b)
                {
                    bands[b].type = juce::roundToInt (p (apvts, ParamID::fxEqType[b]));
                    bands[b].hz = p (apvts, ParamID::fxEqFreq[b]);
                    bands[b].gainDb = p (apvts, ParamID::fxEqGain[b]);
                    bands[b].q = p (apvts, ParamID::fxEqQ[b]);
                }
                insertEq.process (buffer, true, bands);
                const float trim = std::pow (10.f, p (apvts, ParamID::fxEqTrim) / 20.f);
                if (std::abs (trim - 1.f) > 1.0e-4f)
                    buffer.applyGain (trim);
                break;
            }
            case 5:
                processFilter (buffer, apvts, ctx);
                break;
            case 6:
            {
                flanger.setRate (0.05f + p (apvts, ParamID::fxFlaRate) * 4.95f);
                flanger.setDepth (p (apvts, ParamID::fxFlaDepth));
                flanger.setFeedback (p (apvts, ParamID::fxFlaFb) * 0.9f);
                flanger.setMix (p (apvts, ParamID::fxFlaMix));
                flanger.setCentreDelay (p (apvts, ParamID::fxFlaCentre));
                flanger.process (ctx);
                applyRDelay (buffer, flaWidthDly, p (apvts, ParamID::fxFlaWidth) * 8.f);
                break;
            }
            case 7:
                processPhaser (buffer, apvts);
                break;
            case 8:
                processReverb (buffer, apvts);
                break;
            default:
                break;
        }
    }

    void processComp (juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts)
    {
        const float thr = p (apvts, ParamID::fxCmpThr);
        const float ratio = juce::jmax (1.f, p (apvts, ParamID::fxCmpRat));
        const float atkMs = p (apvts, ParamID::fxCmpAtk);
        const float relMs = p (apvts, ParamID::fxCmpRel);
        const float mix = p (apvts, ParamID::fxCmpMix);
        const float makeup = std::pow (10.f, p (apvts, ParamID::fxCmpMake) / 20.f);
        const float knee = p (apvts, ParamID::fxCmpKnee);
        const float hpA = onePoleCoeff (p (apvts, ParamID::fxCmpHpf), sr);
        const float atkA = 1.f - std::exp (-1.f / juce::jmax (1.f, atkMs * 0.001f * (float) sr));
        const float relA = 1.f - std::exp (-1.f / juce::jmax (1.f, relMs * 0.001f * (float) sr));
        const int n = buffer.getNumSamples();
        const int chs = juce::jmin (2, buffer.getNumChannels());
        float peakGr = 0.f;
        for (int i = 0; i < n; ++i)
        {
            float det = 0.f;
            for (int ch = 0; ch < chs; ++ch)
            {
                const float x = buffer.getSample (ch, i);
                det = juce::jmax (det, std::abs (hp1 (x, cmpHpZ[ch], hpA)));
            }
            const float envDb = 20.f * std::log10 (det + 1.0e-8f);
            const float instant = compressorGainReductionDb (envDb, thr, ratio, knee);
            const float coeff = instant > cmpGr ? atkA : relA;
            cmpGr += coeff * (instant - cmpGr);
            peakGr = juce::jmax (peakGr, cmpGr);
            const float g = std::pow (10.f, -cmpGr / 20.f) * makeup;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                const float dry = buffer.getSample (ch, i);
                buffer.setSample (ch, i, dry + mix * (dry * g - dry));
            }
        }
        grDb.store (peakGr, std::memory_order_relaxed);
    }

    void processDist (juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts)
    {
        const float drive = 1.f + p (apvts, ParamID::fxDstDrive) * 12.f;
        const float mix = p (apvts, ParamID::fxDstMix);
        const int kind = juce::roundToInt (p (apvts, ParamID::fxDstType));
        const float bias = p (apvts, ParamID::fxDstBias) * 0.5f;
        const float toneHz = 400.f + p (apvts, ParamID::fxDstTone) * 11600.f;
        const float a = onePoleCoeff (toneHz, sr);
        const int n = buffer.getNumSamples();
        const int chs = buffer.getNumChannels();
        for (int i = 0; i < n; ++i)
        {
            for (int ch = 0; ch < chs; ++ch)
            {
                const float dry = buffer.getSample (ch, i);
                float wet = shapeDist ((dry + bias) * drive, kind);
                wet = lp1 (wet, dstLpZ[ch], a);
                buffer.setSample (ch, i, dry + mix * (wet - dry));
            }
        }
    }

    void processFilter (juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts,
                        juce::dsp::ProcessContextReplacing<float>& ctx)
    {
        const int kind = juce::roundToInt (p (apvts, ParamID::fxFltType));
        const auto svType = kind == 1 ? juce::dsp::StateVariableTPTFilterType::bandpass
                          : kind == 2 ? juce::dsp::StateVariableTPTFilterType::highpass
                                      : juce::dsp::StateVariableTPTFilterType::lowpass;
        svf.setType (svType);
        svf2.setType (svType);
        const float cut = p (apvts, ParamID::fxFltCut);
        const float res = 0.1f + p (apvts, ParamID::fxFltRes) * 8.f;
        svf.setCutoffFrequency (cut);
        svf2.setCutoffFrequency (cut);
        svf.setResonance (res);
        svf2.setResonance (res);
        const float mix = p (apvts, ParamID::fxFltMix);
        const float drive = p (apvts, ParamID::fxFltDrive);
        const bool steep = juce::roundToInt (p (apvts, ParamID::fxFltSlope)) >= 1;
        if (! scratchFits (buffer))
            return;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            scratch.copyFrom (ch, 0, buffer, ch, 0, buffer.getNumSamples());
        if (drive > 0.001f)
        {
            const float amt = 1.f + drive * 8.f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* s = buffer.getWritePointer (ch);
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                    s[i] = std::tanh (s[i] * amt);
            }
        }
        svf.process (ctx);
        if (steep)
            svf2.process (ctx);
        if (mix < 0.999f)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* wet = buffer.getWritePointer (ch);
                const auto* dry = scratch.getReadPointer (ch);
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                    wet[i] = dry[i] + mix * (wet[i] - dry[i]);
            }
        }
    }

    static float allpass (float x, float a, float& z)
    {
        const float y = a * x + z;
        z = x - a * y;
        return y;
    }

    void processPhaser (juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts)
    {
        const float rate = 0.05f + p (apvts, ParamID::fxPhaRate) * 7.95f;
        const float depth = p (apvts, ParamID::fxPhaDepth);
        const float fb = p (apvts, ParamID::fxPhaFb) * 0.85f;
        const float mix = p (apvts, ParamID::fxPhaMix);
        const float centre = p (apvts, ParamID::fxPhaCentre);
        const int stages = 2 * (juce::jlimit (0, 3, juce::roundToInt (p (apvts, ParamID::fxPhaStages))) + 1);
        const float dPhase = rate / (float) sr;
        const int n = buffer.getNumSamples();
        const int chs = buffer.getNumChannels();
        for (int i = 0; i < n; ++i)
        {
            phaPhase += dPhase;
            if (phaPhase >= 1.f)
                phaPhase -= 1.f;
            const float lfo = std::sin (phaPhase * 6.2831853f);
            const float hz = juce::jlimit (40.f, (float) sr * 0.4f,
                                           centre * std::pow (2.f, depth * lfo));
            const float w = juce::jlimit (0.01f, 1.5f, std::tan (3.14159265f * hz / (float) sr));
            const float a = (1.f - w) / (1.f + w);
            for (int ch = 0; ch < chs; ++ch)
            {
                const float dry = buffer.getSample (ch, i);
                float x = dry + phaFbZ[ch] * fb;
                auto* z = phaZ[ch];
                for (int s = 0; s < stages; ++s)
                    x = allpass (x, a, z[s]);
                phaFbZ[ch] = x;
                buffer.setSample (ch, i, dry + mix * (x - dry));
            }
        }
    }

    void processReverb (juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts)
    {
        const float mix = p (apvts, ParamID::fxRevMix);
        const float preMs = p (apvts, ParamID::fxRevPre);
        const bool freeze = p (apvts, ParamID::fxRevFreeze) >= 0.5f;
        const float loA = onePoleCoeff (20.f + p (apvts, ParamID::fxRevLo) * 480.f, sr);
        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = p (apvts, ParamID::fxRevSize);
        rp.damping = p (apvts, ParamID::fxRevDamp);
        rp.wetLevel = 1.f;
        rp.dryLevel = 0.f;
        rp.width = p (apvts, ParamID::fxRevWidth);
        rp.freezeMode = freeze ? 1.f : 0.f;
        reverb.setParameters (rp);

        if (! scratchFits (buffer))
            return;
        const int n = buffer.getNumSamples();
        const int chs = buffer.getNumChannels();
        const float preSamp = juce::jlimit (1.f, (float) revPreDly.getMaximumDelayInSamples() - 1.f,
                                            juce::jmax (1.f, preMs * 0.001f * (float) sr));
        revPreDly.setDelay (preSamp);
        scratch.clear();
        for (int i = 0; i < n; ++i)
        {
            for (int ch = 0; ch < chs; ++ch)
            {
                scratch.setSample (ch, i, revPreDly.popSample (ch));
                revPreDly.pushSample (ch, buffer.getSample (ch, i));
            }
        }
        auto wetBlock = juce::dsp::AudioBlock<float> (scratch).getSubBlock (0, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> wetCtx (wetBlock);
        reverb.process (wetCtx);
        for (int ch = 0; ch < chs; ++ch)
        {
            auto* dry = buffer.getWritePointer (ch);
            auto* wet = scratch.getWritePointer (ch);
            for (int i = 0; i < n; ++i)
            {
                wet[i] = hp1 (wet[i], revHpZ[ch], loA);
                dry[i] += mix * (wet[i] - dry[i]);
            }
        }
    }

    void processDelay (juce::AudioBuffer<float>& buffer, float bpm,
                       juce::AudioProcessorValueTreeState& apvts)
    {
        const bool sync = p (apvts, ParamID::fxDlySync) >= 0.5f;
        float delaySec = 0.01f + p (apvts, ParamID::fxDlyTime) * 1.99f;
        if (sync)
        {
            const int div = juce::jlimit (0, 3, juce::roundToInt (p (apvts, ParamID::fxDlyDiv)));
            const float beats = div == 0 ? 0.25f : div == 1 ? 0.5f : div == 2 ? 1.f : 2.f;
            const float b = bpm > 1.f ? bpm : 120.f;
            delaySec = beats * 60.f / b;
        }
        const float delaySamp = juce::jlimit (1.f, (float) delay.getMaximumDelayInSamples() - 1,
                                              delaySec * (float) sr);
        const float spread = p (apvts, ParamID::fxDlySpread) * delaySamp * 0.25f;
        const bool ping = p (apvts, ParamID::fxDlyPing) >= 0.5f;
        delay.setDelay (delaySamp);

        const float fb = p (apvts, ParamID::fxDlyFb) * 0.92f;
        const float mix = p (apvts, ParamID::fxDlyMix);
        const float cut = 200.f + p (apvts, ParamID::fxDlyCut) * 9800.f;
        *lp.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, cut, 0.7f);
        const float hpA = onePoleCoeff (20.f + p (apvts, ParamID::fxDlyHp) * 1980.f, sr);

        if (! scratchFits (buffer))
            return;
        scratch.clear();
        const int n = buffer.getNumSamples();
        const int chs = buffer.getNumChannels();
        for (int i = 0; i < n; ++i)
        {
            float d[2] {};
            for (int ch = 0; ch < chs; ++ch)
            {
                const float extra = (ch == 1) ? spread : 0.f;
                delay.setDelay (juce::jlimit (1.f, (float) delay.getMaximumDelayInSamples() - 1.f,
                                              delaySamp + extra));
                d[ch] = delay.popSample (ch);
            }
            for (int ch = 0; ch < chs; ++ch)
            {
                const float in = buffer.getSample (ch, i);
                const float back = ping && chs > 1 ? d[1 - ch] : d[ch];
                const float fbIn = hp1 (back, dlyHpZ[ch], hpA);
                scratch.setSample (ch, i, back);
                delay.pushSample (ch, in + fbIn * fb);
            }
        }
        auto wetBlock = juce::dsp::AudioBlock<float> (scratch).getSubBlock (0, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> wetCtx (wetBlock);
        lp.process (wetCtx);
        for (int ch = 0; ch < chs; ++ch)
        {
            auto* dry = buffer.getWritePointer (ch);
            const auto* wet = scratch.getReadPointer (ch);
            for (int i = 0; i < n; ++i)
                dry[i] += mix * (wet[i] - dry[i]);
        }
    }

    bool scratchFits (const juce::AudioBuffer<float>& buffer) const
    {
        return scratch.getNumSamples() >= buffer.getNumSamples()
            && scratch.getNumChannels() >= buffer.getNumChannels();
    }

    using DupIIR = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                  juce::dsp::IIR::Coefficients<float>>;

    juce::dsp::Chorus<float> chorus;
    juce::dsp::Chorus<float> flanger;
    juce::dsp::Reverb reverb;
    juce::dsp::StateVariableTPTFilter<float> svf;
    juce::dsp::StateVariableTPTFilter<float> svf2;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 192000 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> choWidthDly { 8192 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> flaWidthDly { 8192 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> revPreDly { 192000 };
    DupIIR lp;
    ParametricEq insertEq;
    juce::AudioBuffer<float> scratch;
    double sr = 44100.0;
    bool prepared = false;
    const float* blockLfo = nullptr;
    float cmpGr = 0.f;
    float cmpHpZ[2] {};
    float dlyHpZ[2] {};
    float dstLpZ[2] {};
    float revHpZ[2] {};
    float phaZ[2][8] {};
    float phaFbZ[2] {};
    float phaPhase = 0.f;
    std::atomic<float> grDb { 0.f };
};

} // namespace tew
