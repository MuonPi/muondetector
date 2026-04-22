#ifndef DATASTORE_H
#define DATASTORE_H

#include <any>
#include <typeindex>
#include <unordered_map>

class Datastore {
  public:
    template <typename T>
    void store(const T& value) {
        data_[std::type_index(typeid(T))] = value;
    }

    template <typename T>
    const T* get() const {
        auto it = data_.find(std::type_index(typeid(T)));
        if (it != data_.end()) {
            return std::any_cast<T>(&it->second);
        }
        return nullptr;
    }

  private:
    std::unordered_map<std::type_index, std::any> data_;
    mutable std::mutex mutex_;
};

#endif // DATASTORE_H