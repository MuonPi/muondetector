@0x8c6eda5113c5309f;

struct Ads1115EventCapnp {
  deviceId @0 :UInt32;
  channel @1 :UInt8;
  rawValue @2 :UInt16;
  voltage @3 :Float32;
  timestamp @4 :UInt64;
}