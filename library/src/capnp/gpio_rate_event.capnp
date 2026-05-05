@0xa232c0b762dc2c63;

struct GpioRateEventCapnp {
  whichRate @0 :UInt8 = 0;
  rate @1 :List(FloatPair);
}

struct FloatPair {
  first @0 :Float32;
  second @1 :Float32;
}