#include <nanogui/nanogui.h>
#include <GLFW/glfw3.h>
#include "Area.h"
#include <fmt/core.h>

constexpr float Pi = 3.14159f;

class WaveFormCanvas : public nanogui::Canvas {
public:
    WaveFormCanvas(Widget *parent, Area *area_) : Canvas(parent, 1), m_rotation(0.5f) {
        using namespace nanogui;

        if(area_ == nullptr)
            return;
        setArea(area_);
    }

    void set_progress(int n) {
        progress = count - n;
    }

    void setArea(Area *area_) {
        using namespace nanogui;
        count = (area_->end - area_->ptr) / area_->step;
        m_shader = new Shader(
            render_pass(),

            // An identifying name
            "a_waveform_shader",

            // Vertex shader
            // mvp - camera
            // position - vertexes
            // colors - vertexes
            R"(#version 330
            uniform mat4 mvp;
            in vec3 position;
            in vec3 color;
            out vec4 frag_color;
            void main() {
                frag_color = vec4(color, 1.0);
                gl_Position = mvp * vec4(position, 1.0);
            })",

            // Fragment shader
            R"(#version 330
            out vec4 color;
            in vec4 frag_color;
            void main() {
                color = frag_color;
            })"
        );

        constexpr int pos_size = 3;
        float *positions = new float[pos_size * 2 * count];
        constexpr int col_size = 3;
        colors = new float[col_size * 2 * count];
        float color[col_size * 2 * 1] = {1, 0, 0, 1, 0, 0};
        wave = area_;
        Area area = *area_;
        

        float step = 2.f / count;
        int i = 0;
        while(area.ptr < area.end && i < count) {
            float val = *(area++);
            int index = i * 6;
            *(positions + index + 0) = -1 + step * i;
            *(positions + index + 1) = 0.f;
            *(positions + index + 2) = 0.f;
            *(positions + index + 3) = -1 + step * i;
            *(positions + index + 4) = val;
            *(positions + index + 5) = 0.f;

            memcpy((void*)(colors + index), (void*)&color, 6*sizeof(float));
            ++i;
        }

        m_shader->set_buffer("position", VariableType::Float32, {(size_t)2 * count, pos_size}, positions);
        m_shader->set_buffer("color", VariableType::Float32, {(size_t)2 * count, col_size}, colors);
    }
    
    virtual void draw_contents() override {
        if(m_shader == nullptr || wave == nullptr) return;

        using namespace nanogui;

        // all variables in theese three matrix are just picking
        Matrix4f view = Matrix4f::look_at(
            Vector3f(0, 0, zoom),
            Vector3f(posx, 0, 0),
            Vector3f(0, 1, 0)
        );

        Matrix4f proj = Matrix4f::perspective(
            float(25 * Pi / 180),
            0.00001f,
            50.f,
            (m_size.x() / (float) m_size.y()) * std::max(posy, 0.00001f) 
        );

        constexpr int col_size = 3;
        Area area = *wave;

        int i = 0;
        float color_bef[col_size * 2 * 1] = {1, 0, 0, 1, 0, 0};
        float color_aft[col_size * 2 * 1] = {0, 1, 0, 0, 1, 0};
        while(i * 6 < col_size * 2 * count) {
            int index = i * 6;

            if(i * 6 < col_size * 2 * progress)
                memcpy((void*)(colors + index), (void*)&color_aft, 6*sizeof(float));
            else
                memcpy((void*)(colors + index), (void*)&color_bef, 6*sizeof(float));
            ++i;
        }

        m_shader->set_buffer("color", VariableType::Float32, {(size_t)2 * count, col_size}, colors);
        

        Matrix4f mvp = proj * view;

        m_shader->set_uniform("mvp", mvp);

        m_shader->begin();
        m_shader->draw_array(Shader::PrimitiveType::Line, 0, 2 * count, false);
        m_shader->end();
    }

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) {
        if (key == GLFW_KEY_LEFT_SHIFT ) {
            if(action == GLFW_PRESS)
                zoomFactor = 0.5;
            else if(action == GLFW_RELEASE)
                zoomFactor = 0.9;
        }
        if (key == GLFW_KEY_RIGHT_SHIFT ) {
            if(action == GLFW_PRESS)
                posx += 0.1;
        }
        return true;
    }

bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down,
                                       int modifiers) 
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if(down) {
                m_mouse_down_pos_x = p.x();
                posx_down = posx;
                m_mouse_down_pos_y = p.y();
                posy_down = posy;
                drag = true;
            }
            else {
                drag = false;
            }

        }
        return true;
    }

virtual bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button,
                                  int modifiers) override 
    {
        if (button == 1) {
            float value_delta_x = ((p.x() - m_mouse_down_pos_x) / float(10));
            float value_delta_y = ((p.y() - m_mouse_down_pos_y) / float(10));
            posx = posx_down - 0.00001 * (0.5+zoom) * (0.5+zoom)* (0.5+zoom) * value_delta_x;
            posy = posy_down - 0.001 * value_delta_y;
        }
        return true;
    }

    virtual bool scroll_event(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) {
        if(rel.y() > 0)
            zoom *= zoomFactor;
        else
            zoom /= zoomFactor;
        return true;
    }

private:
    nanogui::ref<nanogui::Shader> m_shader = nullptr;
    float m_rotation;
    Area *wave = nullptr;
    int count;
    int progress;
    float *colors;

    float zoom = 9.f;
    float zoomFactor = 0.9f;
    float posx = 0.f;
    float posx_down = 0.f;

    float posy = 0.164f;
    float posy_down = 0.f;
    bool drag = false;
    float m_mouse_down_pos_x;
    float m_mouse_down_pos_y;
};