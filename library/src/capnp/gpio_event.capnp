@0x85dbd2ce93ec4483;

struct GpioEventCapnp {
  sig @0 :UInt8;
  pin @1 :UInt8;
  timestamp @2 :UInt64;
  edge @3 :UInt8;
}