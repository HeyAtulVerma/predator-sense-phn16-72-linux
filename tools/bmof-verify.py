#!/usr/bin/env python3
"""Check daemon/MethodIds.h against the WMI MOF embedded in this machine's firmware.

The firmware ships its WMI class definitions as a compressed binary MOF (BMOF).
Windows resolves PredatorSense's method names through it; this tool does the
same and prints every AcerGamingFunction / APGeAction / BatteryControl method
with its WmiMethodId and parameters.

Usage: sudo tools/bmof-verify.py [bmof-file ...]
"""
import glob
import os
import re
import struct
import sys

CLASSES = {
    '7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56': 'AcerGamingFunction',
    '61EF69EA-865C-4BC3-A502-A0DEBA0CB531': 'APGeAction',
    '79772EC5-04B1-4BFD-843C-61E7F77B6CC9': 'BatteryControl',
}


class Bits:
    def __init__(self, data):
        self.d, self.pos = data, 0

    def bit(self):
        b = (self.d[self.pos >> 3] >> (self.pos & 7)) & 1
        self.pos += 1
        return b

    def bits(self, n):
        return sum(self.bit() << i for i in range(n))


def ds_decompress(data, outlen):
    """Microsoft "DS" (DoubleSpace/MRCI-style) LZ77 used by compressed BMOF."""
    if data[:2] != b'DS':
        raise ValueError('not DS-compressed')
    br, out = Bits(data[4:]), bytearray()
    while len(out) < outlen and (br.pos >> 3) < len(br.d) - 1:
        a, b = br.bit(), br.bit()
        if (a, b) == (0, 1):
            out.append(br.bits(7))
        elif (a, b) == (1, 0):
            out.append(br.bits(7) | 0x80)
        else:
            if a == 0:
                off = br.bits(6)
            elif br.bit() == 0:
                off = br.bits(8) + 64
            else:
                off = br.bits(12) + 320
                if off == 4415:  # block end marker
                    continue
            n = 0
            while br.bit() == 0:
                n += 1
                if n > 16:
                    raise ValueError('corrupt length')
            length = (1 << n) + 1 + br.bits(n)
            if not 0 < off <= len(out):
                raise ValueError('corrupt offset')
            for _ in range(length):
                out.append(out[-off])
    return bytes(out)


def load(path):
    raw = open(path, 'rb').read()
    magic, _ver, clen, dlen = struct.unpack_from('<4sIII', raw)
    if magic != b'FOMB':
        raise ValueError('not a BMOF')
    return ds_decompress(raw[16:16 + clen], dlen)


def methods(mof):
    strs = [(m.start(), m.group().decode('utf-16-le'))
            for m in re.finditer(rb'(?:[\x20-\x7e]\x00){2,}', mof)]
    cls, pending, params, result = None, None, [], {}
    for i, (off, s) in enumerate(strs):
        if s == 'guid' and i + 1 < len(strs):
            cls = CLASSES.get(strs[i + 1][1].strip('{}').upper())
            pending = None
            continue
        if not cls:
            continue
        if re.fullmatch(r'(Get|Set)[A-Za-z0-9]+', s):
            pending, params = s, []
        elif pending and s == 'CIMTYPE':
            params.append(strs[i + 1][1])
        elif pending and s in ('in', 'out'):
            params.append('[' + s + ']')
        elif pending and s == 'MAX':
            params.append('[%d]' % struct.unpack_from('<I', mof, off + 8)[0])
        elif pending and s == 'WmiMethodId':
            mid = struct.unpack_from('<I', mof, off + len(s) * 2 + 2)[0]
            result[(cls, pending)] = (mid, ' '.join(params))
            pending = None
    return result


def expected():
    here = os.path.dirname(os.path.abspath(__file__))
    src = open(os.path.join(here, '..', 'daemon', 'MethodIds.h')).read()
    return {name: int(v) for name, v in re.findall(r'constexpr unsigned (\w+)\s*=\s*(\d+);', src)}


def main():
    paths = sys.argv[1:] or glob.glob('/sys/bus/wmi/devices/05901221-D566-11D1-B2F0-00A0C9062910-*/bmof')
    found = {}
    for p in paths:
        try:
            found.update(methods(load(p)))
        except (OSError, ValueError) as e:
            print(f'{p}: {e}', file=sys.stderr)
    if not found:
        sys.exit('no Acer WMI classes found (run as root?)')
    want = expected()
    bad = 0
    for (cls, name), (mid, params) in sorted(found.items()):
        mark = ''
        if name in want:
            mark = 'OK ' if want[name] == mid else 'MISMATCH (code has %d)' % want[name]
            bad += want[name] != mid
        print(f'{cls:20} {mid:3}  {name:30} {mark:4} {params}')
    missing = [n for n in want if n not in {k[1] for k in found}]
    for n in missing:
        print(f'missing in firmware: {n}')
    sys.exit(1 if bad or missing else 0)


if __name__ == '__main__':
    main()
