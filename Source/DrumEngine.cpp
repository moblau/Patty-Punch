#include "DrumEngine.h"

namespace
{
float decayMul (float seconds, double sr, float end = 1.0e-4f) noexcept
{
    return static_cast<float> (std::exp (std::log (end) /
        juce::jmax (1.0, static_cast<double> (seconds) * sr)));
}

float noise (uint32_t& state) noexcept
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return static_cast<float> (static_cast<int32_t> (state)) / 2147483648.0f;
}

float pitchRatio (float semitones) noexcept
{
    return std::exp2 (semitones / 12.0f);
}

float timeRatio (float modulation) noexcept
{
    return std::exp2 (2.0f * juce::jlimit (-1.0f, 1.0f, modulation));
}

float saturate (float input, float drive) noexcept
{
    drive = juce::jmax (1.0f, drive);
    return std::tanh (input * drive) / std::tanh (drive);
}
}

float evaluateLfoWaveform (int shape, float phase) noexcept
{
    if (! std::isfinite (phase))
        return 0;
    phase -= std::floor (phase);
    if (shape == 1) return 1.0f - 4.0f * std::abs (phase - .5f);
    if (shape == 2) return phase < .5f ? 1.0f : -1.0f;
    return std::sin (juce::MathConstants<float>::twoPi * phase);
}

float repeatCurvePosition (int repeatIndex, int repeatCount, float shape) noexcept
{
    if (repeatCount <= 0 || repeatIndex <= 0)
        return 0;
    const auto x = juce::jlimit (0.0f, 1.0f,
        static_cast<float> (repeatIndex) / static_cast<float> (repeatCount));
    const auto s = juce::jlimit (0.0f, 1.0f, shape);
    return std::pow (x, 1.0f + 3.0f * s);
}

float repeatCurveGain (int repeatIndex, int repeatCount, float shape) noexcept
{
    if (repeatCount <= 0 || repeatIndex <= 0)
        return 1;
    const auto x = juce::jlimit (0.0f, 1.0f,
        static_cast<float> (repeatIndex) / static_cast<float> (repeatCount));
    const auto s = juce::jlimit (0.0f, 1.0f, shape);
    return std::pow (.78f, static_cast<float> (repeatIndex)) * std::exp (-2.0f * s * x);
}

void KickVoice::prepare (double sampleRate)
{
    sr = sampleRate;
    clickLP.reset();
    env = clickEnv = 0;
}

void KickVoice::trigger (float velocity, float pitchMultiplier, const DrumParameters& p)
{
    const auto safeMaximum = static_cast<float> (sr * .4);
    target = juce::jlimit (15.0f, safeMaximum, p.kickTune * pitchMultiplier);
    freq = juce::jlimit (target, safeMaximum, target * pitchRatio (p.kickSweep));
    sweepMul = static_cast<float> (std::pow (target / freq,
        1.0 / juce::jmax (1.0, static_cast<double> (p.kickSweepTime) * sr)));
    env = juce::jmap (velocity, .18f, 1.0f);
    envMul = decayMul (p.kickDecay, sr);
    clickEnv = env;
    clickMul = decayMul (.008f, sr);
    clickAmount = p.kickClick;
    clickLP.setLowPass (p.kickClickTone, sr);
    drive = juce::Decibels::decibelsToGain (p.kickDrive);
    gain = juce::Decibels::decibelsToGain (p.kickLevelDb);
    phase = 0;
}

float KickVoice::render()
{
    if (! active())
        return 0;
    freq = juce::jmax (target, freq * sweepMul);
    phase += juce::MathConstants<double>::twoPi * freq / sr;
    if (phase > juce::MathConstants<double>::twoPi)
        phase -= juce::MathConstants<double>::twoPi;
    const auto input = static_cast<float> (std::sin (phase)) * env
                     + clickLP.low (noise (rng)) * clickEnv * clickAmount;
    env *= envMul;
    clickEnv *= clickMul;
    return saturate (input, drive) * gain;
}

void SnareVoice::prepare (double sampleRate)
{
    sr = sampleRate;
    toneEnv = noiseEnv = 0;
    noiseHP.reset();
    noiseLP.reset();
}

