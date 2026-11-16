/*
 * Resona building blocks: the small DSP parts that Assembly mode wires together.
 *
 * Every block here is plain C++ with no heap, no exceptions and no C library, so it
 * builds the same for the desktop simulator and the device. The generated code of an
 * Assembly patch is nothing but calls to these blocks: reading a patch's source is
 * reading this file's API. You can also use them by hand in a code module.
 *
 * Conventions
 *   - Each block is a struct you place in your State. The host zeroes the State, so a
 *     block is usable after init(); call reset() to clear its signal history.
 *   - set_*() take user units (Hz, ms, %, dB) and may be called at any time. They do
 *     the arithmetic for coefficients, so keep them out of the per-sample path.
 *   - process() takes one sample and returns one sample. A block with a modulation
 *     input has process_cv(x, mod) as well: `mod` is a control signal in [-1, 1]
 *     (an LFO, for example) that replaces the block's own LFO.
 *   - Feedback paths are clamped and flushed, so no block ever returns NaN or
 *     infinity or lingers in denormals.
 *
 * Where the algorithms come from (all written afresh for this file; none is a copy)
 *   Biquad EQ     R. Bristow-Johnson, "Cookbook formulae for audio EQ biquad filter
 *                 coefficients" (W3C Note, adapted with permission).
 *   Room reverb   Freeverb by Jezar at Dreampoint (public domain): eight damped combs
 *                 and four all-passes; tunings rescaled from 44.1 kHz to 48 kHz.
 *   Filter        the topology-preserving state-variable filter (Zavalishin, "The Art of
 *                 VA Filter Design"; Cytomic's note on the same structure).
 *   Oscillator    PolyBLEP band-limiting (Valimaki and Huovilainen, 2007).
 *   Compressor    feed-forward design with a soft knee (Giannoulis, Massberg and Reiss,
 *                 "Digital Dynamic Range Compressor Design", JAES 2012).
 *   Chorus, flanger, phaser, delay, tremolo, fold, bit-crush and the envelope follower are
 *   the textbook structures (Zolzer, "DAFX"; Smith, "Physical Audio Signal Processing").
 *   Pink noise    Voss-McCartney (held values refreshed at octave-spaced rates).
 *   Reverb recipe Schroeder's parallel combs into series all-passes.
 *
 * GPL-3.0 licensed, see LICENSE.
 */
#ifndef RESONA_BLOCKS_H
#define RESONA_BLOCKS_H

#include "resona/blocks/common.h"
#include "resona/blocks/modline.h"
#include "resona/blocks/polyblep.h"
#include "resona/blocks/floorsmall.h"
#include "resona/blocks/lfo.h"
#include "resona/blocks/tremolo.h"
#include "resona/blocks/gain.h"
#include "resona/blocks/mixer.h"
#include "resona/blocks/drive.h"
#include "resona/blocks/filter.h"
#include "resona/blocks/chorus.h"
#include "resona/blocks/flanger.h"
#include "resona/blocks/phaser.h"
#include "resona/blocks/delay.h"
#include "resona/blocks/reverb.h"
#include "resona/blocks/osc.h"
#include "resona/blocks/env.h"
#include "resona/blocks/scale.h"
#include "resona/blocks/multiply.h"
#include "resona/blocks/compressor.h"
#include "resona/blocks/gate.h"
#include "resona/blocks/fold.h"
#include "resona/blocks/crush.h"
#include "resona/blocks/eq.h"
#include "resona/blocks/comb.h"
#include "resona/blocks/allpass.h"
#include "resona/blocks/room.h"
#include "resona/blocks/noise.h"
#include "resona/blocks/value.h"
#include "resona/blocks/slew.h"
#include "resona/blocks/samplehold.h"
#include "resona/blocks/delayline.h"
#include "resona/blocks/phase.h"
#include "resona/blocks/dyncurve.h"
#include "resona/blocks/clip.h"
#include "resona/blocks/rectify.h"
#include "resona/blocks/onepole.h"
#include "resona/blocks/polezero.h"
#include "resona/blocks/unitdelay.h"

#endif /* RESONA_BLOCKS_H */
