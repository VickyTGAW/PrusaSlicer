#include "GLCanvas3DManager.hpp"
#include "../../slic3r/GUI/GUI.hpp"
#include "../../slic3r/GUI/AppConfig.hpp"

#include <GL/glew.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <wx/glcanvas.h>

#include <vector>
#include <string>
#include <iostream>

namespace Slic3r {
namespace GUI {

GLCanvas3DManager::GLVersion::GLVersion()
    : vn_major(0)
    , vn_minor(0)
{
}

bool GLCanvas3DManager::GLVersion::detect()
{
    const char* gl_version = (const char*)::glGetString(GL_VERSION);
    if (gl_version == nullptr)
        return false;

    std::vector<std::string> tokens;
    boost::split(tokens, gl_version, boost::is_any_of(" "), boost::token_compress_on);

    if (tokens.empty())
        return false;

    std::vector<std::string> numbers;
    boost::split(numbers, tokens[0], boost::is_any_of("."), boost::token_compress_on);

    if (numbers.size() > 0)
        vn_major = ::atoi(numbers[0].c_str());

    if (numbers.size() > 1)
        vn_minor = ::atoi(numbers[1].c_str());

    return true;
}

bool GLCanvas3DManager::GLVersion::is_greater_or_equal_to(unsigned int major, unsigned int minor) const
{
    if (vn_major < major)
        return false;
    else if (vn_major > major)
        return true;
    else
        return vn_minor >= minor;
}

GLCanvas3DManager::GLCanvas3DManager()
    : m_gl_initialized(false)
    , m_use_legacy_opengl(false)
    , m_use_VBOs(false)
{
}

bool GLCanvas3DManager::add(wxGLCanvas* canvas, wxGLContext* context)
{
    if (_get_canvas(canvas) != m_canvases.end())
        return false;

    GLCanvas3D* canvas3D = new GLCanvas3D(canvas, context);
    if (canvas3D == nullptr)
        return false;

    canvas->Bind(wxEVT_SIZE, [canvas3D](wxSizeEvent& evt) { canvas3D->on_size(evt); });
    canvas->Bind(wxEVT_IDLE, [canvas3D](wxIdleEvent& evt) { canvas3D->on_idle(evt); });
    canvas->Bind(wxEVT_CHAR, [canvas3D](wxKeyEvent& evt)  { canvas3D->on_char(evt); });

    m_canvases.insert(CanvasesMap::value_type(canvas, canvas3D));

    std::cout << "canvas added: " << (void*)canvas << " (" << (void*)canvas3D << ")" << std::endl;

    return true;
}

bool GLCanvas3DManager::remove(wxGLCanvas* canvas)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it == m_canvases.end())
        return false;

    delete it->second;
    m_canvases.erase(it);

    std::cout << "canvas removed: " << (void*)canvas << std::endl;

    return true;
}

void GLCanvas3DManager::remove_all()
{
    for (CanvasesMap::value_type& item : m_canvases)
    {
        std::cout << "canvas removed: " << (void*)item.second << std::endl;
        delete item.second;
    }
    m_canvases.clear();
}

unsigned int GLCanvas3DManager::count() const
{
    return (unsigned int)m_canvases.size();
}

void GLCanvas3DManager::init_gl()
{
    if (!m_gl_initialized)
    {
        std::cout << "GLCanvas3DManager::init_gl()" << std::endl;

        glewInit();
        m_gl_version.detect();

        const AppConfig* config = GUI::get_app_config();
        m_use_legacy_opengl = (config == nullptr) || (config->get("use_legacy_opengl") == "1");
        m_use_VBOs = !m_use_legacy_opengl && m_gl_version.is_greater_or_equal_to(2, 0);
        m_gl_initialized = true;

        std::cout << "DETECTED OPENGL: " << m_gl_version.vn_major << "." << m_gl_version.vn_minor << std::endl;
        std::cout << "USE VBOS = " << (m_use_VBOs ? "YES" : "NO") << std::endl;
        std::cout << "LAYER EDITING ALLOWED = " << (!m_use_legacy_opengl ? "YES" : "NO") << std::endl;
    }
}

