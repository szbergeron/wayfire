#include "deco-button.hpp"
#include <opengl.hpp>
#include <cairo-util.hpp>

namespace wf
{
namespace decor
{

void button_t::set_button_type(button_type_t type)
{
    // FIXME: path
    // FIXME: different button types
    this->button_icon = cairo_image_surface_create_from_png(
        "/home/ilex/work/wayfire/plugins/decor/resources/close.png");
    update_texture();
}

void button_t::set_hover(bool is_hovered)
{
    // TODO
}

bool button_t::needs_repaint()
{
    return true;
}

void button_t::render(const wf_framebuffer& fb, wf_geometry geometry,
    wf_geometry scissor)
{
    assert(this->button_texture != uint32_t(-1));

    OpenGL::render_begin(fb);
    fb.scissor(scissor);

    gl_geometry gg;
    gg.x1 = geometry.x + fb.geometry.x;
    gg.y1 = geometry.y + fb.geometry.y;
    gg.x2 = gg.x1 + geometry.width;
    gg.y2 = gg.y1 + geometry.height;

    OpenGL::render_transformed_texture(button_texture, gg, {},
        fb.get_orthographic_projection(), {1, 1, 1, 1},
        TEXTURE_TRANSFORM_INVERT_Y);

    OpenGL::render_end();
}

void button_t::update_texture()
{
    if (!button_icon)
        return;

    // XXX: we render at a predefined resolution here ...
    const int WIDTH = 60;
    const int HEIGHT = 30;

    if (!this->button_surface)
    {
        this->button_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
            WIDTH, HEIGHT);
        this->cr = cairo_create(this->button_surface);
    }

    /* Clear the button background */
    cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_fill(cr);

    /* Render button itself */
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
    cairo_scale(cr, 1.0 * WIDTH / cairo_image_surface_get_width(button_icon),
        1.0 * HEIGHT / cairo_image_surface_get_height(button_icon));
    cairo_set_source_surface(cr, this->button_icon, 0, 0);
    cairo_fill(cr);

    /* Upload to GPU */
    cairo_surface_upload_to_texture(this->button_surface,
        this->button_texture);
}

}
}
