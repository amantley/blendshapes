//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <mutex>

#include <gpu/gl/GLBackend.h>

#include <QLoggingCategory>
#include <QResizeEvent>
#include <QTimer>
#include <QWindow>
#include <QElapsedTimer>
#include <QDir>
#include <QGuiApplication>

#include <gl/GLHelpers.h>
#include <gl/GLShaders.h>

#include <gl/QOpenGLContextWrapper.h>

#include <render-utils/simple_vert.h>
#include <render-utils/simple_frag.h>
#include <render-utils/simple_textured_frag.h>
#include <render-utils/simple_textured_unlit_frag.h>

#include <render-utils/deferred_light_vert.h>
#include <render-utils/deferred_light_point_vert.h>
#include <render-utils/deferred_light_spot_vert.h>

#include <render-utils/directional_ambient_light_frag.h>
#include <render-utils/directional_skybox_light_frag.h>

#include <render-utils/standardTransformPNTC_vert.h>
#include <render-utils/standardDrawTexture_frag.h>

#include <render-utils/model_vert.h>
#include <render-utils/model_shadow_vert.h>
#include <render-utils/model_normal_map_vert.h>
#include <render-utils/model_lightmap_vert.h>
#include <render-utils/model_lightmap_normal_map_vert.h>
#include <render-utils/skin_model_vert.h>
#include <render-utils/skin_model_shadow_vert.h>
#include <render-utils/skin_model_normal_map_vert.h>

#include <render-utils/model_frag.h>
#include <render-utils/model_shadow_frag.h>
#include <render-utils/model_normal_map_frag.h>
#include <render-utils/model_lightmap_frag.h>
#include <render-utils/model_lightmap_normal_map_frag.h>
#include <render-utils/model_translucent_frag.h>

#include <entities-renderer/textured_particle_frag.h>
#include <entities-renderer/textured_particle_vert.h>

#include <render-utils/overlay3D_vert.h>
#include <render-utils/overlay3D_frag.h>

#include <graphics/skybox_vert.h>
#include <graphics/skybox_frag.h>

#include <gpu/DrawTransformUnitQuad_vert.h>
#include <gpu/DrawTexcoordRectTransformUnitQuad_vert.h>
#include <gpu/DrawViewportQuadTransformTexcoord_vert.h>
#include <gpu/DrawTexture_frag.h>
#include <gpu/DrawTextureOpaque_frag.h>
#include <gpu/DrawColoredTexture_frag.h>

#include <render-utils/sdf_text3D_vert.h>
#include <render-utils/sdf_text3D_frag.h>

#include <entities-renderer/paintStroke_vert.h>
#include <entities-renderer/paintStroke_frag.h>

#include <entities-renderer/polyvox_vert.h>
#include <entities-renderer/polyvox_frag.h>

// Create a simple OpenGL window that renders text in various ways
class QTestWindow : public QWindow {
    Q_OBJECT
    QOpenGLContextWrapper _context;

protected:
    void renderText();

public:
    QTestWindow() {
        setSurfaceType(QSurface::OpenGLSurface);
        QSurfaceFormat format = getDefaultOpenGLSurfaceFormat();
        setFormat(format);
        _context.setFormat(format);
        _context.create();

        show();
        makeCurrent();
        gl::initModuleGl();
        gpu::Context::init<gpu::gl::GLBackend>();
        makeCurrent();
        resize(QSize(800, 600));
    }

    virtual ~QTestWindow() {
    }

    void draw();
    void makeCurrent() {
        _context.makeCurrent(this);
    }
};



const std::string VERTEX_SHADER_DEFINES{ R"GLSL(
#version 410 core
#define GPU_VERTEX_SHADER
#define GPU_TRANSFORM_IS_STEREO
#define GPU_TRANSFORM_STEREO_CAMERA
#define GPU_TRANSFORM_STEREO_CAMERA_INSTANCED
#define GPU_TRANSFORM_STEREO_SPLIT_SCREEN
)GLSL" };

const std::string PIXEL_SHADER_DEFINES{ R"GLSL(
#version 410 core
#define GPU_PIXEL_SHADER
#define GPU_TRANSFORM_IS_STEREO
#define GPU_TRANSFORM_STEREO_CAMERA
#define GPU_TRANSFORM_STEREO_CAMERA_INSTANCED
#define GPU_TRANSFORM_STEREO_SPLIT_SCREEN
)GLSL" };