bool GLCanvas3DManager::use_VBOs() const
{
    return m_use_VBOs;
}

bool GLCanvas3DManager::init(wxGLCanvas* canvas, bool useVBOs)
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->init(useVBOs, m_use_legacy_opengl) : false;
}

bool GLCanvas3DManager::is_dirty(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->is_dirty() : false;
}

void GLCanvas3DManager::set_dirty(wxGLCanvas* canvas, bool dirty)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_dirty(dirty);
}

bool GLCanvas3DManager::is_shown_on_screen(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->is_shown_on_screen() : false;
}

void GLCanvas3DManager::resize(wxGLCanvas* canvas, unsigned int w, unsigned int h)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->resize(w, h);
}

GLVolumeCollection* GLCanvas3DManager::get_volumes(wxGLCanvas* canvas)
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_volumes() : nullptr;
}

void GLCanvas3DManager::set_volumes(wxGLCanvas* canvas, GLVolumeCollection* volumes)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_volumes(volumes);
}

void GLCanvas3DManager::reset_volumes(wxGLCanvas* canvas)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->reset_volumes();
}

void GLCanvas3DManager::deselect_volumes(wxGLCanvas* canvas)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->deselect_volumes();
}

void GLCanvas3DManager::select_volume(wxGLCanvas* canvas, unsigned int id)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->select_volume(id);
}

DynamicPrintConfig* GLCanvas3DManager::get_config(wxGLCanvas* canvas)
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_config() : nullptr;
}

void GLCanvas3DManager::set_config(wxGLCanvas* canvas, DynamicPrintConfig* config)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_config(config);
}

void GLCanvas3DManager::set_bed_shape(wxGLCanvas* canvas, const Pointfs& shape)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_bed_shape(shape);
}

void GLCanvas3DManager::set_auto_bed_shape(wxGLCanvas* canvas)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_auto_bed_shape();
}

BoundingBoxf3 GLCanvas3DManager::get_bed_bounding_box(wxGLCanvas* canvas)
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->bed_bounding_box() : BoundingBoxf3();
}

BoundingBoxf3 GLCanvas3DManager::get_volumes_bounding_box(wxGLCanvas* canvas)
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->volumes_bounding_box() : BoundingBoxf3();
}

BoundingBoxf3 GLCanvas3DManager::get_max_bounding_box(wxGLCanvas* canvas)
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->max_bounding_box() : BoundingBoxf3();
}

Pointf3 GLCanvas3DManager::get_axes_origin(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_axes_origin() : Pointf3();
}

void GLCanvas3DManager::set_axes_origin(wxGLCanvas* canvas, const Pointf3& origin)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_axes_origin(origin);
}

float GLCanvas3DManager::get_axes_length(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_axes_length() : 0.0f;
}

void GLCanvas3DManager::set_axes_length(wxGLCanvas* canvas, float length)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_axes_length(length);
}

void GLCanvas3DManager::set_cutting_plane(wxGLCanvas* canvas, float z, const ExPolygons& polygons)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_cutting_plane(z, polygons);
}

unsigned int GLCanvas3DManager::get_camera_type(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? (unsigned int)it->second->get_camera_type() : 0;
}

void GLCanvas3DManager::set_camera_type(wxGLCanvas* canvas, unsigned int type)
{
    if ((type <= (unsigned int)GLCanvas3D::Camera::CT_Unknown) || ((unsigned int)GLCanvas3D::Camera::CT_Count <= type))
        return;

    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_camera_type((GLCanvas3D::Camera::EType)type);
}

std::string GLCanvas3DManager::get_camera_type_as_string(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_camera_type_as_string() : "unknown";
}

float GLCanvas3DManager::get_camera_zoom(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_camera_zoom() : 1.0f;
}

