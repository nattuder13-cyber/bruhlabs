#include "PluginEditor.h"

KickEditor::KickEditor (KickProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      osc   ("OSCILLATOR", p.apvts, { { "shape", "Shape" }, { "shapeEnv", "Shape Env" } }, 2),
      vca   ("VCA ENV", p.apvts, { { "vcaDecay", "Decay" }, { "vcaCurve", "Curve" }, { "velSens", "Vel" } }, 3),
      pitch ("PITCH ENV (min 15 ms / 4 st)", p.apvts,
             { { "pDecay", "Decay" }, { "pCurve", "Curve" }, { "pDepth", "Depth" },
               { "endPitch", "End Hz" }, { "keyTrack", "Key Trk" } }, 3),
      f1env ("FILTER 1 ENV", p.apvts,
             { { "f1Decay", "Decay" }, { "f1Curve", "Curve" }, { "f1Amt", "Amount" }, { "f1Link", "Link VCA" } }, 2),
      lfo   ("LFO", p.apvts,
             { { "lfoDepth", "Depth" }, { "lfoWave", "Wave" }, { "lfoMode", "Reset" },
               { "lfoSync", "Sync" }, { "lfoDiv", "Division" }, { "lfoType", "Type" },
               { "lfoRate", "Rate" }, { "lfoPol", "Polarity" }, { "lfoBpm", "Fallbk BPM" },
               { "lfoPitch", "> Pitch" }, { "lfoShape", "> Shape" }, { "lfoF1", "> F1 Cut" },
               { "lfoF2", "> F2 Cut" }, { "lfoD1", "> Drive 1" }, { "lfoD2", "> Drive 2" },
               { "lfoVca", "> VCA" } }, 3),
      f1    ("FILTER 1", p.apvts, { { "f1Mode", "Mode" }, { "f1Cut", "Cutoff" }, { "f1Res", "Res" } }, 3),
      d1    ("DRIVE 1", p.apvts, { { "d1Type", "Type" }, { "d1Drive", "Drive" }, { "d1Bias", "Bias" }, { "d1Mix", "Mix" } }, 4),
      f2    ("FILTER 2", p.apvts, { { "f2Mode", "Mode" }, { "f2Cut", "Cutoff" }, { "f2Res", "Res" } }, 3),
      d2    ("DRIVE 2", p.apvts, { { "d2Type", "Type" }, { "d2Drive", "Drive" }, { "d2Bias", "Bias" }, { "d2Mix", "Mix" } }, 4),
      out   ("OUT", p.apvts, { { "outDb", "Level" } }, 1),
      scope (p)
{
    setLookAndFeel (&laf);

    for (juce::Component* c : { (juce::Component*) &osc, (juce::Component*) &vca, (juce::Component*) &pitch,
                                (juce::Component*) &f1env, (juce::Component*) &lfo, (juce::Component*) &f1,
                                (juce::Component*) &d1, (juce::Component*) &f2, (juce::Component*) &d2,
                                (juce::Component*) &out, (juce::Component*) &scope,
                                (juce::Component*) &trigBtn, (juce::Component*) &bpmLabel })
        addAndMakeVisible (c);

    trigBtn.onClick = [this] { proc.audition.store (true); };

    bpmLabel.setJustificationType (juce::Justification::centred);
    bpmLabel.setFont (juce::Font (juce::FontOptions (13.0f)));

    setResizable (true, true);
    setResizeLimits (1000, 640, 1900, 1100);
    setSize (1200, 760);
    startTimerHz (4);
    timerCallback();
}

void KickEditor::timerCallback()
{
    bpmLabel.setText ("Host BPM: " + juce::String (proc.hostBpm.load(), 1), juce::dontSendNotification);
}

void KickEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffc3c7cb));
    g.setColour (juce::Colour (0xff2b3036));
    g.fillRect (0, 0, getWidth(), 26);
    g.setColour (juce::Colour (0xffff8a3d));
    g.setFont (juce::Font (juce::FontOptions (15.0f)).boldened());
    g.drawText ("MORPHKICK", 12, 0, 300, 26, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffa4a9ae));
    g.setFont (11.f);
    g.drawText ("SINE > TRIANGLE KICK SYNTH", getWidth() - 312, 0, 300, 26, juce::Justification::centredRight);
}

void KickEditor::resized()
{
    auto r = getLocalBounds().reduced (8);
    r.removeFromTop (22);
    const int gap = 6;

    // bottom row: signal chain F1 > D1 > F2 > D2 > Out
    auto bottom = r.removeFromBottom (juce::jmax (110, r.getHeight() * 17 / 100));
    r.removeFromBottom (gap);
    {
        const int totalCols = 15;
        const int usable = bottom.getWidth() - 4 * gap;
        int x = bottom.getX();
        auto place = [&] (juce::Component& c, int cols, bool last = false)
        {
            const int w = usable * cols / totalCols;
            c.setBounds (x, bottom.getY(), w, bottom.getHeight());
            x += w + (last ? 0 : gap);
        };
        place (f1, 3); place (d1, 4); place (f2, 3); place (d2, 4); place (out, 1, true);
    }

    // left column: oscillator + envelopes
    auto left = r.removeFromLeft (230);
    r.removeFromLeft (gap);
    {
        const int units = 6;  // 1 + 1 + 2 + 2
        const int unit = (left.getHeight() - 3 * gap) / units;
        osc.setBounds   (left.removeFromTop (unit));       left.removeFromTop (gap);
        vca.setBounds   (left.removeFromTop (unit));       left.removeFromTop (gap);
        pitch.setBounds (left.removeFromTop (unit * 2));   left.removeFromTop (gap);
        f1env.setBounds (left);
    }

    // right column: LFO + trigger
    auto right = r.removeFromRight (260);
    r.removeFromRight (gap);
    {
        lfo.setBounds (right.removeFromTop (right.getHeight() * 78 / 100));
        right.removeFromTop (gap);
        bpmLabel.setBounds (right.removeFromTop (24));
        trigBtn.setBounds (right.reduced (20, 8));
    }

    // centre: oscilloscope
    scope.setBounds (r);
}
