@0xcf354d2da303b1b4;

struct MonTxCapnp {
  pending @0 :List(UInt16);
  usage @1 :List(UInt8);
  peakUsage @2 :List(UInt8);
  tUsage @3 :UInt8 = 0;
  tPeakUsage @4 :UInt8 = 0;
}