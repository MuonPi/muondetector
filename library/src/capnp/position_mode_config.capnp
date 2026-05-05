@0xbf5147d4f6c1d2ab;

struct GeoPositionCapnp {
  longitude @0 :Float64 = 0.0;
  latitude @1 :Float64 = 0.0;
  altitude @2 :Float64 = 0.0;
  horError @3 :Float64 = 0.0;
  vertError @4 :Float64 = 0.0;
}

struct PositionModeConfigCapnp {
  enum Mode {
    static @0;
    lockIn @1;
    auto @2;
  }
  enum FilterType {
    none @0;
    kalman @1;
    histoMean @2;
    histoMedian @3;
    histoMpv @4;
  }
  mode @0 :Mode;
  staticPosition @1 :GeoPositionCapnp;
  lockInMaxDop @2 :Float64 = 3.0;
  lockInMinErrorMeters @3 :Float64 = 7.5;
  filterConfig @4 :FilterType;
}