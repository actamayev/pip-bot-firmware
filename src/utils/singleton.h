#pragma once

template <typename T> class Singleton {
  protected:
    static T* instance;

    // Protected constructor
    Singleton() {}

    // Delete copy constructor and assignment operator
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

  public:
    static T& get_instance() {
        if (instance == nullptr) {
            instance = new T();
        }
        return *instance;
    }

    virtual ~Singleton() = default;
};

// Initialize the static member
template <typename T> T* Singleton<T>::instance = nullptr;
