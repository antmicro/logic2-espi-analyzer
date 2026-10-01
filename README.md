# eSPI protocol decoder for Saleae Logic 2 analyzer

Copyright (c) 2026 [Antmicro](https://www.antmicro.com)

This repository contains a low-level protocol decoder for the Intel Enhanced Serial Peripheral Interface (eSPI), built with the [Saleae Analyzer SDK](https://github.com/saleae/AnalyzerSDK).
It decodes eSPI commands and responses captured with [Logic 2](https://www.saleae.com/).
The decoded eSPI payload includes configuration registers, statuses, Virtual Wires and short I/O accesses.

## Features

* Single, Dual and Quad I/O decoding, with automatic mode changes after accepted `SET_CONFIGURATION` commands.
* Command, turnaround, wait-state and response framing for the Peripheral, Virtual Wire, Out-of-Band (OOB), Flash, configuration, status and short Peripheral commands.
* Response identification: `ACCEPT`, `DEFER`, `NON_FATAL_ERROR`, `FATAL_ERROR` and `WAIT_STATE`.
* Configuration register decoding for Device Identification, General, Peripheral, Virtual Wire, OOB and Flash registers.
* `GET_STATUS` queue flags, pending-service commands, response modifiers and appended Virtual Wire data.
* Virtual Wire IRQ and system-event decoding at indices 0–7.
* `PUT_IORD_SHORT` and `PUT_IOWR_SHORT` decoding for 1-, 2- and 4-byte accesses, including address, data and response status.
* Shared IO1 or dedicated `ALERT#` monitoring, optional external `RESET#` monitoring and in-band reset detection.
* Configurable CS# glitch filtering and display filters for alerts and short I/O reads.

## Dependencies

* A C++11 compiler and a build tool supported by CMake, such as Make or Ninja.
* CMake 3.13 or newer.

## Building

From the repository root, run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build --config Release
```

The analyzer library is written to `build/Analyzers/`.

To include debug information in bubbles, table entries and terminal output, add `-DESPI_DEBUG_TRANSACTION_DETAILS=ON` to the CMake configuration command.

## Usage

### Loading the analyzer

1. Open Logic 2 settings and set the **Custom Low Level Analyzers** directory to the directory containing the compiled library.
2. Restart Logic 2 and open or record an eSPI capture.
3. In the **Analyzers** panel, click **+**, select **Intel eSPI** and assign the captured channels as described below.
4. Set **Initial I/O mode** to the mode used at the start of the capture.

See Saleae's [custom analyzer installation instructions](https://www.saleae.com/support/extensions-api/protocol-analyzer-sdk/setting-up-developer-directory) for platform-specific details.

### Channel configuration

| Input | Purpose |
| --- | --- |
| Clock | Required eSPI clock. |
| CS# | Required active-low chip select. |
| IO0, IO1 | Required data lines. IO1 is also used for shared alerts when no dedicated alert channel is selected. |
| IO2, IO3 | Required for Quad I/O decoding and in-band reset validation; optional for Single and Dual I/O. |
| RESET# | Optional active-low reset input. Resets the decoder to Single I/O mode. |
| ALERT# (optional) | Dedicated active-low alert input, used instead of IO1. Monitored regardless of CS# state. |

The minimum capture rate is 80 MHz (four samples per clock at 20 MHz).
Faster eSPI clocks require a higher capture rate.

### Analyzer settings

| Setting | Behavior |
| --- | --- |
| Initial I/O mode | Single (default), Dual or Quad. I has to match the eSPI transmission mode being used at the start of the capture. |
| CS# glitch filter (ns) | Ignore high and low CS# pulses shorter than the specified duration. Defaults to `0` (disabled). |
| Ignore ALERT# | Suppress alert frames in bubbles, tables, terminal output and exports. Disabled by default. |
| Ignore PUT_IORD_SHORT | Hide short I/O reads from bubbles, tables, terminal output and exports. Decoding continues to keep track of transaction boundaries. Disabled by default. |

An in-band reset returns the decoder to Single mode after all four I/O lines have been high for 16 clocks.
Shared IO1 alerts are ignored until the first transaction, both at capture start and after a reset.

## Limitations

* Peripheral, OOB and Flash packet parsing determines command lengths only. Headers, addresses, tags and payloads are not decoded in the output.
* Short memory commands show the command name, but no decoded address or data fields.
* CRC bytes are counted but not checked. Traffic with CRC disabled is not supported.
* The initial I/O mode must be set manually, including for captures that start mid-transaction.
* Virtual Wire index 8 and other unnamed system-event groups are shown as raw data. Platform-specific and GPIO expander ranges have category labels, but no signal names.
* Simulation data generation is not implemented.

## References

* [Intel eSPI Interface Base Specification, revision 1.6](https://cdrdv2-public.intel.com/841685/841685_ESPI_IBS_TS_Rev_1_6.pdf)
* [Saleae Analyzer SDK](https://github.com/saleae/AnalyzerSDK)

## License

This project is licensed under the [Apache License, Version 2.0](LICENSE).
