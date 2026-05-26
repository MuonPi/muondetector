#ifndef GEOHASH_H
#define GEOHASH_H

#include <string>

class GeoHash {
  public:
    static auto hashFromCoordinates(double lon, double lat, int precision = 6) -> std::string;

  private:
    GeoHash() = delete;
};

#endif // GEOHASH_H
