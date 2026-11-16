# Twelve UI parameters

Encoder 1 selects P01–P12; encoder 2 changes its value. `get_param()` returns the
current UI-owned values so the host can save every parameter in a preset. The
host reads it after events and before Save, outside the audio callback. Keep it
read-only and bounded. This example passes audio through unchanged.

Values span0–255 so every integer has an exact representation in the device's
8-bit preset format. Other parameter ranges retain the same normalized precision.
Rebuild custom-UI algorithms with the new getter to report their own edits;
existing binaries without the optional getter retain host-set parameter behavior.
