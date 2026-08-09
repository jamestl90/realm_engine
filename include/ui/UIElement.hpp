#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <limits>

namespace ui {

// Forward declarations
class UIElement;

// Unique identifier for UI elements
using ElementID = std::uint32_t;
constexpr ElementID INVALID_ELEMENT_ID = 0;

// Rectangle for layout calculations
struct Rect {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};

    [[nodiscard]] constexpr bool contains(float px, float py) const noexcept {
        return px >= x && px < x + width && py >= y && py < y + height;
    }

    [[nodiscard]] constexpr float right() const noexcept { return x + width; }
    [[nodiscard]] constexpr float bottom() const noexcept { return y + height; }
};

// Size constraints for layout
struct SizeConstraints {
    float min_width{0.0f};
    float min_height{0.0f};
    float max_width{std::numeric_limits<float>::max()};
    float max_height{std::numeric_limits<float>::max()};
    float preferred_width{0.0f};
    float preferred_height{0.0f};
};

struct TextMetrics {
    float width{0.0f};
    float height{0.0f};
};

using TextMeasureCallback = std::function<TextMetrics(const std::string& text, float fontSize)>;

// Visibility state
enum class Visibility : std::uint8_t {
    Visible,    // Rendered and participates in layout
    Hidden,     // Not rendered but participates in layout
    Collapsed   // Not rendered and does not participate in layout
};

// Base class for all UI elements
class UIElement {
public:
    explicit UIElement(ElementID id = INVALID_ELEMENT_ID) noexcept;
    virtual ~UIElement();

    // Non-copyable, movable
    UIElement(const UIElement&) = delete;
    UIElement& operator=(const UIElement&) = delete;
    UIElement(UIElement&&) noexcept;
    UIElement& operator=(UIElement&&) noexcept;

    // Identity
    [[nodiscard]] ElementID id() const noexcept { return m_id; }
    void setName(std::string name) noexcept { m_name = std::move(name); }
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }

    // Hierarchy
    [[nodiscard]] UIElement* parent() const noexcept { return m_parent; }
    [[nodiscard]] const std::vector<std::unique_ptr<UIElement>>& children() const noexcept { return m_children; }
    void addChild(std::unique_ptr<UIElement> child);
    std::unique_ptr<UIElement> removeChild(UIElement* child);
    void clearChildren();

    // Layout
    [[nodiscard]] const Rect& bounds() const noexcept { return m_bounds; }
    [[nodiscard]] const Rect& localBounds() const noexcept { return m_localBounds; }
    void setBounds(const Rect& bounds) noexcept;
    void setLocalBounds(const Rect& bounds) noexcept;

    [[nodiscard]] const SizeConstraints& sizeConstraints() const noexcept { return m_sizeConstraints; }
    void setSizeConstraints(const SizeConstraints& constraints) noexcept { m_sizeConstraints = constraints; }

    // Supplied by UIManager so layout uses the same font metrics as rendering.
    void setTextMeasurer(TextMeasureCallback callback);

    // Visibility
    [[nodiscard]] Visibility visibility() const noexcept { return m_visibility; }
    void setVisibility(Visibility vis) noexcept { m_visibility = vis; }
    [[nodiscard]] bool isVisible() const noexcept { return m_visibility == Visibility::Visible; }

    // Enable/disable
    [[nodiscard]] bool isEnabled() const noexcept { return m_enabled; }
    void setEnabled(bool enabled) noexcept { m_enabled = enabled; }

    // Layout invalidation
    void invalidateLayout() noexcept;
    [[nodiscard]] bool isLayoutDirty() const noexcept { return m_layoutDirty; }

    // Core virtual methods
    virtual void measure(float availableWidth, float availableHeight);
    virtual void arrange(const Rect& finalRect);
    virtual void update(float dt);

    // Hit testing
    [[nodiscard]] virtual bool hitTest(float x, float y) const;
    [[nodiscard]] UIElement* hitTestRecursive(float x, float y);

    // Measured size accessors for layout containers
    [[nodiscard]] float measuredWidth() const noexcept { return m_measuredWidth; }
    [[nodiscard]] float measuredHeight() const noexcept { return m_measuredHeight; }

protected:
    void setParent(UIElement* parent) noexcept { m_parent = parent; }
    void markLayoutClean() noexcept { m_layoutDirty = false; }
    [[nodiscard]] TextMetrics measureText(const std::string& text, float fontSize) const;

    // Measured size from measure pass
    float m_measuredWidth{0.0f};
    float m_measuredHeight{0.0f};

    // Layout bounds - accessible to derived classes
    Rect m_bounds{};        // Screen-space bounds
    Rect m_localBounds{};   // Parent-relative bounds

private:
    ElementID m_id{INVALID_ELEMENT_ID};
    std::string m_name;
    UIElement* m_parent{nullptr};
    std::vector<std::unique_ptr<UIElement>> m_children;

    SizeConstraints m_sizeConstraints{};
    TextMeasureCallback m_textMeasurer;

    Visibility m_visibility{Visibility::Visible};
    bool m_enabled{true};
    bool m_layoutDirty{true};
};

// ID generator for UI elements
class ElementIDGenerator {
public:
    [[nodiscard]] static ElementID next() noexcept {
        static ElementID s_nextId = 1;
        return s_nextId++;
    }
};

} // namespace ui
