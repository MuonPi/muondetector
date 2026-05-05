@0xe16519bf0d4657b5;

struct VersionEventCapnp {
  hwVer @0 :VersionCapnp;
  swVer @1 :VersionCapnp;
}

struct VersionCapnp {
  major @0 :Int32;
  minor @1 :Int32;
  patch @2 :Int32;
  additional @3 :Text;
  hash @4 :Text;
}