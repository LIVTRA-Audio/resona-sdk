// dsp.cpp: Tiny Synth, a mono subtractive synth built from blocks: Oscillator, Filter, and a
// Slew that turns the GATE into an attack/release envelope for a Multiply.
//
// Packs receive parameters, not notes, so the instrument is played through NOTE (a MIDI note
// number) and GATE (on/off): a box app turns key presses, MIDI notes or its own sequencer
// into those two values.
#include "dsp.h"

#include <resona/math.h>

// NOTE is a MIDI note number; the oscillator takes Hz (440 Hz at note 69).
static void set_note(Dsp &d, float note)
{
    d.osc1.set_freq(440.0f * resona_exp2f((note - 69.0f) / 12.0f));
}

void dsp_init(Dsp &d)
{
    d.osc1.init();
    d.osc1.set_freq(220.0f);   // pitch
    d.osc1.set_shape(0.0f);   // shape
    d.osc1.set_level(-6.0f);   // level
    d.filter1.init();
    d.filter1.set_cutoff(1000.0f);   // cutoff
    d.filter1.set_resonance(20.0f);   // resonance
    d.filter1.set_mode(0.0f);   // mode
    d.slew1.init();
    d.slew1.set_rise(10.0f);   // rise
    d.slew1.set_fall(100.0f);   // fall
    d.multiply1.init();
    d.gain1.init();
    d.gain1.set_level(0.0f);   // level
}

void dsp_reset(Dsp &d)
{
    d.osc1.reset();
    d.filter1.reset();
    d.slew1.reset();
    d.multiply1.reset();
    d.gain1.reset();
}

void dsp_param(Dsp &d, uint32_t index, float value)
{
    switch (index) {
    case P_WAVE:
        d.osc1.set_shape(value);
        break;
    case P_CUTOFF:
        d.filter1.set_cutoff(value);
        break;
    case P_RES:
        d.filter1.set_resonance(value);
        break;
    case P_ATTACK:
        d.slew1.set_rise(value);
        break;
    case P_RELEASE:
        d.slew1.set_fall(value);
        break;
    case P_LEVEL:
        d.gain1.set_level(value);
        break;
    case P_NOTE:
        set_note(d, value);
        break;
    case P_GATE:
        d.gate = value > 0.5f ? 1.0f : 0.0f;
        break;
    default:
        break;
    }
}

void dsp_process(Dsp &d, const float *in, float *out, uint32_t frames)
{
    for (uint32_t i = 0; i < frames; ++i) {
        (void)in;  // an instrument: it makes the sound
        const float osc1_out = d.osc1.process();  // Oscillator
        const float filter1_out = d.filter1.process(osc1_out);  // Filter
        const float slew1_out = d.slew1.process(d.gate);  // Slew: the gate, softened into an attack/release envelope
        const float multiply1_out = d.multiply1.process(filter1_out, slew1_out);  // Multiply: the envelope sets the volume
        const float gain1_out = d.gain1.process(multiply1_out);  // Gain
        out[i] = gain1_out;  // Output
    }
}
