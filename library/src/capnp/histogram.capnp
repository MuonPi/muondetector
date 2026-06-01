@0x9d7d2f8e6b5a4c31;

struct HistogramCapnp {
  name       @0 :Text;
  unit       @1 :Text;

  nrBins     @2 :Int32;
  min        @3 :Float64;
  max        @4 :Float64;

  overflow   @5 :Float64;
  underflow  @6 :Float64;

  autoscale  @7 :Bool;

  bins       @8 :List(BinEntryCapnp);
}

struct BinEntryCapnp {
  bin     @0 :Int32;
  content @1 :Float64;
}