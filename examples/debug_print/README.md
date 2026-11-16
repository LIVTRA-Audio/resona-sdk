# Debug Print

A minimal custom algorithm that prints two startup messages to the box's USB
Serial Monitor and passes mono audio through at unity gain. The GAIN parameter
can attenuate it from 1 to 0. It uses the existing module ABI; no firmware rebuild
or Arduino library is needed.

1. Open this example through **New project → From an example** (or copy the
   folder to your workspace).
2. Build it: `resona-lab pack build "/path/to/Debug Print"`.
3. Open **Serial monitor**, enable **Serial output** and keep it connected.
4. In a terminal, make it a spare preset (here 50) and play it while the monitor stays open:

   ```sh
   resona-lab presets import --request '{"preset":50,"path":"/path/to/Debug Print/build/<id>.rpk","run":true}'
   ```

5. Look for:

   ```text
   Debug Print: Hello from my custom algorithm!
   Debug Print: audio ready (48 kHz, mono).
   ```

Keep the monitor connected before loading to capture complete startup lines;
bytes sent before a serial port is ready are not guaranteed to survive opening
or reconnecting it. The CLI import above keeps the Lab monitor open during
initialization. The import stores the pack in the box's user area and replaces preset 50;
choose a preset you do not need.

The key code is in `src/main.cpp`:

```cpp
if (host && host->log) {
    host->log("Hello from my custom algorithm!");
}
```

The host appends the newline, similar to `Serial.println`. Logging is optional,
so check the pointer. Keep each message within 128 UTF-8 bytes. The callback is
currently for **`init()` only**: do not retain it and print from `process`,
`set_param`, or encoder callbacks. Admission trials may initialize the algorithm
more than once, so repeated startup lines during an import are expected.

Turning output off suppresses messages; re-enable it and load the algorithm
again to see new startup messages. No messages are generated continuously while
audio runs. Desktop simulation sends these messages to the host console instead
of the physical USB Serial Monitor.

For CLI capture, close Lab's monitor first, then run
`resona-lab serial monitor --seconds 10 --output <capture.txt>`. Use
`resona-lab serial on` / `serial off` to control logging. These commands contact
the connected box; the monitor does not execute a firmware shell.
