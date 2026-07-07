# eSPI Analyzer

A low-level Intel eSPI protocol analyzer for Saleae Logic 2.

## Current support

| Area | Support |
| --- | --- |
| I/O modes | Single, Dual, and Quad. The initial mode is selectable, and accepted `SET_CONFIGURATION` mode changes are followed automatically. |
| Transaction framing | Command, turnaround, wait-state, and response phases for standard Peripheral, Virtual Wire, OOB, Flash, configuration, status, and short Peripheral commands. |
| Responses | `ACCEPT`, `DEFER`, `NON_FATAL_ERROR`, `FATAL_ERROR`, and `WAIT_STATE`. |
| Configuration | Detailed `GET_CONFIGURATION` and `SET_CONFIGURATION` output for Device Identification, General, Peripheral, Virtual Wire, OOB, and Flash registers. |
| Status | `GET_STATUS` queue state, pending-service commands, response modifiers, and appended Virtual Wire data. |
| Virtual Wire | `PUT_VWIRE` and `GET_VWIRE` groups. IRQ, standard system-event, platform-specific, and GPIO expander groups are named; other groups are shown as raw data. |
| Short I/O | `PUT_IORD_SHORT` and `PUT_IOWR_SHORT` for 1-, 2-, and 4-byte accesses, including address, data, response, and status. |
| Alert | Shared `ALERT#` detection on IO1 while CS# is inactive. |
| In-band RESET | Requires all four I/O lines high for the complete 16-clock sequence before returning to Single mode. |
| Output | Logic 2 bubbles and tables plus text/CSV export with timing, mode, byte counts, command/response previews, and decoded details. |

CLK, CS#, IO0, and IO1 are required inputs. IO2 and IO3 are optional for Single
and Dual I/O captures, but both are required to decode Quad I/O traffic and to
validate an in-band RESET. Use a capture rate of at least 80 MHz.

Enable `Ignore ALERT#` to suppress shared IO1 alert frames from bubbles, tables,
and text/CSV exports.

### Known limitations

- Peripheral, OOB, and Flash packet boundaries are detected, but their headers,
  addresses, tags, and payloads are not yet decoded into protocol fields.
- CRC bytes are included in transaction framing but are not validated. Traffic
  with CRC disabled is not currently supported.
- The initial I/O mode must match the link state at the beginning of the
  capture. It cannot be inferred reliably from arbitrary mid-session traffic.
- Only shared IO1 `ALERT#` signaling is supported; a dedicated alert input is
  not available. An alert already asserted at capture start is reported from
  the first observable idle sample.
- Simulation data generation is not implemented.

## Build

The build downloads the Saleae Analyzer SDK, so Git and network access are
required during initial configuration.

```sh
cmake -S . -B build
cmake --build build --config Release
```

The analyzer library is written to `build/Analyzers/` (or its
configuration-specific subdirectory on multi-config generators). Import that
library into Saleae Logic 2 to use the analyzer.
