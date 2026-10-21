# Contributing

Thanks for helping! The most valuable contributions right now are **hardware reports** from Predator/Nitro models
other than the PHN16-72 and bug reports with logs.

## Reporting hardware

Open a *Hardware report* issue and attach:

```bash
predatorctl info
cat /sys/class/dmi/id/product_name /sys/class/dmi/id/bios_version
journalctl -u predator-sensed -b --no-pager | tail -n 100
```

If your model has a PredatorSense INI file (`<Product Name>.ini` in the Windows package) with an `[OC_GPU]` section,
it can be added under `data/models/` in the same format.

## Development setup

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/predator-sensed --simulate &                 # no hardware, no root
PREDATOR_SENSE_BUS=session ./build/gui/predator-sense
make -C kernel                                       # kernel module
```

Debug messages are off by default. Run with `QT_LOGGING_RULES="predator.*.debug=true"` to see every WMI call; for the
installed service use `sudo systemctl edit predator-sensed` and add `Environment=QT_LOGGING_RULES=predator.*.debug=true`.

## Rules for firmware code

- Every value sent to the firmware must come from the PredatorSense binaries or the firmware itself (BMOF/ACPI).
  Document the source (function name/address) in the code and in `docs/REVERSE_ENGINEERING.md`.
- Never guess method IDs or encodings and never add "probing" of unknown methods. A new method needs its ID from the
  BMOF and an entry in the kernel allow-list.
- Keep the kernel module free of feature logic.

## Style

- C++20, Qt 6, 4-space indentation, `clang-format` (`.clang-format` in the repo).
- Kernel code follows the Linux kernel coding style (`scripts/checkpatch.pl --no-tree -f kernel/predator_wmi.c`).
- Commit messages: short imperative subject line, body explaining *why*.
