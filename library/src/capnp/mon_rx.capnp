@0xe34aaa129e6e10e0;

struct MonRxCapnp {
  pending @0 :List(UInt16);
  usage @1 :List(UInt8);
  peakUsage @2 :List(UInt8);
  tUsage @3 :UInt8 = 0;
  tPeakUsage @4 :UInt8 = 0;
}