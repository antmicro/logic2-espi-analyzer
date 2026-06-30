# eSPI Analyzer

A low-level Intel eSPI protocol analyzer for Saleae Logic 2.

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
