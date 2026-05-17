#ifndef DATA_STORE_H
#define DATA_STORE_H

#include "utility/geoposmanager.h"
#include "utility/ublox_ratebuffer.h"

#include <any>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <typeindex>
#include <unordered_map>

class Histogram;
struct GnssPosStruct;
struct UbxTimeMarkStruct;
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

        if constexpr (has_histo_filler_v<T>) {
            fillHisto(value);
        }
        data_[std::type_index(typeid(T))] = Entry{value, std::chrono::system_clock::now()};
    }

    // TODO: Replace any_cast with std::optional !

    template <typename T>
    const T* get() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return getUnlocked<T>();
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

    template <typename... Ts>
    struct type_list {};

    template <typename T, typename List>
    struct contains;

    template <typename T, typename... Ts>
    struct contains<T, type_list<Ts...>> : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

    // Define here which messages will be used to fill histograms with data
    // For each type in the list there must be a specialization in data_store.cpp
    using histo_enabled_types = type_list<GnssPosStruct, UbxTimeMarkStruct>;
    // AdcTraceEvent,
    // Ads1115Event,
    // NavSat,
    // UbxMsgRates

    template <typename T>
    inline static constexpr bool has_histo_filler_v = contains<T, histo_enabled_types>::value;

    // Prevent definition in list "histo_enabled_types" but no specialization
    // -> default template function static_assert
    template <typename T>
    void fillHisto(const T&) {
        static_assert(sizeof(T) == 0, "No specialization for function 'fillHisto' for this type");
    }

    auto histo(const std::string& histoName) -> std::shared_ptr<Histogram>;
    auto allHistos() const -> const std::unordered_map<std::string, std::shared_ptr<Histogram>>&;

    // GeoPos
    auto geoPosManager() -> GeoPosManager&;

    // Ublox Ratebuffer
    auto ubloxRateBuffer() -> CounterRateBuffer&;

  private:
    template <typename T>
    const T* getUnlocked() const {
        auto it = data_.find(std::type_index(typeid(T)));
        if (it != data_.end()) {
            return std::any_cast<T>(&it->second.value);
        }
        return nullptr;
    }
    std::unordered_map<std::string, std::shared_ptr<Histogram>> m_histo_map;
    std::unique_ptr<GeoPosManager> m_geopos_manager;
    std::unique_ptr<CounterRateBuffer> m_ublox_ratebuffer;
    std::unordered_map<std::type_index, Entry> data_;
    mutable std::mutex mutex_;
};

template <>
void DataStore::fillHisto(const GnssPosStruct& pos);

template <>
void DataStore::fillHisto(const UbxTimeMarkStruct& tm);

#endif // DATASTORE_H