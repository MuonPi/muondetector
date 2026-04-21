#!/usr/bin/env python3

import sys
import yaml
from pathlib import Path


TYPE_MAP = {
    "uint8": "UInt8",
    "uint16": "UInt16",
    "uint32": "UInt32",
    "uint64": "UInt64",
    "int8": "Int8",
    "int16": "Int16",
    "int32": "Int32",
    "int64": "Int64",
    "float": "Float32",
    "double": "Float64",
    "bool": "Bool",
    "string": "Text",
}


def map_type(t: str) -> str:
    if t not in TYPE_MAP:
        raise RuntimeError(f"Unknown type: {t}")
    return TYPE_MAP[t]


def generate_capnp(data) -> str:
    lines = []

    # fixed id for now
    lines.append("@0xbf5147cbbecf40af;")
    lines.append("")

    commands = data.get("commands", [])

    for cmd in commands:
        name = cmd["name"]
        fields = cmd.get("fields", [])

        lines.append(f"struct {name} {{")

        for idx, field in enumerate(fields):
            fname = field["name"]
            ftype = map_type(field["type"])
            lines.append(f"  {fname} @{idx} :{ftype};")

        lines.append("}")
        lines.append("")

    return "\n".join(lines)


def main():
    if len(sys.argv) != 3:
        print("Usage:")
        print("  generate.py protocol.yaml output.capnp")
        sys.exit(1)

    input_file = Path(sys.argv[1])
    output_file = Path(sys.argv[2])

    if not input_file.exists():
        print(f"Input file not found: {input_file}")
        sys.exit(2)

    with open(input_file, "r", encoding="utf-8") as f:
        data = yaml.safe_load(f)

    text = generate_capnp(data)

    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_text(text, encoding="utf-8")

    print(f"Generated {output_file}")


if __name__ == "__main__":
    main()