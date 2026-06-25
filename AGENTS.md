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
| `config.h` | Local config (gitignored) — edit fan curves, `INTERVAL`, `STEPDOWN_MAX` |
| `macros.h` | `DBG()`, `DIE()`, `likely/unlikely`, `ATTR_INLINE` helpers |
| `nvspeed.c` | Main daemon — NVML init, fan control loop, CLI `--medium`/`--high` |
| `nvspeed-print.c` | Utility to print current fan curve from `/tmp/nvspeed/curve` |

## Oddities / gotchas

- `make clean` only removes `nvspeed` binary, **not** `nvspeed-print`.
- `.gitignore` ignores `nvspeed` (binary) and `config.h` (generated local config).
- Runtime requires `sudo` (NVML fan control). Single-instance enforced via `/tmp/nvspeed/nvspeed.lock`.
- NVML path hardcoded: lib at `/opt/cuda/lib64`, header at `/opt/cuda/include/nvml.h`.
- C89-ish style — no VLAs, no `//` comments, space-indented, no clang-format config.
- No tests, no CI, no linter/formatter, no typechecker — `CFLAGS` include `-fanalyzer -Wall -Wextra -Wpedantic` for static analysis at build time.

## Conventions

- Every change must be documented in `CHANGES.md` with date/time, rationale, and affected files.

## Commands

```sh
make clean && CPPFLAGS=-DDEBUG=1 make && sudo ./nvspeed --medium
```