void SnareVoice::trigger (float velocity, float pitchMultiplier, const DrumParameters& p)
{
    f1 = juce::jlimit (50.0f, static_cast<float> (sr * .2), p.snareTune * pitchMultiplier);
    f2 = juce::jmin (f1 * 1.607f, static_cast<float> (sr * .4));
    toneEnv = juce::jmap (velocity, .18f, 1.0f);
    noiseEnv = toneEnv;
    toneMul = decayMul (p.snareDecay * .72f, sr);
    tone2Mul = decayMul (p.snareDecay * .53f, sr);
    noiseMul = decayMul (p.snareDecay, sr);
    snappy = p.snareSnappy;
    drive = juce::Decibels::decibelsToGain (p.snareDrive);
    gain = juce::Decibels::decibelsToGain (p.snareLevelDb);
    noiseHP.setLowPass (700, sr);
    noiseLP.setLowPass (p.snareTone, sr);
    p1 = p2 = 0;
}

float SnareVoice::render()
{
    if (! active())
        return 0;
    p1 += juce::MathConstants<double>::twoPi * f1 / sr;
    p2 += juce::MathConstants<double>::twoPi * f2 / sr;
    if (p1 > juce::MathConstants<double>::twoPi) p1 -= juce::MathConstants<double>::twoPi;
    if (p2 > juce::MathConstants<double>::twoPi) p2 -= juce::MathConstants<double>::twoPi;
    const auto tonal = (static_cast<float> (std::sin (p1)) * .62f
                      + static_cast<float> (std::sin (p2)) * .38f)
                     * toneEnv * (1.0f - snappy * .65f);
    const auto wires = noiseLP.low (noiseHP.high (noise (rng))) * noiseEnv * snappy;
    toneEnv *= toneMul;
    noiseEnv *= noiseMul;
    return saturate (tonal + wires, drive) * gain;
}

void HatEngine::prepare (double sampleRate)
{
    sr = sampleRate;
    midiPitchSmooth = 1.0f - static_cast<float> (std::exp (-1.0 / (.005 * sr)));
    reset();
    lowBand.setLowPass (3440, sr);
    highBand.setLowPass (7100, sr);
}

void HatEngine::reset()
{
    phases.fill (0);
    for (auto& envelope : envs) envelope = {};
    lowBand.reset(); highBand.reset(); hp.reset();
    midiPitchTarget = midiPitchCurrent = 1;
    maximumOscillatorFrequency = 0;
}

void HatEngine::trigger (float velocity, float pitchMultiplier, const DrumParameters& p)
{
    if (p.hatChoke)
        for (auto& envelope : envs)
            if (envelope.level > 0)
                envelope.chokeMul = decayMul (.004f, sr, .001f);
    auto& envelope = envs[static_cast<size_t> (cursor++ % static_cast<int> (envs.size()))];
    envelope.level = juce::jmap (velocity, .18f, 1.0f);
    envelope.mul = decayMul (p.hatDecay, sr);
    envelope.chokeMul = 1;
    midiPitchTarget = juce::jlimit (.125f, 8.0f, pitchMultiplier);
}

namespace
{
float polyBlep (float phase, float delta) noexcept
{
    if (phase < delta) { phase /= delta; return phase + phase - phase * phase - 1; }
    if (phase > 1 - delta) { phase = (phase - 1) / delta; return phase * phase + phase + phase + 1; }
    return 0;
}
}

float HatEngine::render (const DrumParameters& p)
{
    static constexpr float base[] { 205.3f, 304.4f, 369.6f, 522.7f, 540, 800 };
    midiPitchCurrent += (midiPitchTarget - midiPitchCurrent) * midiPitchSmooth;
    const auto ratio = midiPitchCurrent * pitchRatio (p.hatTune);
    const auto safeMaximum = static_cast<float> (sr * .45);
    float metal = 0;
    maximumOscillatorFrequency = 0;
    for (size_t i = 0; i < 6; ++i)
    {
        const auto frequency = juce::jmin (base[i] * ratio, safeMaximum);
        maximumOscillatorFrequency = juce::jmax (maximumOscillatorFrequency, frequency);
        const auto delta = frequency / static_cast<float> (sr);
        const auto phase = static_cast<float> (phases[i]);
        metal += ((phase < .5f ? 1.0f : -1.0f) + polyBlep (phase, delta)
               - polyBlep (std::fmod (phase + .5f, 1.0f), delta)) / 6.0f;
        phases[i] = std::fmod (phases[i] + delta, 1.0);
    }
    hp.setLowPass (p.hatHP, sr);
    const auto source = hp.high (juce::jmap (p.hatTone, lowBand.high (metal), highBand.high (metal)));
    float sum = 0;
    for (auto& envelope : envs)
        if (envelope.level > 1.0e-5f)
        {
            sum += source * envelope.level;
            envelope.level *= envelope.mul * envelope.chokeMul;
        }
    return sum * juce::Decibels::decibelsToGain (p.hatLevelDb);
}

