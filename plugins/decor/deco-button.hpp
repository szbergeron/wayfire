#pragma once

#include <string>
#include <util.hpp>
#include <surface.hpp>
#include <animation.hpp>
#include <render-manager.hpp>

#include <cairo.h>

namespace wf
{
namespace decor
{

enum button_type_t
{
    BUTTON_CLOSE,
};

class button_t
{
  public:
    /**
     * Set the type of the button. This will affect the displayed icon and
     * potentially other appearance like colors.
     */
    void set_button_type(button_type_t type);

    /**
     * Set the button hover state.
     * Affects appearance.
     */
    void set_hover(bool is_hovered);

    /**
     * Set whether the button is pressed or not.
     * Affects appearance.
     */
    void set_pressed(bool is_pressed);

    /**
     * @return Whether the button needs to be repainted
     */
    bool needs_repaint();

    /**
     * Render the button on the given framebuffer at the given coordinates.
     * Precondition: set_button_type() has been called, otherwise result is no-op
     *
     * @param buffer The target framebuffer
     * @param geometry The geometry of the button, in logical coordinates
     */
    void render(const wf_framebuffer& buffer, wf_geometry geometry);

  private:
    /* Whether the button needs repaint */
    bool damaged = false;

    cairo_t *cr = nullptr;
    cairo_surface_t *button_icon = nullptr;
    cairo_surface_t *button_surface = nullptr;
    uint32_t button_texture = -1;

    /**
     * Redraw the button surface and store it as a texture
     */
    void update_texture();
};
}
}
