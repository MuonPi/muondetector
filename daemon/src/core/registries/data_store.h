#ifndef DATA_STORE_H
#define DATA_STORE_H

#include <any>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <typeindex>
#include <unordered_map>

class Histogram;
class GeoPosManager;
class DataStore {
  private:
    struct Entry {
        std::any value;
        std::chrono::system_clock::time_point last_update;
    };

  public:
    DataStore();
    ~DataStore();

    template <typename T>
    void store(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        data_[std::type_index(typeid(T))] = Entry{value, std::chrono::system_clock::now()};
    }

    template <typename T>
    const T* get() const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = data_.find(std::type_index(typeid(T)));
        if (it != data_.end()) {
            return std::any_cast<T>(&it->second.value);
        }

        return nullptr;
    }

    template <typename T>
    const T* getEntry() const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = data_.find(std::type_index(typeid(T)));
        if (it != data_.end()) {
            return &it;
        }

        return nullptr;
    }

    template <typename T>
    std::optional<std::chrono::system_clock::time_point> lastUpdate() const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = data_.find(std::type_index(typeid(T)));
        if (it != data_.end()) {
            return it->second.last_update;
        }

        return std::nullopt;
    }

    // Histograms
    void setupHistos();
    auto clearHisto(const std::string& histoName) -> std::shared_ptr<Histogram>;
    void fillHisto(const std::string& histoName, double value);
    auto histo(const std::string& histoName) -> std::shared_ptr<Histogram>;
    auto allHistos() const -> const std::unordered_map<std::string, std::shared_ptr<Histogram>>&;

    // GeoPos
    auto geoPosManager() -> GeoPosManager&;

  private:
    std::unordered_map<std::string, std::shared_ptr<Histogram>> m_histo_map;
    std::unique_ptr<GeoPosManager> m_geopos_manager;
    std::unordered_map<std::type_index, Entry> data_;
    mutable std::mutex mutex_;
};

#endif // DATASTORE_H