void testShaderBuild(const std::string& vs_src, const std::string& fs_src) {
    std::string error;
    GLuint vs, fs;
    if (!gl::compileShader(GL_VERTEX_SHADER, VERTEX_SHADER_DEFINES + vs_src, vs, error) ||
        !gl::compileShader(GL_FRAGMENT_SHADER, PIXEL_SHADER_DEFINES + fs_src, fs, error)) {
        throw std::runtime_error("Failed to compile shader");
    }
    gl::CachedShader binary;
    auto pr = gl::compileProgram({ vs, fs }, error, binary);
    if (!pr) {
        throw std::runtime_error("Failed to link shader");
    }
}

void QTestWindow::draw() {
    if (!isVisible()) {
        return;
    }

    makeCurrent();
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static std::once_flag once;
    std::call_once(once, [&]{
        testShaderBuild(sdf_text3D_vert::getSource(), sdf_text3D_frag::getSource());

        testShaderBuild(DrawTransformUnitQuad_vert::getSource(), DrawTexture_frag::getSource());
        testShaderBuild(DrawTexcoordRectTransformUnitQuad_vert::getSource(), DrawTexture_frag::getSource());
        testShaderBuild(DrawViewportQuadTransformTexcoord_vert::getSource(), DrawTexture_frag::getSource());
        testShaderBuild(DrawTransformUnitQuad_vert::getSource(), DrawTextureOpaque_frag::getSource());
        testShaderBuild(DrawTransformUnitQuad_vert::getSource(), DrawColoredTexture_frag::getSource());

        testShaderBuild(skybox_vert::getSource(), skybox_frag::getSource());
        testShaderBuild(simple_vert::getSource(), simple_frag::getSource());
        testShaderBuild(simple_vert::getSource(), simple_textured_frag::getSource());
        testShaderBuild(simple_vert::getSource(), simple_textured_unlit_frag::getSource());
        testShaderBuild(deferred_light_vert::getSource(), directional_ambient_light_frag::getSource());
        testShaderBuild(deferred_light_vert::getSource(), directional_skybox_light_frag::getSource());
        testShaderBuild(standardTransformPNTC_vert::getSource(), standardDrawTexture_frag::getSource());
        testShaderBuild(standardTransformPNTC_vert::getSource(), DrawTextureOpaque_frag::getSource());

        testShaderBuild(model_vert::getSource(), model_frag::getSource());
        testShaderBuild(model_normal_map_vert::getSource(), model_normal_map_frag::getSource());
        testShaderBuild(model_vert::getSource(), model_translucent_frag::getSource());
        testShaderBuild(model_normal_map_vert::getSource(), model_translucent_frag::getSource());
        testShaderBuild(model_lightmap_vert::getSource(), model_lightmap_frag::getSource());
        testShaderBuild(model_lightmap_normal_map_vert::getSource(), model_lightmap_normal_map_frag::getSource());

        testShaderBuild(skin_model_vert::getSource(), model_frag::getSource());
        testShaderBuild(skin_model_normal_map_vert::getSource(), model_normal_map_frag::getSource());
        testShaderBuild(skin_model_vert::getSource(), model_translucent_frag::getSource());
        testShaderBuild(skin_model_normal_map_vert::getSource(), model_translucent_frag::getSource());

        testShaderBuild(model_shadow_vert::getSource(), model_shadow_frag::getSource());
        testShaderBuild(textured_particle_vert::getSource(), textured_particle_frag::getSource());
/* FIXME: Bring back the ssao shader tests
        testShaderBuild(gaussian_blur_vert::getSource()ical_vert::getSource(), gaussian_blur_frag::getSource());
        testShaderBuild(gaussian_blur_horizontal_vert::getSource(), gaussian_blur_frag::getSource());
        testShaderBuild(ambient_occlusion_vert::getSource(), ambient_occlusion_frag::getSource());
        testShaderBuild(ambient_occlusion_vert::getSource(), occlusion_blend_frag::getSource());
*/

        testShaderBuild(overlay3D_vert::getSource(), overlay3D_frag::getSource());

        testShaderBuild(paintStroke_vert::getSource(),paintStroke_frag::getSource());
        testShaderBuild(polyvox_vert::getSource(), polyvox_frag::getSource());

    });
    _context.swapBuffers(this);
}

const char * LOG_FILTER_RULES = R"V0G0N(
hifi.gpu=true
)V0G0N";

int main(int argc, char** argv) {
    setupHifiApplication("Shaders Test");

    QGuiApplication app(argc, argv);
    QLoggingCategory::setFilterRules(LOG_FILTER_RULES);
    QTestWindow window;
    QTimer timer;
    timer.setInterval(1);
    app.connect(&timer, &QTimer::timeout, &app, [&] {
        window.draw();
    });
    timer.start();
    app.exec();
    return 0;
}

#include "main.moc"
