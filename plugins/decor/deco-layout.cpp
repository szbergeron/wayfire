#include "deco-layout.hpp"

#define BUTTON_ASPECT_RATIO 1.5
#define BUTTON_HEIGHT_PC 0.8

namespace wf
{
namespace decor
{
/**
 * Represents an area of the decoration which reacts to input events.
 */
decoration_area_t::decoration_area_t(decoration_area_type_t type, wf_geometry g)
{
    this->type = type;
    this->geometry = g;

    if (this->type == DECORATION_AREA_BUTTON)
        button = std::make_unique<button_t>();
}

wf_geometry decoration_area_t::get_geometry() const
{
    return geometry;
}

button_t& decoration_area_t::as_button()
{
    assert(button);
    return *button;
}

decoration_area_type_t decoration_area_t::get_type() const
{
    return type;
}

decoration_layout_t::decoration_layout_t(int thickness, int border) :
    titlebar_size(thickness),
    border_size(border),
    button_width(titlebar_size * BUTTON_HEIGHT_PC * BUTTON_ASPECT_RATIO),
    button_height(titlebar_size * BUTTON_HEIGHT_PC),
    button_padding((titlebar_size - button_height) / 2)
{
    assert(titlebar_size >= border_size);
}

/** Regenerate layout using the new size */
void decoration_layout_t::resize(int width, int height)
{
    this->layout_areas.clear();

    /* Close button */
    wf_geometry button_geometry = {
        width - border_size - button_padding - button_width,
        button_padding,
        button_width,
        button_height,
    };

    this->layout_areas.push_back(std::make_unique<decoration_area_t>(
            DECORATION_AREA_BUTTON, button_geometry));
    this->layout_areas.back()->as_button().set_button_type(BUTTON_CLOSE);

    /* Padding around the button, allows move */
    button_geometry.x -= button_padding;
    button_geometry.y -= button_padding;
    button_geometry.width += 2 * button_padding;
    button_geometry.height += 2 * button_padding;
    this->layout_areas.push_back(std::make_unique<decoration_area_t> (
            DECORATION_AREA_MOVE, button_geometry));

    /* Titlebar dragging area (for move) */
    wf_geometry title_geometry = {
        border_size,
        border_size,
        /* Up to the button, but subtract the padding to the left of the title
         * and the padding between title and button */
        button_geometry.x - border_size,
        titlebar_size,
    };
    this->layout_areas.push_back(std::make_unique<decoration_area_t>(
            DECORATION_AREA_TITLE, title_geometry));

    /* Resizing edges - left */
    wf_geometry border_geometry = { 0, 0, border_size, height };
    this->layout_areas.push_back(std::make_unique<decoration_area_t> (
            DECORATION_AREA_RESIZE, border_geometry));

    /* Resizing edges - right */
    border_geometry = { width - border_size, 0, border_size, height };
    this->layout_areas.push_back(std::make_unique<decoration_area_t> (
            DECORATION_AREA_RESIZE, border_geometry));

    /* Resizing edges - top */
    border_geometry = { 0, 0, width, border_size };
    this->layout_areas.push_back(std::make_unique<decoration_area_t> (
            DECORATION_AREA_RESIZE, border_geometry));

    /* Resizing edges - bottom */
    border_geometry = { 0, height - border_size, width, border_size };
    this->layout_areas.push_back(std::make_unique<decoration_area_t> (
            DECORATION_AREA_RESIZE, border_geometry));
}

/**
 * @return The decoration areas which need to be rendered, in top to bottom
 *  order.
 */
std::vector<nonstd::observer_ptr<decoration_area_t>>
decoration_layout_t::get_renderable_areas()
{
    std::vector<nonstd::observer_ptr<decoration_area_t>> renderable;
    for (auto& area : layout_areas)
    {
        if (area->get_type() & DECORATION_AREA_RENDERABLE_BIT)
            renderable.push_back({area});
    }

    return renderable;
}

wf_region decoration_layout_t::calculate_region() const
{
    wf_region r{};
    for (auto& area : layout_areas)
        r |= area->get_geometry();

    return r;
}

/** Handle motion event to (x, y) relative to the decoration */
void decoration_layout_t::handle_motion(int x, int y)
{
    this->current_input = {x, y};
    /* TODO: hover */
}

/**
 * Handle press or release event.
 * @param pressed Whether the event is a press(true) or release(false)
 *  event.
 * @return The action which needs to be carried out in response to this
 *  event.
 * */
decoration_layout_action_t decoration_layout_t::handle_press_event(bool pressed)
{
    if (pressed)
    {
        auto area = find_area_at(current_input);
        if (area && area->get_type() == DECORATION_AREA_MOVE)
            return DECORATION_ACTION_MOVE;
        if (area && area->get_type() == DECORATION_AREA_RESIZE)
            return DECORATION_ACTION_RESIZE;

        is_grabbed = true;
        grab_origin = current_input;
    }

    if (!pressed && is_grabbed)
    {
        auto begin_area = find_area_at(grab_origin);
        auto end_area = find_area_at(current_input);

        if (begin_area && end_area && begin_area == end_area)
        {
            if (begin_area->get_type() == DECORATION_AREA_BUTTON)
            {
                /* TODO: return button action */
            }
        }
    }

    return DECORATION_ACTION_NONE;
}

/**
 * Find the layout area at the given coordinates, if any
 * @return The layout area or null on failure
 */
nonstd::observer_ptr<decoration_area_t>
decoration_layout_t::find_area_at(wf_point point)
{
    for (auto& area : this->layout_areas)
    {
        if (area->get_geometry() & point)
            return {area};
    }

    return nullptr;
}

void decoration_layout_t::handle_focus_lost()
{
    this->is_grabbed = false;
}

}
}