void GLCanvas3DManager::set_camera_zoom(wxGLCanvas* canvas, float zoom)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_camera_zoom(zoom);
}

float GLCanvas3DManager::get_camera_phi(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_camera_phi() : 0.0f;
}

void GLCanvas3DManager::set_camera_phi(wxGLCanvas* canvas, float phi)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_camera_phi(phi);
}

float GLCanvas3DManager::get_camera_theta(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_camera_theta() : 0.0f;
}

void GLCanvas3DManager::set_camera_theta(wxGLCanvas* canvas, float theta)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_camera_theta(theta);
}

float GLCanvas3DManager::get_camera_distance(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_camera_distance() : 0.0f;
}

void GLCanvas3DManager::set_camera_distance(wxGLCanvas* canvas, float distance)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_camera_distance(distance);
}

Pointf3 GLCanvas3DManager::get_camera_target(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_camera_target() : Pointf3(0.0, 0.0, 0.0);
}

void GLCanvas3DManager::set_camera_target(wxGLCanvas* canvas, const Pointf3& target)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_camera_target(target);
}

bool GLCanvas3DManager::is_layers_editing_enabled(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->is_layers_editing_enabled() : false;
}

bool GLCanvas3DManager::is_picking_enabled(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->is_picking_enabled() : false;
}

bool GLCanvas3DManager::is_layers_editing_allowed(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->is_layers_editing_allowed() : false;
}

bool GLCanvas3DManager::is_multisample_allowed(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->is_multisample_allowed() : false;
}

void GLCanvas3DManager::enable_layers_editing(wxGLCanvas* canvas, bool enable)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->enable_layers_editing(enable);
}

void GLCanvas3DManager::enable_warning_texture(wxGLCanvas* canvas, bool enable)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->enable_warning_texture(enable);
}

void GLCanvas3DManager::enable_legend_texture(wxGLCanvas* canvas, bool enable)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->enable_legend_texture(enable);
}

void GLCanvas3DManager::enable_picking(wxGLCanvas* canvas, bool enable)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->enable_picking(enable);
}

void GLCanvas3DManager::enable_shader(wxGLCanvas* canvas, bool enable)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->enable_shader(enable);
}

void GLCanvas3DManager::allow_multisample(wxGLCanvas* canvas, bool allow)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->allow_multisample(allow);
}

bool GLCanvas3DManager::is_mouse_dragging(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->is_mouse_dragging() : false;
}

void GLCanvas3DManager::set_mouse_dragging(wxGLCanvas* canvas, bool dragging)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_mouse_dragging(dragging);
}

Pointf GLCanvas3DManager::get_mouse_position(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_mouse_position() : Pointf();
}

void GLCanvas3DManager::set_mouse_position(wxGLCanvas* canvas, const Pointf& position)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_mouse_position(position);
}

int GLCanvas3DManager::get_hover_volume_id(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_hover_volume_id() : -1;
}

void GLCanvas3DManager::set_hover_volume_id(wxGLCanvas* canvas, int id)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_hover_volume_id(id);
}

unsigned int GLCanvas3DManager::get_layers_editing_z_texture_id(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_layers_editing_z_texture_id() : 0;
}

float GLCanvas3DManager::get_layers_editing_band_width(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_layers_editing_band_width() : 0.0f;
}

void GLCanvas3DManager::set_layers_editing_band_width(wxGLCanvas* canvas, float band_width)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_layers_editing_band_width(band_width);
}

float GLCanvas3DManager::get_layers_editing_strength(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_layers_editing_strength() : 0.0f;
}

void GLCanvas3DManager::set_layers_editing_strength(wxGLCanvas* canvas, float strength)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_layers_editing_strength(strength);
}

int GLCanvas3DManager::get_layers_editing_last_object_id(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_layers_editing_last_object_id() : -1;
}

void GLCanvas3DManager::set_layers_editing_last_object_id(wxGLCanvas* canvas, int id)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_layers_editing_last_object_id(id);
}

