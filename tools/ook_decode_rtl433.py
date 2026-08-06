#!/usr/bin/env python3

import argparse
import json
import re
import sys
import time
from dataclasses import dataclass


SYNC = bytes.fromhex("555555552d")
HEX_KEYS = {"data", "code", "codes"}
PLAIN_HEX_RE = re.compile(r"\b(?:data|code|codes)\b\s*[:=]\s*([^\r\n]+)", re.IGNORECASE)


@dataclass(frozen=True)
class DecodedFrame:
    frame_hex: str
    payload: bytes
    crc_received: int
    crc_expected: int


def crc8(data: bytes) -> int:
    crc = 0x00
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x07) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc


def parse_hexish(value: str) -> bytes | None:
    value = re.sub(r"^\s*\{\d+\}\s*", "", value)
    value = value.replace("0x", "").replace("0X", "")
    hex_text = "".join(re.findall(r"[0-9a-fA-F]", value))

    if len(hex_text) < len(SYNC) * 2 or len(hex_text) % 2 != 0:
        return None

    try:
        return bytes.fromhex(hex_text)
    except ValueError:
        return None


def collect_json_candidates(value, key: str = "") -> list[bytes]:
    candidates: list[bytes] = []

    if isinstance(value, dict):
        for child_key, child_value in value.items():
            candidates.extend(collect_json_candidates(child_value, str(child_key).lower()))
    elif isinstance(value, list):
        for item in value:
            candidates.extend(collect_json_candidates(item, key))
    elif isinstance(value, str) and key in HEX_KEYS:
        parsed = parse_hexish(value)
        if parsed is not None:
            candidates.append(parsed)

    return candidates


def collect_plain_candidates(line: str) -> list[bytes]:
    candidates: list[bytes] = []
    for match in PLAIN_HEX_RE.finditer(line):
        parsed = parse_hexish(match.group(1))
        if parsed is not None:
            candidates.append(parsed)
    return candidates


def collect_candidates(line: str) -> list[bytes]:
    try:
        value = json.loads(line)
    except json.JSONDecodeError:
        return collect_plain_candidates(line)

    return collect_json_candidates(value)


def decode_frames(data: bytes) -> list[DecodedFrame]:
    frames: list[DecodedFrame] = []
    search_from = 0

    while True:
        sync_at = data.find(SYNC, search_from)
        if sync_at < 0:
            break

        length_at = sync_at + len(SYNC)
        if len(data) <= length_at:
            break

        payload_len = data[length_at]
        payload_at = length_at + 1
        crc_at = payload_at + payload_len
        if len(data) <= crc_at:
            search_from = sync_at + 1
            continue

        payload = data[payload_at:crc_at]
        crc_received = data[crc_at]
        frame = data[sync_at : crc_at + 1]
        frames.append(
            DecodedFrame(
                frame_hex=frame.hex(),
                payload=payload,
                crc_received=crc_received,
                crc_expected=crc8(payload),
            )
        )
        search_from = sync_at + 1

    return frames


def format_frame(frame: DecodedFrame, encoding: str, raw: bool) -> str:
    payload_text = frame.payload.decode(encoding, errors="replace")
    if raw:
        return payload_text

    crc_status = "ok" if frame.crc_received == frame.crc_expected else "bad"
    return (
        f"payload={payload_text!r} "
        f"payload_hex={frame.payload.hex()} "
        f"crc={crc_status} "
        f"frame={frame.frame_hex}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Decode MuonPi OOK frames from rtl_433 flex-decoder output."
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="print every repeated frame too; this is the default unless --dedupe-window is set",
    )
    parser.add_argument(
        "--dedupe-window",
        type=float,
        default=0.0,
        metavar="SECONDS",
        help="suppress identical frames only within this many seconds, default: 0.0",
    )
    parser.add_argument(
        "--encoding",
        default="ascii",
        help="payload text encoding, default: ascii",
    )
    parser.add_argument(
        "--raw",
        action="store_true",
        help="print only the decoded payload text (CRC-valid frames by default)",
    )
    parser.add_argument(
        "--accept-bad-crc",
        action="store_true",
        help="also print corrupted frames; useful when investigating reception problems",
    )
    args = parser.parse_args()

    last_printed: dict[str, float] = {}
    for line in sys.stdin:
        for candidate in collect_candidates(line):
            for frame in decode_frames(candidate):
                if frame.crc_received != frame.crc_expected and not args.accept_bad_crc:
                    continue

                now = time.monotonic()
                last_seen = last_printed.get(frame.frame_hex)
                if (
                    not args.all
                    and args.dedupe_window > 0
                    and last_seen is not None
                    and now - last_seen < args.dedupe_window
                ):
                    continue
                last_printed[frame.frame_hex] = now
                print(format_frame(frame, args.encoding, args.raw), flush=True)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
