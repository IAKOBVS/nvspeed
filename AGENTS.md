# nvspeed — agent guide

## Build

```sh
make config      # copy config.def.h -> config.h (edit config.h first)
make             # builds nvspeed + nvspeed-print
sudo make install  # strips, copies to /usr/local/bin
```

## Debug / dev loop

```sh
# debug build with verbose DBG() stderr output:
CPPFLAGS=-DDEBUG=1 make

# rebuild + reinstall + restart running daemon:
./reload
```

## Key files

| Path | Role |
|------|------|
| `config.def.h` | Default config template — `make config` copies it to `config.h` |
| `config.h` | Local config (gitignored) — edit fan curves, `INTERVAL`, `STEPDOWN_MAX`, `PRINT_TEMP` |
| `macros.h` | `DBG()`, `DIE()`, `likely/unlikely`, `ATTR_INLINE` helpers |
| `nvspeed.c` | Main daemon — NVML init, fan control loop, CLI `--medium`/`--high`, temp file write |
| `nvspeed-print.c` | Utility to print current fan curve from `/tmp/nvspeed/curve` |

## Temperature file (`PRINT_TEMP`)

The daemon can write the current GPU temperature (max across all devices) to
`/tmp/nvspeed/temp` every `INTERVAL` seconds. This is controlled by the
compile-time switch `PRINT_TEMP` in `config.h` / `config.def.h` (enabled by
default). The temperature is written in **millidegrees Celsius** (e.g.
`45000\n` for 45 °C), mimicking the Linux hwmon sysfs interface so external
tools like `sensors` or custom scripts can read it without calling NVML
directly.

- **Disable:** set `#define PRINT_TEMP 0` (or comment it out) and rebuild.
- **Filename config:** `NVSPEED_FILE_TEMP` in `config.h` (default: `"temp"`).
- **No CLI flag** — this is a compile-time only switch.

## Oddities / gotchas

- `.gitignore` ignores `nvspeed`, `nvspeed-print`, `test` (binaries) and `config.h` (generated local config).
- Runtime requires `sudo` (NVML fan control). Single-instance enforced via `/tmp/nvspeed/nvspeed.lock`.
- NVML path hardcoded: lib at `/opt/cuda/lib64`, header at `/opt/cuda/include/nvml.h`.
- C89-ish style — no VLAs, no `//` comments, space-indented, no clang-format config.
- No CI, no linter/formatter, no typechecker — `CFLAGS` include `-fanalyzer -Wall -Wextra -Wpedantic` for static analysis at build time.
- Tests (`make test`) test `nv_step`, fan curve table integrity, and macros. No GPU required.

## Conventions

- Every change must be documented in `CHANGES.md` with date/time, rationale, and affected files.

## Commands

```sh
make clean && CPPFLAGS=-DDEBUG=1 make && sudo ./nvspeed --medium
```
