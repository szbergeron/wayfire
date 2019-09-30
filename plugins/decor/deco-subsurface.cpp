#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include <linux/input-event-codes.h>

#include <compositor-surface.hpp>
#include <output.hpp>
#include <opengl.hpp>
#include <core.hpp>
#include <debug.hpp>
#include <decorator.hpp>
#include <view-transform.hpp>
#include <signal-definitions.hpp>
#include "deco-subsurface.hpp"
#include "deco-layout.hpp"
#include "cairo-util.hpp"

#include <cairo.h>

extern "C"
{
#define static
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_xcursor_manager.h>
#undef static
}

const int titlebar_thickness = 100;
const int resize_edge_threshold = 5;
const int normal_thickness = resize_edge_threshold;

GLuint get_text_texture(int width, int height,
    std::string text, std::string cairo_font)
{
    const auto format = CAIRO_FORMAT_ARGB32;
    auto surface = cairo_image_surface_create(format, width, height);
    auto cr = cairo_create(surface);

    const float font_scale = 0.8;
    const float font_size = height * font_scale;

    // render text
    cairo_select_font_face(cr, cairo_font.c_str(), CAIRO_FONT_SLANT_NORMAL,
        CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_source_rgba(cr, 1, 1, 1, 1);

    cairo_set_font_size(cr, font_size);

    cairo_text_extents_t ext;
    cairo_text_extents(cr, text.c_str(), &ext);

    cairo_move_to(cr, normal_thickness, font_size);
    cairo_show_text(cr, text.c_str());

    cairo_destroy(cr);

    GLuint tex = -1;
    cairo_surface_upload_to_texture(surface, tex);
    cairo_surface_destroy(surface);

    return tex;
}

class simple_decoration_surface : public wf::surface_interface_t,
    public wf::compositor_surface_t, public wf_decorator_frame_t
{
    bool _mapped = true;
    int thickness = normal_thickness;
    int titlebar = titlebar_thickness;

    wayfire_view view;
    wf_option font_option;
    wf::signal_callback_t title_set;

    int width = 100, height = 100;

    bool active = true; // when views are mapped, they are usually activated
    float border_color[4] = {0.15f, 0.15f, 0.15f, 0.8f};
    float border_color_inactive[4] = {0.25f, 0.25f, 0.25f, 0.95f};

    GLuint tex = -1;

    wf::decor::decoration_layout_t layout;
    wf_region cached_region;

  public:
    simple_decoration_surface(wayfire_view view, wf_option font) :
        surface_interface_t(view.get()),
        layout(titlebar_thickness - resize_edge_threshold, resize_edge_threshold)
    {
        this->font_option = font;
        this->view = view;

        title_set = [=] (wf::signal_data_t *data)
        {
            if (get_signaled_view(data) == view)
                notify_view_resized(view->get_wm_geometry());
        };
        view->connect_signal("title-changed", &title_set);

        // make sure to hide frame if the view is fullscreen
        update_decoration_size();
    }


    virtual ~simple_decoration_surface()
    {
        _mapped = false;
        wf::emit_map_state_change(this);
        view->disconnect_signal("title-changed", &title_set);
    }

    /* wf::surface_interface_t implementation */
    virtual bool is_mapped() const final
    {
        return _mapped;
    }

    wf_point get_offset() final
    {
        return { -thickness, -titlebar };
    }

    virtual wf_size_t get_size() const final
    {
        return {width, height};
    }

    void render_background(const wf_framebuffer& fb,
        wf_geometry geometry)
    {
        float projection[9];
        wlr_matrix_projection(projection,
            fb.viewport_width, fb.viewport_height,
            (wl_output_transform)fb.wl_transform);

        float matrix[9];
        wlr_matrix_project_box(matrix, &geometry,
            WL_OUTPUT_TRANSFORM_NORMAL, 0, projection);

        wlr_render_quad_with_matrix(wf::get_core().renderer,
            active ? border_color : border_color_inactive, matrix);
    }

    void render_title(const wf_framebuffer& fb,
        wf_geometry geometry)
    {
        if (tex == (uint)-1)
        {
            tex = get_text_texture(width * fb.scale, titlebar * fb.scale,
                view->get_title(), font_option->as_string());
        }

        gl_geometry gg;
        gg.x1 = geometry.x + fb.geometry.x;
        gg.y1 = geometry.y + fb.geometry.y;
        gg.x2 = gg.x1 + geometry.width;
        gg.y2 = gg.y1 + geometry.height;

        OpenGL::render_transformed_texture(tex, gg, {},
            fb.get_orthographic_projection(), {1, 1, 1, 1},
            TEXTURE_TRANSFORM_INVERT_Y);
    }

    void render_scissor_box(const wf_framebuffer& fb, wf_point origin,
        const wlr_box& scissor)
    {
        wlr_box geometry {origin.x, origin.y, width, height};
        geometry = fb.damage_box_from_geometry_box(geometry);
        auto renderables = layout.get_renderable_areas();

        OpenGL::render_begin(fb);
        fb.scissor(scissor);
        render_background(fb, geometry);
        OpenGL::render_end();

        for (auto item : renderables)
        {
            if (item->get_type() == wf::decor::DECORATION_AREA_TITLE) {
                OpenGL::render_begin(fb);
                fb.scissor(scissor);
                render_title(fb, item->get_geometry() + origin);
                OpenGL::render_end();
            } else { // button
                item->as_button().render(fb,
                    item->get_geometry() + origin, scissor);
            }
        }
    }

    virtual void simple_render(const wf_framebuffer& fb, int x, int y,
        const wf_region& damage) override
    {
        wf_region frame = this->cached_region + wf_point{x, y};
        frame *= fb.scale;
        frame &= damage;

        for (const auto& box : frame)
        {
            auto sbox = fb.framebuffer_box_from_damage_box(
                wlr_box_from_pixman_box(box));
            render_scissor_box(fb, {x, y}, sbox);
        }
    }

    bool accepts_input(int32_t sx, int32_t sy) override
    {
        return pixman_region32_contains_point(cached_region.to_pixman(),
            sx, sy, NULL);
    }

    /* wf::compositor_surface_t implementation */
    virtual void on_pointer_enter(int x, int y) override
    {
        cursor_x = x;
        cursor_y = y;

        update_cursor();
    }

    virtual void on_pointer_leave() override
    { }

    int cursor_x, cursor_y;
    virtual void on_pointer_motion(int x, int y) override
    {
        cursor_x = x;
        cursor_y = y;

        update_cursor();
    }

    void send_move_request()
    {
        move_request_signal move_request;
        move_request.view = view;
        get_output()->emit_signal("move-request", &move_request);
    }

    void send_resize_request(int x, int y)
    {
        resize_request_signal resize_request;
        resize_request.view = view;
        resize_request.edges = get_edges(x, y);
        get_output()->emit_signal("resize-request", &resize_request);
    }

    uint32_t get_edges(int x, int y)
    {
        uint32_t edges = 0;
        if (x <= thickness)
            edges |= WLR_EDGE_LEFT;
        if (x >= width - thickness)
            edges |= WLR_EDGE_RIGHT;
        if (y <= thickness)
            edges |= WLR_EDGE_TOP;
        if (y >= height - thickness)
            edges |= WLR_EDGE_BOTTOM;

        return edges;
    }

    std::string get_cursor(uint32_t edges)
    {
        if (edges)
            return wlr_xcursor_get_resize_name((wlr_edges) edges);
        return "default";
    }

    void update_cursor()
    {
        wf::get_core().set_cursor(get_cursor(get_edges(cursor_x, cursor_y)));
    }

    virtual void on_pointer_button(uint32_t button, uint32_t state) override
    {
        if (button != BTN_LEFT || state != WLR_BUTTON_PRESSED)
            return;

        if (get_edges(cursor_x, cursor_y))
            return send_resize_request(cursor_x, cursor_y);
        send_move_request();
    }

    virtual void on_touch_down(int x, int y) override
    {
        if (get_edges(x, y))
            return send_resize_request(x, y);

        send_move_request();
    }

    /* frame implementation */
    virtual wf_geometry expand_wm_geometry(
        wf_geometry contained_wm_geometry) override
    {
        contained_wm_geometry.x -= thickness;
        contained_wm_geometry.y -= titlebar;
        contained_wm_geometry.width += 2 * thickness;
        contained_wm_geometry.height += thickness + titlebar;

        return contained_wm_geometry;
    }

    virtual void calculate_resize_size(
        int& target_width, int& target_height) override
    {
        target_width -= 2 * thickness;
        target_height -= thickness + titlebar;

        target_width = std::max(target_width, 1);
        target_height = std::max(target_height, 1);
    }

    virtual void notify_view_activated(bool active) override
    {
        if (this->active != active)
            view->damage();

        this->active = active;
    }

    virtual void notify_view_resized(wf_geometry view_geometry) override
    {
        view->damage();

        if (tex != (uint32_t)-1)
        {
            GL_CALL(glDeleteTextures(1, &tex));
        }

        tex = -1;
        width = view_geometry.width;
        height = view_geometry.height;

        layout.resize(width, height);
        this->cached_region = layout.calculate_region();

        view->damage();
    };

    virtual void notify_view_tiled() override
    { }

    void update_decoration_size()
    {
        if (view->fullscreen)
        {
            thickness = 0;
            titlebar = 0;
        } else
        {
            thickness = normal_thickness;
            titlebar = titlebar_thickness;
        }
        this->cached_region = layout.calculate_region();
    }

    virtual void notify_view_fullscreen() override
    {
        update_decoration_size();
    };
};

void init_view(wayfire_view view, wf_option font)
{
    auto surf = new simple_decoration_surface(view, font);
    view->set_decoration(surf);
    view->damage();
}
