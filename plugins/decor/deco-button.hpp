#pragma once

#include <string>
#include <util.hpp>
#include <surface.hpp>
#include <render-manager.hpp>

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
     * Create a new button for the given surface at the given
     * position.
     *
     * @param surface The surface this button is attached to.
     * @param position The position of the button on the surface, in
     * logical coordinates (unscaled, untransformed)
     */
    button_t(wf::surface_interface_t *surface, wf_point position);

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
     * Render the button on the given framebuffer at the given coordinates.
     *
     * @param buffer The target framebuffer
     * @param position The position of the button in logical coordinates
     */
    void render(wf_framebuffer& buffer, wf_point position);

  private:
    wf::wl_idle_call idle_damage;
    /** Add damage to the output on next loop idle */
    void damage_self();

    wf::surface_interface_t *surface;

    /**
     * Position on the surface.
     * This can be different from the position the button is drawn at, because
     * the surface itself can be rendered at another place.
     */
    wf_point position;
};
}
}
