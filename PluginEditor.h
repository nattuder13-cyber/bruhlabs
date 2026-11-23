#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"


// ---------------------------------------------------------------------------
// Hardware-groove look: silver body, dark knobs with white pointers,
// orange accents, amber LCD. (Inspired by late-90s/2000s groove-box panels.)
// ---------------------------------------------------------------------------
class EsxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EsxLookAndFeel()
    {
        const juce::Colour dark (0xff1b1e22), amber (0xffffb347), lcd (0xff17110a), orange (0xffe8652c);
        setColour (juce::Label::textColourId, dark);
        setColour (juce::Slider::textBoxTextColourId, dark);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::thumbColourId, dark);
        setColour (juce::Slider::trackColourId, orange);
        setColour (juce::Slider::backgroundColourId, juce::Colour (0xff7d838a));
        setColour (juce::ComboBox::backgroundColourId, lcd);
        setColour (juce::ComboBox::textColourId, amber);
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff555a60));
        setColour (juce::ComboBox::arrowColourId, amber);
        setColour (juce::PopupMenu::backgroundColourId, lcd);
        setColour (juce::PopupMenu::textColourId, amber);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, orange);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b3036));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xffff8a3d));
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xffff8a3d));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float a0, float a1, juce::Slider&) override
    {
        const float size = (float) juce::jmin (w, h) - 6.f;
        if (size < 8.f) return;
        const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
        const float R = size * 0.5f, r = R * 0.74f;

        juce::Path track, val;
        track.addCentredArc (cx, cy, R - 1.5f, R - 1.5f, 0.f, a0, a1, true);
        val.addCentredArc (cx, cy, R - 1.5f, R - 1.5f, 0.f, a0, a0 + (a1 - a0) * pos, true);
        g.setColour (juce::Colour (0x55000000));
        g.strokePath (track, juce::PathStrokeType (2.5f));
        g.setColour (juce::Colour (0xffe8652c));
        g.strokePath (val, juce::PathStrokeType (2.5f));

        g.setColour (juce::Colour (0xff23272b));
        g.fillEllipse (cx - r, cy - r, r * 2.f, r * 2.f);
        g.setColour (juce::Colour (0xff6b7076));
        g.drawEllipse (cx - r, cy - r, r * 2.f, r * 2.f, 1.5f);

        const float ang = a0 + pos * (a1 - a0);
        g.setColour (juce::Colours::white);
        g.drawLine (cx + std::sin (ang) * r * 0.3f, cy - std::cos (ang) * r * 0.3f,
                    cx + std::sin (ang) * r * 0.92f, cy - std::cos (ang) * r * 0.92f, 2.5f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        const bool on = b.getToggleState();
        g.setColour (on ? juce::Colour (0xffe8452c) : juce::Colour (0xff3a3f45));
        g.fillRoundedRectangle (r, 3.f);
        g.setColour (juce::Colour (0xff111417));
        g.drawRoundedRectangle (r, 3.f, 1.f);
        g.setColour (on ? juce::Colours::white : juce::Colour (0xffa4a9ae));
        g.setFont (11.f);
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
    }
};

// ---------------------------------------------------------------------------
// One labelled control, auto-built from the parameter type (knob/combo/toggle)
// ---------------------------------------------------------------------------
class ParamControl : public juce::Component
{
public:
    ParamControl (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (juce::FontOptions (11.0f)));
        addAndMakeVisible (label);

        auto* param = apvts.getParameter (id);
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            combo.addItemList (c->choices, 1);
            addAndMakeVisible (combo);
            comboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, id, combo);
        }
        else if (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
        {
            toggle.setButtonText ("On");
            addAndMakeVisible (toggle);
            buttonAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, id, toggle);
        }
        else
        {
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            addAndMakeVisible (slider);
            sliderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, id, slider);
        }
    }

    void resized() override
    {
        auto a = getLocalBounds();
        label.setBounds (a.removeFromTop (14));
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, juce::jmax (30, getWidth() - 4), 14);
        slider.setBounds (a);
        combo.setBounds (a.withSizeKeepingCentre (juce::jmax (20, a.getWidth() - 4), 22));
        toggle.setBounds (a.withSizeKeepingCentre (juce::jmin (a.getWidth(), 60), 24));
    }

private:
    juce::Label label;
    juce::Slider slider;
    juce::ComboBox combo;
    juce::ToggleButton toggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAtt;
};

