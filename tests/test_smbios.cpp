// Parses the Acer OEM SMBIOS tables of a real Predator PHN16-72 (BIOS V1.20).
#include "../daemon/SmbiosOem.h"
#include <cstdio>

int main(int argc, char **argv)
{
    if (argc < 2)
        return 2;
    const SmbiosOem s = SmbiosOem::read(QString::fromLocal8Bit(argv[1]));
    int fails = 0;
    auto expect = [&](const char *what, int got, int want) {
        if (got != want) {
            std::fprintf(stderr, "FAIL %s: got %d, want %d\n", what, got, want);
            ++fails;
        }
    };
    expect("found172", s.found172, 1);
    expect("keyboardArea", s.keyboardArea, 4);
    expect("keyboardColor", s.keyboardColor, 2);
    expect("keyboardInterface", s.keyboardInterface, 1);
    expect("tableType", s.tableType, 2);
    expect("tableVersion", s.tableVersion, 0x5B);
    expect("bkHotkey", s.bkHotkey, 0x84);
    if (!fails)
        std::puts("smbios parser OK");
    return fails ? 1 : 0;
}
