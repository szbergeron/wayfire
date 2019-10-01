#pragma once

#include <vector>
#include <util.hpp>
#include "deco-button.hpp"

namespace wf
{
namespace decor
{
static constexpr uint32_t DECORATION_AREA_RENDERABLE_BIT = (1 << 16);
/** Different types of areas around the decoration */
enum decoration_area_type_t
{
    DECORATION_AREA_MOVE = 0,
    DECORATION_AREA_TITLE = 1 | DECORATION_AREA_RENDERABLE_BIT,
    DECORATION_AREA_RESIZE = 2,
    DECORATION_AREA_BUTTON = 3 | DECORATION_AREA_RENDERABLE_BIT,
};

/**
 * Represents an area of the decoration which reacts to input events.
 */
struct decoration_area_t
{
  public:
    /**
     * Initialize a new decoration area with the given type and geometry
     */
    decoration_area_t(decoration_area_type_t type, wf_geometry g);

    /**
     * Initialize a new decoration area holding a button
     */
    decoration_area_t(wf_geometry g, const decoration_theme_t& theme);

    /** @return The geometry of the decoration area, relative to the layout */
    wf_geometry get_geometry() const;

    /** @return The area's button, if the area is a button. Otherwise UB */
    button_t& as_button();

    /** @return The type of the decoration area */
    decoration_area_type_t get_type() const;

  private:
    decoration_area_type_t type;
    wf_geometry geometry;

    /* For buttons only */
    std::unique_ptr<button_t> button;
};


/**
 * Action which needs to be taken in response to an input event
 */
enum decoration_layout_action_t
{
    DECORATION_ACTION_NONE     = 0,

    /* Drag actions */
    DECORATION_ACTION_MOVE     = 1,
    DECORATION_ACTION_RESIZE   = 2,

    /* Button actions */
    DECORATION_ACTION_CLOSE    = 3,
    DECORATION_ACTION_MAXIMIZE = 4,
    DECORATION_ACTION_MINIMIZE = 5,
};

class decoration_theme_t;
/**
 * Manages the layout of the decorations, i.e positioning of the title,
 * buttons, etc.
 *
 * Also dispatches the input events to the appropriate place.
 */
class decoration_layout_t
{
  public:
    /**
     * Create a new decoration layout for the given theme.
     * When the theme changes, the decoration layout needs to be created again.
     */
    decoration_layout_t(const decoration_theme_t& theme);

    /** Regenerate layout using the new size */
    void resize(int width, int height);

    /**
     * @return The decoration areas which need to be rendered, in top to bottom
     *  order.
     */
    std::vector<nonstd::observer_ptr<decoration_area_t>> get_renderable_areas();

    /** @return The combined region of all layout areas */
    wf_region calculate_region() const;

    /** Handle motion event to (x, y) relative to the decoration */
    void handle_motion(int x, int y);

    /**
     * Handle press or release event.
     * @param pressed Whether the event is a press(true) or release(false)
     *  event.
     * @return The action which needs to be carried out in response to this
     *  event.
     */
    decoration_layout_action_t handle_press_event(bool pressed = true);

    /**
     * Handle focus lost event.
     */
    void handle_focus_lost();

  private:
    const int titlebar_size;
    const int border_size;
    const int button_width;
    const int button_height;
    const int button_padding;
    const decoration_theme_t& theme;

    std::vector<std::unique_ptr<decoration_area_t>> layout_areas;

    bool is_grabbed = false;
    /* Position where the grab has started */
    wf_point grab_origin;
    /* Last position of the input */
    wf_point current_input;

    /**
     * Find the layout area at the given coordinates, if any
     * @return The layout area or null on failure
     */
    nonstd::observer_ptr<decoration_area_t> find_area_at(wf_point point);

};
}
}