void TomVoice::prepare (double sampleRate)
{
    sr = sampleRate;
    env = attackEnv = 0;
    attackLP.reset();
}

void TomVoice::trigger (float velocity, float pitchMultiplier, const DrumParameters& p)
{
    const auto primaryMaximum = static_cast<float> (sr * .27);
    target = juce::jlimit (35.0f, primaryMaximum, p.tomTune * pitchMultiplier);
    freq = juce::jlimit (target, primaryMaximum, target * pitchRatio (p.tomSweep));
    const auto bendSeconds = .018f + .035f * (1.0f - p.tomTone);
    sweepMul = static_cast<float> (std::pow (target / freq,
        1.0 / juce::jmax (1.0, static_cast<double> (bendSeconds) * sr)));
    env = juce::jmap (velocity, .2f, 1.0f);
    envMul = decayMul (p.tomDecay, sr);
    attackEnv = env;
    attackMul = decayMul (.0045f, sr);
    modeMix = .04f + .18f * p.tomTone;
    secondRatio = 1.42f + .13f * p.tomTone;
    attackAmount = p.tomAttack;
    attackLP.setLowPass (1200.0f + 8500.0f * p.tomTone, sr);
    drive = 2.0f + p.tomTone * 1.6f;
    gain = juce::Decibels::decibelsToGain (p.tomLevelDb);
    phase1 = phase2 = 0;
}

float TomVoice::render()
{
    if (! active())
        return 0;
    freq = juce::jmax (target, freq * sweepMul);
    const auto secondFrequency = juce::jmin (freq * secondRatio, static_cast<float> (sr * .44));
    phase1 += juce::MathConstants<double>::twoPi * freq / sr;
    phase2 += juce::MathConstants<double>::twoPi * secondFrequency / sr;
    if (phase1 > juce::MathConstants<double>::twoPi) phase1 -= juce::MathConstants<double>::twoPi;
    if (phase2 > juce::MathConstants<double>::twoPi) phase2 -= juce::MathConstants<double>::twoPi;
    const auto body = (static_cast<float> (std::sin (phase1))
                     + modeMix * static_cast<float> (std::sin (phase2))) * env;
    const auto stick = attackLP.low (noise (rng)) * attackEnv * attackAmount;
    env *= envMul;
    attackEnv *= attackMul;
    return saturate ((body + stick) * .82f, drive) * gain;
}

void DrumEngine::prepare (double sampleRate, int)
{
    sr = sampleRate;
    for (auto& voice : kicks) voice.prepare (sampleRate);
    for (auto& voice : snares) voice.prepare (sampleRate);
    for (auto& voice : toms) voice.prepare (sampleRate);
    hats.prepare (sampleRate);
    dcBlock.setLowPass (20, sampleRate);
    reset();
}

void DrumEngine::reset()
{
    for (auto& voice : kicks) voice.prepare (sr);
    for (auto& voice : snares) voice.prepare (sr);
    for (auto& voice : toms) voice.prepare (sr);
    hats.reset();
    lfoPhases.fill (0); lfoOutputs.fill (0); modulation.fill (0);
    lastTraceBins.fill (-1); lastShapes.fill (-1);
    for (auto& scheduler : repeatSchedulers) scheduler.clear();
    repeatTriggerCounts.fill (0);
    masterGain = 0;
    dcBlock.reset();
    for (size_t i = 0; i < lfoCount; ++i)
    {
        initialiseTrace (i, 0);
        lfoVisuals[i].phase.store (0);
        lfoVisuals[i].output.store (0);
    }
}

