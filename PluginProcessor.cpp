#include "PluginProcessor.h"
#include "PluginEditor.h"

using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;

Layout KickProcessor::createLayout()
{
    Layout layout;

    auto addF = [&] (const char* id, const char* name, float lo, float hi, float def,
                     float centre, const char* unit, int decimals)
    {
        juce::NormalisableRange<float> r (lo, hi, 0.0f);
        if (centre > 0.f) r.setSkewForCentre (centre);
        const juce::String u (unit);
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, r, def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                [decimals, u] (float v, int) { return juce::String (v, decimals) + u; })));
    };
    auto addC = [&] (const char* id, const char* name, juce::StringArray items, int def)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def));
    };
    auto addB = [&] (const char* id, const char* name, bool def)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
    };

    // Oscillator
    addF ("shape",    "Shape",     0.f, 1.f, 0.2f, 0.f, "", 2);
    addF ("shapeEnv", "Shape Env", -1.f, 1.f, 0.f, 0.f, "", 2);

    // VCA envelope
    addF ("vcaDecay", "VCA Decay", 10.f, 4000.f, 450.f, 300.f, " ms", 0);
    addF ("vcaCurve", "VCA Curve", 0.f, 1.f, 0.5f, 0.f, "", 2);
    addF ("velSens",  "Vel Sens",  0.f, 1.f, 0.5f, 0.f, "", 2);

    // Pitch envelope. Ranges enforce the limits: decay >= 15 ms, depth >= 4 semitones.
    addF ("pDecay",   "Pitch Decay", mk::kMinPitchDecayMs, 1000.f, 60.f, 120.f, " ms", 0);
    addF ("pCurve",   "Pitch Curve", 0.f, 1.f, 0.5f, 0.f, "", 2);
    addF ("pDepth",   "Pitch Depth", mk::kMinPitchDepthSemis, mk::kMaxPitchDepthSemis, 23.f, 0.f, " st", 1);
    addF ("endPitch", "End Pitch",   20.f, 200.f, 48.f, 0.f, " Hz", 1);
    addB ("keyTrack", "Key Track",   false);

    // Filter 1 envelope
    addF ("f1Decay", "F1 Env Decay", 5.f, 2000.f, 200.f, 200.f, " ms", 0);
    addF ("f1Curve", "F1 Env Curve", 0.f, 1.f, 0.5f, 0.f, "", 2);
    addF ("f1Amt",   "F1 Env Amount", -6.f, 6.f, 0.f, 0.f, " oct", 2);
    addB ("f1Link",  "F1 Link VCA", false);

    // Filter 1 / Drive 1
    addC ("f1Mode", "F1 Mode", { "LP", "BP", "HP" }, 0);
    addF ("f1Cut",  "F1 Cutoff", 20.f, 20000.f, 20000.f, 1000.f, " Hz", 0);
    addF ("f1Res",  "F1 Res", 0.f, 0.95f, 0.f, 0.f, "", 2);
    addC ("d1Type", "D1 Type", { "Tanh", "Asym", "Fold" }, 0);
    addF ("d1Drive", "D1 Drive", 0.f, 1.f, 0.15f, 0.f, "", 2);
    addF ("d1Bias",  "D1 Bias", -1.f, 1.f, 0.f, 0.f, "", 2);
    addF ("d1Mix",   "D1 Mix", 0.f, 1.f, 1.f, 0.f, "", 2);

    // Filter 2 / Drive 2
    addC ("f2Mode", "F2 Mode", { "LP", "BP", "HP", "Notch" }, 0);
    addF ("f2Cut",  "F2 Cutoff", 20.f, 20000.f, 20000.f, 1000.f, " Hz", 0);
    addF ("f2Res",  "F2 Res", 0.f, 0.95f, 0.f, 0.f, "", 2);
    addC ("d2Type", "D2 Type", { "Tanh", "Asym", "Fold" }, 0);
    addF ("d2Drive", "D2 Drive", 0.f, 1.f, 0.15f, 0.f, "", 2);
    addF ("d2Bias",  "D2 Bias", -1.f, 1.f, 0.f, 0.f, "", 2);
    addF ("d2Mix",   "D2 Mix", 0.f, 1.f, 1.f, 0.f, "", 2);

    addF ("outDb", "Output", -24.f, 6.f, -3.f, 0.f, " dB", 1);

    // LFO
    addF ("lfoDepth", "LFO Depth", 0.f, 1.f, 0.f, 0.f, "", 2);
    addC ("lfoWave",  "LFO Wave", { "Sine", "Tri", "Saw Up", "Saw Dn", "Square", "S&H" }, 0);
    addC ("lfoMode",  "LFO Reset", { "Note Reset", "Transport", "Free Run" }, 0);
    addB ("lfoSync",  "LFO Sync", true);
    addC ("lfoDiv",   "LFO Division", { "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32" }, 6);
    addC ("lfoType",  "LFO Type", { "Straight", "Triplet", "Dotted" }, 0);
    addF ("lfoRate",  "LFO Rate", 0.05f, 20.f, 2.f, 2.f, " Hz", 2);
    addC ("lfoPol",   "LFO Polarity", { "Bipolar", "Unipolar" }, 0);
    addF ("lfoBpm",   "Fallback BPM", 40.f, 300.f, 120.f, 0.f, "", 0);
    addF ("lfoPitch", "LFO > Pitch", -12.f, 12.f, 0.f, 0.f, " st", 1);
    addF ("lfoShape", "LFO > Shape", -1.f, 1.f, 0.f, 0.f, "", 2);
    addF ("lfoF1",    "LFO > F1 Cut", -4.f, 4.f, 0.f, 0.f, " oct", 2);
    addF ("lfoF2",    "LFO > F2 Cut", -4.f, 4.f, 0.f, 0.f, " oct", 2);
    addF ("lfoD1",    "LFO > Drive1", -1.f, 1.f, 0.f, 0.f, "", 2);
    addF ("lfoD2",    "LFO > Drive2", -1.f, 1.f, 0.f, 0.f, "", 2);
    addF ("lfoVca",   "LFO > VCA", -1.f, 1.f, 0.f, 0.f, "", 2);

    return layout;
}

