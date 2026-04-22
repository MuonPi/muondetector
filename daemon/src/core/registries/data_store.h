#ifndef DATA_STORE_H
#define DATA_STORE_H

class DataStore {
  public:
    void update(const GpsEvent& e);
    void update(const TempEvent& e);

    std::optional<GpsEvent> latestGps() const;

  private:
    mutable std::mutex m;

    GpsEvent latestGps_;
    TempEvent latestTemp_;
};

#endif // DATA_STORE_H