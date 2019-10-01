#pragma once
#include <render-manager.hpp>
#include "deco-button.hpp"

namespace wf
{
namespace decor
{

/**
 * A  class which manages the outlook of decorations.
 * It is responsible for determining the background colors, sizes, etc.
 */
class decoration_theme_t
{
  public:
    /** Create a new theme with the default parameters */
    decoration_theme_t();

    /** @return The available height for displaying the title */
    int get_title_height() const;
    /** @return The available border for resizing */
    int get_border_size() const;

    /**
     * Fill the given rectange with the background color(s).
     *
     * @param fb The target framebuffer, must have been bound already
     * @param rectangle The rectangle to redraw.
     * @param scissor The GL scissor rectangle to use.
     * @param active Whether to use active or inactive colors
     */
    void render_background(const wf_framebuffer& fb, wf_geometry rectangle,
        const wf_geometry& scissor, bool active) const;

    /**
     * Render the given text on a cairo_surface_t with the given size.
     * The caller is responsible for freeing the memory afterwards.
     */
    cairo_surface_t *render_text(std::string text, int width, int height) const;

    /**
     * Get the icon for the given button.
     * The caller is responsible for freeing the memory afterwards.
     *
     * @param button The button type.
     * @param width The width of the generated surface.
     * @param height The height of the generated surface.
     */
    cairo_surface_t *get_button_surface(button_type_t button,
        int width, int height) const;

  private:
    wf_option font_opt;
    wf_option title_height;
    wf_option border_size;
    wf_option active_color;
    wf_option inactive_color;
};
}
}
