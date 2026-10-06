#pragma once

#include "../parameters/ParameterIDs.h"

#include <JuceHeader.h>
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
        phaser.prepare (spec);
        compressor.prepare (spec);
        reverb.prepare (spec);
        svf.prepare (spec);
        delay.prepare (spec);
        delay.setMaximumDelayInSamples ((int) std::ceil (sr * 2.0) + 8);
        delay.reset();
        lp.prepare (spec);
        lowShelf.prepare (spec);
        midPeak.prepare (spec);
        highShelf.prepare (spec);
        scratch.setSize (ch, n);
        prepared = true;
    }

    void process (juce::AudioBuffer<float>& buffer, float bpm,
                  juce::AudioProcessorValueTreeState& apvts, std::uint64_t packedOrder)
    {
        if (! prepared || buffer.getNumSamples() <= 0)
            return;

        int order[fxCount];
        int n = unpackFxOrder (packedOrder, order);
        bool used[fxCount] {};
        auto on = [&] (int t) -> bool
        {
            auto* p = apvts.getRawParameterValue (ParamID::fxOnIds[t]);
            return p != nullptr && p->load() >= 0.5f;
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
    }

private:
    float p (juce::AudioProcessorValueTreeState& apvts, const char* id) const
    {
        auto* v = apvts.getRawParameterValue (id);
        return v != nullptr ? v->load() : 0.f;
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
                chorus.setCentreDelay (7.5f);
                chorus.process (ctx);
                break;
            }
            case 1:
            {
                compressor.setThreshold (p (apvts, ParamID::fxCmpThr));
                compressor.setRatio (p (apvts, ParamID::fxCmpRat));
                compressor.setAttack (p (apvts, ParamID::fxCmpAtk));
                compressor.setRelease (p (apvts, ParamID::fxCmpRel));
                const float mix = p (apvts, ParamID::fxCmpMix);
                ensureScratch (buffer);
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    scratch.copyFrom (ch, 0, buffer, ch, 0, buffer.getNumSamples());
                auto wetBlock = juce::dsp::AudioBlock<float> (scratch).getSubBlock (0, (size_t) buffer.getNumSamples());
                juce::dsp::ProcessContextReplacing<float> wetCtx (wetBlock);
                compressor.process (wetCtx);
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                {
                    auto* dry = buffer.getWritePointer (ch);
                    const auto* wet = scratch.getReadPointer (ch);
                    for (int i = 0; i < buffer.getNumSamples(); ++i)
                        dry[i] += mix * (wet[i] - dry[i]);
                }
                break;
            }
            case 2:
                processDelay (buffer, bpm, apvts);
                break;
            case 3:
            {
                const float drive = 1.f + p (apvts, ParamID::fxDstDrive) * 12.f;
                const float mix = p (apvts, ParamID::fxDstMix);
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                {
                    auto* s = buffer.getWritePointer (ch);
                    for (int i = 0; i < buffer.getNumSamples(); ++i)
                    {
                        const float wet = std::tanh (s[i] * drive);
                        s[i] += mix * (wet - s[i]);
                    }
                }
                break;
            }
            case 4:
            {
                const float lowG = p (apvts, ParamID::fxEqLow);
                const float midG = p (apvts, ParamID::fxEqMid);
                const float highG = p (apvts, ParamID::fxEqHigh);
                *lowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (sr, 120.f, 0.7f, juce::Decibels::decibelsToGain (lowG));
                *midPeak.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, 1000.f, 0.8f, juce::Decibels::decibelsToGain (midG));
                *highShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 4000.f, 0.7f, juce::Decibels::decibelsToGain (highG));
                lowShelf.process (ctx);
                midPeak.process (ctx);
                highShelf.process (ctx);
                break;
            }
            case 5:
            {
                const int kind = juce::roundToInt (p (apvts, ParamID::fxFltType));
                svf.setType (kind == 1 ? juce::dsp::StateVariableTPTFilterType::bandpass
                           : kind == 2 ? juce::dsp::StateVariableTPTFilterType::highpass
                                       : juce::dsp::StateVariableTPTFilterType::lowpass);
                svf.setCutoffFrequency (p (apvts, ParamID::fxFltCut));
                svf.setResonance (0.1f + p (apvts, ParamID::fxFltRes) * 8.f);
                const float mix = p (apvts, ParamID::fxFltMix);
                ensureScratch (buffer);
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    scratch.copyFrom (ch, 0, buffer, ch, 0, buffer.getNumSamples());
                svf.process (ctx);
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
                break;
            }
            case 6:
            {
                flanger.setRate (0.05f + p (apvts, ParamID::fxFlaRate) * 4.95f);
                flanger.setDepth (p (apvts, ParamID::fxFlaDepth));
                flanger.setFeedback (p (apvts, ParamID::fxFlaFb) * 0.9f);
                flanger.setMix (p (apvts, ParamID::fxFlaMix));
                flanger.setCentreDelay (1.6f);
                flanger.process (ctx);
                break;
            }
            case 7:
            {
                phaser.setRate (0.05f + p (apvts, ParamID::fxPhaRate) * 7.95f);
                phaser.setDepth (p (apvts, ParamID::fxPhaDepth));
                phaser.setFeedback (p (apvts, ParamID::fxPhaFb) * 0.85f);
                phaser.setMix (p (apvts, ParamID::fxPhaMix));
                phaser.setCentreFrequency (p (apvts, ParamID::fxPhaCentre));
                phaser.process (ctx);
                break;
            }
            case 8:
            {
                juce::dsp::Reverb::Parameters rp;
                rp.roomSize = p (apvts, ParamID::fxRevSize);
                rp.damping = p (apvts, ParamID::fxRevDamp);
                const float mix = p (apvts, ParamID::fxRevMix);
                rp.wetLevel = mix;
                rp.dryLevel = 1.f - mix * 0.85f;
                rp.width = p (apvts, ParamID::fxRevWidth);
                rp.freezeMode = 0.f;
                reverb.setParameters (rp);
                reverb.process (ctx);
                break;
            }
            default:
                break;
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
        delay.setDelay (delaySamp);

        const float fb = p (apvts, ParamID::fxDlyFb) * 0.92f;
        const float mix = p (apvts, ParamID::fxDlyMix);
        const float cut = 200.f + p (apvts, ParamID::fxDlyCut) * 9800.f;
        *lp.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, cut, 0.7f);

        ensureScratch (buffer);
        scratch.clear();
        const int n = buffer.getNumSamples();
        const int chs = buffer.getNumChannels();
        for (int i = 0; i < n; ++i)
        {
            for (int ch = 0; ch < chs; ++ch)
            {
                const float in = buffer.getSample (ch, i);
                float d = delay.popSample (ch);
                scratch.setSample (ch, i, d);
                delay.pushSample (ch, in + d * fb);
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

    void ensureScratch (const juce::AudioBuffer<float>& buffer)
    {
        // ponytail: grow only if host exceeds prepare size; rare first-block case
        if (scratch.getNumSamples() < buffer.getNumSamples()
            || scratch.getNumChannels() < buffer.getNumChannels())
            scratch.setSize (buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
    }

    using DupIIR = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                  juce::dsp::IIR::Coefficients<float>>;

    juce::dsp::Chorus<float> chorus;
    juce::dsp::Chorus<float> flanger;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Reverb reverb;
    juce::dsp::StateVariableTPTFilter<float> svf;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 192000 };
    DupIIR lp, lowShelf, midPeak, highShelf;
    juce::AudioBuffer<float> scratch;
    double sr = 44100.0;
    bool prepared = false;
};

} // namespace tew
