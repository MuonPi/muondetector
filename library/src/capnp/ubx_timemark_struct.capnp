@0x9ff021183fd64140;

struct UbxTimeMarkStructCapnp {
  rising @0 :TimeSpecCapnp;
  falling @1 :TimeSpecCapnp;

  risingValid @2 :Bool = false;
  fallingValid @3 :Bool = false;

  accuracyNs @4 :UInt32 = 0;
  valid @5 :Bool = false;

  timeBase @6 :UInt8 = 0;
  utcAvailable @7 :Bool = false;

  flags @8 :UInt8 = 0;
  evtCounter @9 :UInt16 = 0;
}

struct TimeSpecCapnp {
  seconds @0 :Int64 = 0;
  nanoseconds @1 :Int32 = 0;
}