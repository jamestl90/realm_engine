#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <any>

namespace ui {

// Forward declarations
class UIElement;

// Property change notification
class INotifyPropertyChanged {
public:
    virtual ~INotifyPropertyChanged() = default;

    using PropertyChangedCallback = std::function<void(const std::string& propertyName)>;

    virtual void addPropertyChangedListener(PropertyChangedCallback callback) = 0;
    virtual void removeAllListeners() = 0;

protected:
    virtual void notifyPropertyChanged(const std::string& propertyName) = 0;
};

// Observable value wrapper
template<typename T>
class Observable : public INotifyPropertyChanged {
public:
    Observable() = default;
    explicit Observable(T value) : m_value(std::move(value)) {}

    [[nodiscard]] const T& get() const noexcept { return m_value; }

    void set(T value) {
        if (m_value != value) {
            m_value = std::move(value);
            notifyPropertyChanged("value");
        }
    }

    Observable& operator=(const T& value) {
        set(value);
        return *this;
    }

    operator const T&() const noexcept { return m_value; }

    void addPropertyChangedListener(PropertyChangedCallback callback) override {
        m_listeners.push_back(std::move(callback));
    }

    void removeAllListeners() override {
        m_listeners.clear();
    }

protected:
    void notifyPropertyChanged(const std::string& propertyName) override {
        for (const auto& listener : m_listeners) {
            if (listener) {
                listener(propertyName);
            }
        }
    }

private:
    T m_value{};
    std::vector<PropertyChangedCallback> m_listeners;
};

// Binding mode
enum class BindingMode : std::uint8_t {
    OneWay,         // Source -> Target only
    TwoWay,         // Source <-> Target
    OneWayToSource  // Target -> Source only
};

// Binding base class
class BindingBase {
public:
    virtual ~BindingBase() = default;
    virtual void update() = 0;
    virtual void unbind() = 0;
};

// Type-erased binding
template<typename TSource, typename TTarget>
class Binding : public BindingBase {
public:
    using SourceGetter = std::function<TSource()>;
    using TargetSetter = std::function<void(const TTarget&)>;
    using Converter = std::function<TTarget(const TSource&)>;

    Binding(SourceGetter sourceGetter, TargetSetter targetSetter, Converter converter = nullptr)
        : m_sourceGetter(std::move(sourceGetter))
        , m_targetSetter(std::move(targetSetter))
        , m_converter(std::move(converter)) {
    }

    void update() override {
        if (!m_sourceGetter || !m_targetSetter) {
            return;
        }

        TSource sourceValue = m_sourceGetter();
        if (m_converter) {
            m_targetSetter(m_converter(sourceValue));
        } else if constexpr (std::is_convertible_v<TSource, TTarget>) {
            m_targetSetter(static_cast<TTarget>(sourceValue));
        }
    }

    void unbind() override {
        m_sourceGetter = nullptr;
        m_targetSetter = nullptr;
    }

private:
    SourceGetter m_sourceGetter;
    TargetSetter m_targetSetter;
    Converter m_converter;
};

// Binding context - manages bindings for a UI tree
class BindingContext {
public:
    BindingContext() = default;
    ~BindingContext() = default;

    // Non-copyable
    BindingContext(const BindingContext&) = delete;
    BindingContext& operator=(const BindingContext&) = delete;
    BindingContext(BindingContext&&) noexcept = default;
    BindingContext& operator=(BindingContext&&) noexcept = default;

    // Add a binding
    template<typename TSource, typename TTarget>
    void bind(
        std::function<TSource()> sourceGetter,
        std::function<void(const TTarget&)> targetSetter,
        std::function<TTarget(const TSource&)> converter = nullptr
    ) {
        auto binding = std::make_unique<Binding<TSource, TTarget>>(
            std::move(sourceGetter),
            std::move(targetSetter),
            std::move(converter)
        );
        m_bindings.push_back(std::move(binding));
    }

    // Bind to an observable
    template<typename T>
    void bindObservable(Observable<T>& observable, std::function<void(const T&)> targetSetter) {
        // Initial update
        targetSetter(observable.get());

        // Listen for changes
        observable.addPropertyChangedListener([&observable, setter = std::move(targetSetter)](const std::string&) {
            setter(observable.get());
        });
    }

    // Update all bindings
    void updateAll() {
        for (auto& binding : m_bindings) {
            if (binding) {
                binding->update();
            }
        }
    }

    // Clear all bindings
    void clear() {
        for (auto& binding : m_bindings) {
            if (binding) {
                binding->unbind();
            }
        }
        m_bindings.clear();
    }

private:
    std::vector<std::unique_ptr<BindingBase>> m_bindings;
};

} // namespace ui