void DrumEngine::triggerSynth (Instrument instrument, float velocity, float pitchMultiplier,
                               const DrumParameters& p)
{
    velocity = juce::jlimit (0.0f, 1.0f, velocity);
    if (instrument == kick) kicks[kickCursor++ % kicks.size()].trigger (velocity, pitchMultiplier, p);
    else if (instrument == snare) snares[snareCursor++ % snares.size()].trigger (velocity, pitchMultiplier, p);
    else if (instrument == hat) hats.trigger (velocity, pitchMultiplier, p);
    else toms[tomCursor++ % toms.size()].trigger (velocity, pitchMultiplier, p);
}

void DrumEngine::scheduleRepeats (Instrument instrument, float velocity, float pitchMultiplier,
                                  const RepeatParameters& parameters) noexcept
{
    auto& scheduler = repeatSchedulers[static_cast<size_t> (instrument)];
    scheduler.clear();
    scheduler.count = juce::jlimit (0, 10, parameters.count);
    scheduler.velocity = juce::jlimit (0.0f, 1.0f, velocity);
    scheduler.pitchMultiplier = juce::jlimit (.03125f, 32.0f, pitchMultiplier);
    const auto time = juce::jlimit (.01f, .5f, parameters.timeSeconds);
    const auto shape = juce::jlimit (0.0f, 1.0f, parameters.shape);
    int64_t previous = 0;
    for (int repeat = 1; repeat <= scheduler.count; ++repeat)
    {
        const auto position = repeatCurvePosition (repeat, scheduler.count, shape);
        auto offset = static_cast<int64_t> (std::llround (sr * time * scheduler.count * position));
        offset = juce::jmax (previous + 1, offset);
        scheduler.offsets[static_cast<size_t> (repeat - 1)] = offset;
        scheduler.gains[static_cast<size_t> (repeat - 1)] = repeatCurveGain (repeat, scheduler.count, shape);
        previous = offset;
    }
}

void DrumEngine::trigger (Instrument instrument, float velocity, float pitchMultiplier,
                          const DrumParameters& p)
{
    triggerSynth (instrument, velocity, pitchMultiplier, applyModulation (p));
    scheduleRepeats (instrument, velocity, pitchMultiplier,
                     p.repeats[static_cast<size_t> (instrument)]);
}

void DrumEngine::processRepeats (const DrumParameters& p)
{
    for (size_t index = 0; index < repeatSchedulers.size(); ++index)
    {
        auto& scheduler = repeatSchedulers[index];
        while (scheduler.next < scheduler.count
               && scheduler.elapsed >= scheduler.offsets[static_cast<size_t> (scheduler.next)])
        {
            const auto gain = scheduler.gains[static_cast<size_t> (scheduler.next)];
            triggerSynth (static_cast<Instrument> (index), scheduler.velocity * gain,
                          scheduler.pitchMultiplier, applyModulation (p));
            ++scheduler.next;
            ++repeatTriggerCounts[index];
        }
        if (scheduler.next < scheduler.count)
            ++scheduler.elapsed;
    }
}

void DrumEngine::initialiseTrace (size_t index, int shape) noexcept
{
    for (size_t bin = 0; bin < lfoTraceSize; ++bin)
        lfoVisuals[index].trace[bin].store (
            evaluateLfoWaveform (shape, static_cast<float> (bin) / static_cast<float> (lfoTraceSize)),
            std::memory_order_relaxed);
}