float GLCanvas3DManager::get_layers_editing_last_z(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_layers_editing_last_z() : 0.0f;
}

void GLCanvas3DManager::set_layers_editing_last_z(wxGLCanvas* canvas, float z)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_layers_editing_last_z(z);
}

unsigned int GLCanvas3DManager::get_layers_editing_last_action(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_layers_editing_last_action() : 0;
}

void GLCanvas3DManager::set_layers_editing_last_action(wxGLCanvas* canvas, unsigned int action)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->set_layers_editing_last_action(action);
}

GLShader* GLCanvas3DManager::get_layers_editing_shader(wxGLCanvas* canvas)
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_layers_editing_shader() : nullptr;
}

float GLCanvas3DManager::get_layers_editing_cursor_z_relative(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_layers_editing_cursor_z_relative(*it->second) : 0.0f;
}

int GLCanvas3DManager::get_layers_editing_first_selected_object_id(wxGLCanvas* canvas, unsigned int objects_count) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->get_layers_editing_first_selected_object_id(objects_count) : 0.;
}

void GLCanvas3DManager::zoom_to_bed(wxGLCanvas* canvas)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->zoom_to_bed();
}

void GLCanvas3DManager::zoom_to_volumes(wxGLCanvas* canvas)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->zoom_to_volumes();
}

void GLCanvas3DManager::select_view(wxGLCanvas* canvas, const std::string& direction)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->select_view(direction);
}

bool GLCanvas3DManager::start_using_shader(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    return (it != m_canvases.end()) ? it->second->start_using_shader() : false;
}

void GLCanvas3DManager::stop_using_shader(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->stop_using_shader();
}

void GLCanvas3DManager::picking_pass(wxGLCanvas* canvas)
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->picking_pass();
}

void GLCanvas3DManager::render_background(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->render_background();
}

void GLCanvas3DManager::render_bed(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->render_bed();
}

void GLCanvas3DManager::render_axes(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->render_axes();
}

void GLCanvas3DManager::render_volumes(wxGLCanvas* canvas, bool fake_colors) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->render_volumes(fake_colors);
}

void GLCanvas3DManager::render_objects(wxGLCanvas* canvas, bool useVBOs)
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->render_objects(useVBOs);
}

void GLCanvas3DManager::render_cutting_plane(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->render_cutting_plane();
}

void GLCanvas3DManager::render_warning_texture(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->render_warning_texture();
}

void GLCanvas3DManager::render_legend_texture(wxGLCanvas* canvas) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->render_legend_texture();
}

void GLCanvas3DManager::render_layer_editing_overlay(wxGLCanvas* canvas, const Print& print) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->render_layer_editing_overlay(print);
}

void GLCanvas3DManager::render_texture(wxGLCanvas* canvas, unsigned int tex_id, float left, float right, float bottom, float top) const
{
    CanvasesMap::const_iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->render_texture(tex_id, left, right, bottom, top);
}

void GLCanvas3DManager::register_on_viewport_changed_callback(wxGLCanvas* canvas, void* callback)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->register_on_viewport_changed_callback(callback);
}

void GLCanvas3DManager::register_on_mark_volumes_for_layer_height_callback(wxGLCanvas* canvas, void* callback)
{
    CanvasesMap::iterator it = _get_canvas(canvas);
    if (it != m_canvases.end())
        it->second->register_on_mark_volumes_for_layer_height_callback(callback);
}

GLCanvas3DManager::CanvasesMap::iterator GLCanvas3DManager::_get_canvas(wxGLCanvas* canvas)
{
    return (canvas == nullptr) ? m_canvases.end() : m_canvases.find(canvas);
}

GLCanvas3DManager::CanvasesMap::const_iterator GLCanvas3DManager::_get_canvas(wxGLCanvas* canvas) const
{
    return (canvas == nullptr) ? m_canvases.end() : m_canvases.find(canvas);
}

} // namespace GUI
} // namespace Slic3r
