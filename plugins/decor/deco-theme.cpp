#include "deco-theme.hpp"
#include <core.hpp>
#include <config.hpp>
#include <opengl.hpp>
#include <config.h>

extern "C"
{
#define static
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#undef static
}

namespace wf
{
namespace decor
{
/** Create a new theme with the default parameters */
decoration_theme_t::decoration_theme_t()
{
    auto section = wf::get_core().config->get_section("decoration");

    font_opt       = section->get_option("font", "serif");
    title_height   = new_static_option("50");
    border_size    = new_static_option("5");
    active_color   = new_static_option("0.15 0.15 0.15 0.8");
    inactive_color = new_static_option("0.25 0.25 0.25 0.95");
}

/** @return The available height for displaying the title */
int decoration_theme_t::get_title_height() const
{
    return title_height->as_cached_int();
}

/** @return The available border for resizing */
int decoration_theme_t::get_border_size() const
{
    return border_size->as_cached_int();
}

/**
 * Fill the given rectange with the background color(s).
 *
 * @param fb The target framebuffer, must have been bound already
 * @param rectangle The rectangle to redraw.
 * @param scissor The GL scissor rectangle to use.
 * @param active Whether to use active or inactive colors
 */
void decoration_theme_t::render_background(const wf_framebuffer& fb,
    wf_geometry rectangle, const wf_geometry& scissor, bool active) const
{
    /* Prepare matrices */
    rectangle = fb.damage_box_from_geometry_box(rectangle);

    float projection[9];
    wlr_matrix_projection(projection,
        fb.viewport_width, fb.viewport_height,
        (wl_output_transform)fb.wl_transform);

    float matrix[9];
    wlr_matrix_project_box(matrix, &rectangle,
        WL_OUTPUT_TRANSFORM_NORMAL, 0, projection);

    /* Calculate color */
    auto color = active ?
        active_color->as_cached_color() : inactive_color->as_cached_color();
    float color4f[] = {color.r, color.g, color.b, color.a};

    /* Actual rendering */
    OpenGL::render_begin(fb);
    fb.scissor(scissor);
    wlr_render_quad_with_matrix(wf::get_core().renderer, color4f, matrix);
    OpenGL::render_end();
}

/**
 * Render the given text on a cairo_surface_t with the given size.
 * The caller is responsible for freeing the memory afterwards.
 */
cairo_surface_t *decoration_theme_t::render_text(std::string text,
    int width, int height) const
{
    const auto format = CAIRO_FORMAT_ARGB32;
    auto surface = cairo_image_surface_create(format, width, height);
    auto cr = cairo_create(surface);

    const float font_scale = 0.8;
    const float font_size = height * font_scale;

    // render text
    cairo_select_font_face(cr, font_opt->as_string().c_str(),
        CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_source_rgba(cr, 1, 1, 1, 1);

    cairo_set_font_size(cr, font_size);
    cairo_move_to(cr, 0, font_size);

    cairo_text_extents_t ext;
    cairo_text_extents(cr, text.c_str(), &ext);
    cairo_show_text(cr, text.c_str());
    cairo_destroy(cr);

    return surface;
}

/**
 * Get the icon for the given button.
 * The caller is responsible for freeing the memory afterwards.
 *
 * @param button The button type.
 * @param width The width of the generated surface.
 * @param height The height of the generated surface.
 */
cairo_surface_t *decoration_theme_t::get_button_surface(button_type_t button,
    int width, int height) const
{
    cairo_surface_t *button_icon;
    switch (button)
    {
        case BUTTON_CLOSE:
            button_icon = cairo_image_surface_create_from_png(
                INSTALL_PREFIX "/share/wayfire/decoration/resources/close.png");
            break;
        default:
            assert(false);
    }

    cairo_surface_t *button_surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    auto cr = cairo_create(button_surface);

    /* Clear the button background */
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_fill(cr);

    /* Render button itself */
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_scale(cr, 1.0 * width / cairo_image_surface_get_width(button_icon),
        1.0 * height / cairo_image_surface_get_height(button_icon));
    cairo_set_source_surface(cr, button_icon, 0, 0);
    cairo_fill(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(button_icon);

    return button_surface;
}

}
}