void DrumEngine::advanceLfos (const DrumParameters& p) noexcept
{
    const auto previousOutputs = lfoOutputs;
    std::array<float, lfoCount> nextOutputs {};
    for (size_t i = 0; i < lfoCount; ++i)
    {
        const auto& parameters = p.lfos[i];
        if (parameters.shape != lastShapes[i])
        {
            if (lastShapes[i] < 0) initialiseTrace (i, parameters.shape);
            lastShapes[i] = parameters.shape;
            lastTraceBins[i] = -1;
        }
        const auto rate = std::isfinite (parameters.rate)
                        ? juce::jlimit (.05f, 20.0f, parameters.rate) : 1.0f;
        lfoPhases[i] += static_cast<double> (rate) / sr;
        if (lfoPhases[i] >= 1.0) lfoPhases[i] -= std::floor (lfoPhases[i]);
        float sourceOutput = 0;
        if (parameters.modSource >= 0 && parameters.modSource < static_cast<int> (lfoCount)
            && parameters.modSource != static_cast<int> (i))
            sourceOutput = previousOutputs[static_cast<size_t> (parameters.modSource)];
        const auto warp = std::isfinite (parameters.warp)
                        ? juce::jlimit (-1.0f, 1.0f, parameters.warp) : 0.0f;
        auto output = evaluateLfoWaveform (parameters.shape,
            static_cast<float> (lfoPhases[i]) + sourceOutput * warp * .25f);
        if (! std::isfinite (output)) output = 0;
        nextOutputs[i] = juce::jlimit (-1.0f, 1.0f, output);
        const auto bin = juce::jlimit (0, static_cast<int> (lfoTraceSize) - 1,
            static_cast<int> (lfoPhases[i] * static_cast<double> (lfoTraceSize)));
        if (bin != lastTraceBins[i])
        {
            lfoVisuals[i].trace[static_cast<size_t> (bin)].store (nextOutputs[i], std::memory_order_relaxed);
            lastTraceBins[i] = bin;
        }
        lfoVisuals[i].phase.store (static_cast<float> (lfoPhases[i]), std::memory_order_relaxed);
        lfoVisuals[i].output.store (nextOutputs[i], std::memory_order_relaxed);
    }
    lfoOutputs = nextOutputs;
    modulation.fill (0);
    for (size_t i = 0; i < lfoCount; ++i)
    {
        for (const auto& route : p.lfos[i].routes)
        {
            const auto destination = static_cast<size_t> (route.destination);
            if (destination > 0 && destination < modulation.size() && std::isfinite (route.depth))
                modulation[destination] += lfoOutputs[i] * juce::jlimit (-1.0f, 1.0f, route.depth);
        }
        modulation[static_cast<size_t> (Params::ModDestination::hatPitch)] +=
            lfoOutputs[i] * p.lfos[i].legacyHatPitchDepth / 24.0f;
        modulation[static_cast<size_t> (Params::ModDestination::hatDecay)] +=
            lfoOutputs[i] * p.lfos[i].legacyHatDecayDepth;
    }
    for (auto& value : modulation)
        value = std::isfinite (value) ? juce::jlimit (-1.0f, 1.0f, value) : 0;
}

float DrumEngine::getModulationForTests (Params::ModDestination destination) const noexcept
{
    const auto index = static_cast<size_t> (destination);
    return index < modulation.size() ? modulation[index] : 0;
}