// ---------------------------------------------------------------------------
// Titled panel that lays its controls out on a grid
// ---------------------------------------------------------------------------
class Section : public juce::Component
{
public:
    Section (juce::String t, juce::AudioProcessorValueTreeState& apvts,
             std::vector<std::pair<juce::String, juce::String>> items, int numCols)
        : title (std::move (t)), cols (numCols)
    {
        for (auto& it : items)
        {
            controls.push_back (std::make_unique<ParamControl> (apvts, it.first, it.second));
            addAndMakeVisible (*controls.back());
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xffa4a9ae));
        g.fillRoundedRectangle (b, 4.f);
        g.setColour (juce::Colour (0xff2b3036));
        g.fillRoundedRectangle (b.removeFromTop (19.f), 4.f);
        g.setColour (juce::Colour (0xffff8a3d));
        g.setFont (juce::Font (juce::FontOptions (11.5f)).boldened());
        g.drawText (title, 8, 2, getWidth() - 16, 16, juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto a = getLocalBounds().withTrimmedTop (20).reduced (4);
        const int n = (int) controls.size();
        const int rows = (n + cols - 1) / cols;
        const int cw = a.getWidth() / cols, ch = a.getHeight() / rows;
        for (int i = 0; i < n; ++i)
            controls[(size_t) i]->setBounds (a.getX() + (i % cols) * cw, a.getY() + (i / cols) * ch, cw, ch);
    }

private:
    juce::String title;
    int cols;
    std::vector<std::unique_ptr<ParamControl>> controls;
};

// ---------------------------------------------------------------------------
// Oscilloscope: reads the lock-free tap buffer on a 60 Hz timer.
// ---------------------------------------------------------------------------
class Scope : public juce::Component, private juce::Timer
{
public:
    explicit Scope (KickProcessor& p) : proc (p)
    {
        tapBox.addItemList ({ "Oscillator", "Post Filter 1", "Post Drive 1", "Post Filter 2", "Post Drive 2", "Output" }, 1);
        tapBox.setSelectedId (6, juce::dontSendNotification);
        tapBox.onChange = [this] { proc.scope.tap.store (tapBox.getSelectedId() - 1); };

        trigBox.addItemList ({ "Note Trigger", "Zero-Cross", "Free Run" }, 1);
        trigBox.setSelectedId (1, juce::dontSendNotification);

        timeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        timeSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 62, 18);
        timeSlider.setRange (5.0, 2000.0, 1.0);
        timeSlider.setSkewFactorFromMidPoint (150.0);
        timeSlider.setValue (400.0);
        timeSlider.setTextValueSuffix (" ms");

        gainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        gainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 46, 18);
        gainSlider.setRange (0.25, 8.0, 0.01);
        gainSlider.setSkewFactorFromMidPoint (1.0);
        gainSlider.setValue (1.0);
        gainSlider.setTextValueSuffix ("x");

        holdBtn.setButtonText ("Hold");
        markBtn.setButtonText ("15ms");
        markBtn.setToggleState (true, juce::dontSendNotification);

        for (juce::Component* c : { (juce::Component*) &tapBox, (juce::Component*) &trigBox,
                                    (juce::Component*) &timeSlider, (juce::Component*) &gainSlider,
                                    (juce::Component*) &holdBtn, (juce::Component*) &markBtn })
            addAndMakeVisible (c);

        snap.reserve ((size_t) mk::ScopeBuffer::size / 2);
        startTimerHz (60);
    }

    ~Scope() override { stopTimer(); }

    void resized() override
    {
        auto c = getLocalBounds().removeFromBottom (30).reduced (2);
        tapBox.setBounds (c.removeFromLeft (115));   c.removeFromLeft (4);
        trigBox.setBounds (c.removeFromLeft (105));  c.removeFromLeft (4);
        holdBtn.setBounds (c.removeFromRight (58));
        markBtn.setBounds (c.removeFromRight (58));
        gainSlider.setBounds (c.removeFromRight (c.getWidth() / 3));
        timeSlider.setBounds (c);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().withTrimmedBottom (32).toFloat().reduced (2.f);
        g.setColour (juce::Colour (0xff17110a));
        g.fillRoundedRectangle (r, 6.f);

        const double timeMs = timeSlider.getValue();

        // graticule
        g.setColour (juce::Colour (0x26ffa13a));
        for (int i = 1; i < 10; ++i)
            g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) i / 10.f), r.getY(), r.getBottom());
        for (int i = 1; i < 8; ++i)
            g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * (float) i / 8.f), r.getX(), r.getRight());
        g.setColour (juce::Colour (0x66ffa13a));
        g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());

        g.setColour (juce::Colour (0xaaffa13a));
        g.setFont (10.f);
        for (int i = 1; i < 10; i += 2)
            g.drawText (juce::String (timeMs * i / 10.0, timeMs < 50.0 ? 1 : 0) + " ms",
                        (int) (r.getX() + r.getWidth() * (float) i / 10.f) + 2, (int) r.getBottom() - 14, 60, 12,
                        juce::Justification::left);

        // 15 ms guaranteed-pitch-transient marker
        if (trigBox.getSelectedId() == 1 && markBtn.getToggleState())
        {
            const float w = juce::jmin (r.getWidth(), r.getWidth() * (float) (15.0 / timeMs));
            g.setColour (juce::Colour (0x30ff3b2f));
            g.fillRect (r.getX(), r.getY(), w, r.getHeight());
            g.setColour (juce::Colour (0xccff6a5a));
            g.setFont (10.f);
            g.drawText ("15 ms", (int) r.getX() + 3, (int) r.getY() + 2, 50, 12, juce::Justification::left);
        }

        if (! holdBtn.getToggleState()) capture (timeMs);

        const int count = (int) snap.size();
        if (count < 2 || windowN < 2) return;

        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (r.toNearestInt());

        const float midY = r.getCentreY();
        const float amp  = r.getHeight() * 0.5f * 0.95f * (float) gainSlider.getValue();
        const int   W    = juce::jmax (2, (int) r.getWidth());
        const float spc  = (float) windowN / (float) W;   // samples per pixel

        juce::Path trace;
        float peak = 0.f;
        for (float s : snap) peak = juce::jmax (peak, std::fabs (s));

        if (spc >= 2.f)
        {
            // min/max decimation per pixel column
            for (int x = 0; x < W; ++x)
            {
                const int i0 = (int) ((int64_t) x * windowN / W);
                int i1 = (int) ((int64_t) (x + 1) * windowN / W);
                if (i0 >= count) break;
                i1 = juce::jmin (juce::jmax (i1, i0 + 1), count);
                float mn = snap[(size_t) i0], mx = mn;
                for (int i = i0 + 1; i < i1; ++i) { mn = juce::jmin (mn, snap[(size_t) i]); mx = juce::jmax (mx, snap[(size_t) i]); }
                const float px = r.getX() + (float) x + 0.5f;
                trace.startNewSubPath (px, midY - mx * amp);
                trace.lineTo (px, midY - mn * amp + 0.5f);
            }
        }
        else
        {
            // zoomed in: interpolated polyline
            bool first = true;
            for (int x = 0; x < W; ++x)
            {
                const float pos = (float) x * (float) (windowN - 1) / (float) (W - 1);
                const int i0 = (int) pos;
                if (i0 >= count - 1) break;
                const float fr = pos - (float) i0;
                const float v = snap[(size_t) i0] * (1.f - fr) + snap[(size_t) i0 + 1] * fr;
                const float px = r.getX() + (float) x, py = midY - v * amp;
                if (first) { trace.startNewSubPath (px, py); first = false; } else trace.lineTo (px, py);
            }
        }

        g.setColour (juce::Colour (0x40ff8a1f));
        g.strokePath (trace, juce::PathStrokeType (4.f));
        g.setColour (juce::Colour (0xffffc15a));
        g.strokePath (trace, juce::PathStrokeType (1.3f));

        g.setColour (juce::Colour (0xffffc15a));
        g.setFont (11.f);
        g.drawText ("peak " + juce::String (juce::Decibels::gainToDecibels (peak, -100.f), 1) + " dB",
                    (int) r.getRight() - 110, (int) r.getY() + 3, 104, 14, juce::Justification::right);
    }