KickProcessor::KickProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

void KickProcessor::prepareToPlay (double sampleRate, int)
{
    voice.prepare (sampleRate);
}

bool KickProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

mk::Params KickProcessor::readParams()
{
    auto f = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto i = [&] (const char* id) { return (int) std::lround (f (id)); };

    mk::Params p;
    p.shape = f ("shape");  p.shapeEnv = f ("shapeEnv");
    p.vcaMs = f ("vcaDecay"); p.vcaCurve = f ("vcaCurve"); p.velSens = f ("velSens");
    p.pMs = f ("pDecay"); p.pCurve = f ("pCurve"); p.pDepth = f ("pDepth"); p.endHz = f ("endPitch");
    p.keyTrack = f ("keyTrack") > 0.5f;
    p.f1Ms = f ("f1Decay"); p.f1Curve = f ("f1Curve"); p.f1Amt = f ("f1Amt"); p.f1Link = f ("f1Link") > 0.5f;
    p.f1Mode = i ("f1Mode"); p.f1Cut = f ("f1Cut"); p.f1Res = f ("f1Res");
    p.d1Type = i ("d1Type"); p.d1Drive = f ("d1Drive"); p.d1Bias = f ("d1Bias"); p.d1Mix = f ("d1Mix");
    p.f2Mode = i ("f2Mode"); p.f2Cut = f ("f2Cut"); p.f2Res = f ("f2Res");
    p.d2Type = i ("d2Type"); p.d2Drive = f ("d2Drive"); p.d2Bias = f ("d2Bias"); p.d2Mix = f ("d2Mix");
    p.outGain = juce::Decibels::decibelsToGain (f ("outDb"));
    p.lfoDepth = f ("lfoDepth"); p.lfoWave = i ("lfoWave"); p.lfoMode = i ("lfoMode");
    p.lfoUni = i ("lfoPol") == 1;
    p.lfoPitch = f ("lfoPitch"); p.lfoShape = f ("lfoShape"); p.lfoF1 = f ("lfoF1"); p.lfoF2 = f ("lfoF2");
    p.lfoD1 = f ("lfoD1"); p.lfoD2 = f ("lfoD2"); p.lfoVca = f ("lfoVca");
    return p;
}

void KickProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    mk::Params p = readParams();

    // ---- host tempo / transport ----
    double bpm = apvts.getRawParameterValue ("lfoBpm")->load();
    double ppq = 0.0;
    bool playing = false, havePpq = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) { ppq = *q; havePpq = true; }
            playing = pos->getIsPlaying();
        }
    hostBpm.store ((float) bpm);

    // ---- LFO rate (beats per cycle) ----
    static const double divBeats[9] = { 32, 16, 8, 4, 2, 1, 0.5, 0.25, 0.125 };
    const int divIdx  = (int) std::lround (apvts.getRawParameterValue ("lfoDiv")->load());
    const int typeIdx = (int) std::lround (apvts.getRawParameterValue ("lfoType")->load());
    double beats = divBeats[juce::jlimit (0, 8, divIdx)];
    if (typeIdx == 1) beats *= 2.0 / 3.0;
    if (typeIdx == 2) beats *= 1.5;

    const bool sync = apvts.getRawParameterValue ("lfoSync")->load() > 0.5f;
    p.lfoHz = sync ? (float) (bpm / 60.0 / beats) : apvts.getRawParameterValue ("lfoRate")->load();
    p.lfoHz = juce::jlimit (0.01f, 50.f, p.lfoHz);

    voice.setParams (p);
    voice.setTap (scope.tap.load());

    // Transport lock: derive LFO phase from the host song position each block.
    if (sync && p.lfoMode == 1 && playing && havePpq)
        voice.setLfoPhase ((float) std::fmod (ppq / beats, 1.0));

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    auto render = [&] (int start, int end)
    {
        for (int n = start; n < end; ++n)
        {
            float tap = 0.f;
            const float y = voice.process (tap);
            L[n] = y;
            if (R != nullptr) R[n] = y;
            scope.push (tap);
        }
    };

    auto trigger = [&] (int note, float vel) { voice.noteOn (note, vel); scope.markNoteOn(); };

    if (audition.exchange (false))
        trigger (36, 1.0f);

    // ---- sample-accurate MIDI ----
    int pos = 0;
    for (const auto metadata : midi)
    {
        const int evPos = juce::jlimit (pos, numSamples, metadata.samplePosition);
        render (pos, evPos);
        pos = evPos;

        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            trigger (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isPitchWheel())
            voice.setBend ((float) (msg.getPitchWheelValue() - 8192) / 8192.f * 2.f);
        else if (msg.isController() && msg.getControllerNumber() == 1)
            voice.setModWheel ((float) msg.getControllerValue() / 127.f);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            voice.kill();
        // Note-off is intentionally ignored: the kick is one-shot.
    }
    render (pos, numSamples);
}

juce::AudioProcessorEditor* KickProcessor::createEditor() { return new KickEditor (*this); }

void KickProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void KickProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KickProcessor(); }