DrumParameters DrumEngine::applyModulation (const DrumParameters& base) const noexcept
{
    auto result = base;
    const auto m = [this] (Params::ModDestination destination)
    {
        return modulation[static_cast<size_t> (destination)];
    };
    result.kickTune = juce::jlimit (15.0f, 800.0f, base.kickTune * pitchRatio (24 * m (Params::ModDestination::kickPitch)));
    result.kickSweep = juce::jlimit (0.0f, 60.0f, base.kickSweep + 30 * m (Params::ModDestination::kickSweep));
    result.kickDecay = juce::jlimit (.04f, 8.0f, base.kickDecay * timeRatio (m (Params::ModDestination::kickDecay)));
    result.kickClick = juce::jlimit (0.0f, 1.0f, base.kickClick + .75f * m (Params::ModDestination::kickClick));
    result.kickDrive = juce::jlimit (0.0f, 18.0f, base.kickDrive + 12 * m (Params::ModDestination::kickDrive));
    result.kickLevelDb = juce::jlimit (-60.0f, 6.0f, base.kickLevelDb + 18 * m (Params::ModDestination::kickLevel));

    result.snareTune = juce::jlimit (50.0f, 1200.0f, base.snareTune * pitchRatio (24 * m (Params::ModDestination::snarePitch)));
    result.snareDecay = juce::jlimit (.03f, 4.0f, base.snareDecay * timeRatio (m (Params::ModDestination::snareDecay)));
    result.snareSnappy = juce::jlimit (0.0f, 1.0f, base.snareSnappy + .75f * m (Params::ModDestination::snareSnappy));
    result.snareTone = juce::jlimit (300.0f, 16000.0f, base.snareTone * timeRatio (m (Params::ModDestination::snareTone)));
    result.snareDrive = juce::jlimit (0.0f, 18.0f, base.snareDrive + 12 * m (Params::ModDestination::snareDrive));
    result.snareLevelDb = juce::jlimit (-60.0f, 6.0f, base.snareLevelDb + 18 * m (Params::ModDestination::snareLevel));

    result.hatTune = juce::jlimit (-48.0f, 48.0f, base.hatTune + 24 * m (Params::ModDestination::hatPitch));
    result.hatDecay = juce::jlimit (.01f, 4.0f, base.hatDecay * timeRatio (m (Params::ModDestination::hatDecay)));
    result.hatTone = juce::jlimit (0.0f, 1.0f, base.hatTone + .75f * m (Params::ModDestination::hatTone));
    result.hatHP = juce::jlimit (1000.0f, 18000.0f, base.hatHP * std::exp2 (m (Params::ModDestination::hatCharacter)));
    result.hatLevelDb = juce::jlimit (-60.0f, 6.0f, base.hatLevelDb + 18 * m (Params::ModDestination::hatLevel));

    result.tomTune = juce::jlimit (35.0f, 1600.0f, base.tomTune * pitchRatio (24 * m (Params::ModDestination::tomPitch)));
    result.tomSweep = juce::jlimit (0.0f, 48.0f, base.tomSweep + 24 * m (Params::ModDestination::tomSweep));
    result.tomDecay = juce::jlimit (.03f, 4.0f, base.tomDecay * timeRatio (m (Params::ModDestination::tomDecay)));
    result.tomTone = juce::jlimit (0.0f, 1.0f, base.tomTone + .75f * m (Params::ModDestination::tomTone));
    result.tomAttack = juce::jlimit (0.0f, 1.0f, base.tomAttack + .75f * m (Params::ModDestination::tomAttack));
    result.tomLevelDb = juce::jlimit (-60.0f, 6.0f, base.tomLevelDb + 18 * m (Params::ModDestination::tomLevel));
    return result;
}

void DrumEngine::getLfoSnapshot (size_t index, LfoDisplaySnapshot& snapshot) const noexcept
{
    if (index >= lfoCount) return;
    snapshot.phase = lfoVisuals[index].phase.load (std::memory_order_relaxed);
    snapshot.output = lfoVisuals[index].output.load (std::memory_order_relaxed);
    for (size_t bin = 0; bin < lfoTraceSize; ++bin)
        snapshot.trace[bin] = lfoVisuals[index].trace[bin].load (std::memory_order_relaxed);
}

std::array<float, 8> DrumEngine::getKickFinalFrequenciesForTests() const
{
    std::array<float, 8> result {};
    for (size_t i = 0; i < kicks.size(); ++i) result[i] = kicks[i].getFinalFrequencyForTests();
    return result;
}

std::array<float, 8> DrumEngine::getTomFinalFrequenciesForTests() const
{
    std::array<float, 8> result {};
    for (size_t i = 0; i < toms.size(); ++i) result[i] = toms[i].getFinalFrequencyForTests();
    return result;
}

std::array<float, 8> DrumEngine::getSnareFinalFrequenciesForTests() const
{
    std::array<float, 8> result {};
    for (size_t i = 0; i < snares.size(); ++i) result[i] = snares[i].getTonalFrequenciesForTests().first;
    return result;
}

void DrumEngine::render (juce::AudioBuffer<float>& buffer, int start, int count,
                         const DrumParameters& p)
{
    const auto smooth = 1.0f - std::exp (-1.0f / (.01f * static_cast<float> (sr)));
    for (int sample = 0; sample < count; ++sample)
    {
        advanceLfos (p);
        processRepeats (p);
        const auto current = applyModulation (p);
        float output = 0;
        for (auto& voice : kicks) output += voice.render();
        for (auto& voice : snares) output += voice.render();
        for (auto& voice : toms) output += voice.render();
        output += hats.render (current);
        output = dcBlock.high (output);
        masterGain += (p.master - masterGain) * smooth;
        output = saturate (output, 1.25f) * masterGain;
        if (! std::isfinite (output)) output = 0;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.addSample (channel, start + sample, output);
    }
}
