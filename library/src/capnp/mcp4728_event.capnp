@0xe16519bf0d4657b5;

struct MCP4728EventCapnp {
  dacValues @0 :List(DacEntry);
  eepromValues @1 :List(EepromEntry);
  voltages @2 :List(VoltageEntry);
}

struct DacEntry {
  channel @0 :UInt8;
  value @1 :UInt16;
}

struct EepromEntry {
  channel @0 :UInt8;
  value @1 :UInt16;
}

struct VoltageEntry {
  channel @0 :UInt8;
  value @1 :Float32;
}