private:
    void timerCallback() override { repaint(); }

    // Copy the window of samples to display into 'snap' (UI thread only).
    void capture (double timeMs)
    {
        const double sr = proc.getSampleRate();
        if (sr <= 0.0) return;

        auto& sb = proc.scope;
        const int64_t size = mk::ScopeBuffer::size;
        const int64_t N  = juce::jlimit<int64_t> (64, size / 2, (int64_t) (timeMs * 0.001 * sr));
        const int64_t wp = (int64_t) sb.writePos.load (std::memory_order_acquire);
        auto at = [&] (int64_t i) { return sb.data[(size_t) ((uint64_t) i & mk::ScopeBuffer::mask)].load (std::memory_order_relaxed); };

        int64_t start = wp - N;
        const int mode = trigBox.getSelectedId();
        if (mode == 1)
        {
            const int64_t n = (int64_t) sb.noteOnPos.load (std::memory_order_acquire);
            if (n <= wp && wp - n < size - 2048) start = n;
        }
        else if (mode == 2)
        {
            const int64_t from = juce::jmax<int64_t> (1, wp - 2 * N);
            for (int64_t i = from; i < wp - N; ++i)
                if (at (i - 1) < 0.f && at (i) >= 0.f) { start = i; break; }
        }
        start = juce::jmax<int64_t> (0, start);

        const int64_t count = juce::jmin (N, wp - start);
        windowN = (int) N;
        snap.resize ((size_t) juce::jmax<int64_t> (0, count));
        for (int64_t i = 0; i < count; ++i) snap[(size_t) i] = at (start + i);
    }

    KickProcessor& proc;
    juce::ComboBox tapBox, trigBox;
    juce::Slider timeSlider, gainSlider;
    juce::ToggleButton holdBtn, markBtn;
    std::vector<float> snap;
    int windowN = 0;
};

// ---------------------------------------------------------------------------
class KickEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit KickEditor (KickProcessor&);
    ~KickEditor() override { stopTimer(); setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    EsxLookAndFeel laf;
    KickProcessor& proc;
    Section osc, vca, pitch, f1env, lfo, f1, d1, f2, d2, out;
    Scope scope;
    juce::TextButton trigBtn { "TRIGGER" };
    juce::Label bpmLabel;
};
