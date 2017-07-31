#include "window.h"
#include <QApplication>
#include "scene.h"
#include "scene_gles.h"
#include <QQuickView>
#include <QScreen>
#include <QQmlContext>

QStringList filepath {
    "animationModels/three_js_models/monster/monster.dae",
    "animationModels/three_js_models/avatar/avatar.dae",
    "../AstroBoy_Walk/astroBoy_walk_Maya.dae",
    "animationModels/three_js_models/duck/duck.dae"
};

QString getFilepath() {
    return filepath[2];
}

class SceneSelect : public SceneSelector {
public:
    SceneBase* createScene(QPair<int, int> glVersion) {
        if (!m_scene)
            delete m_scene;

        // use Scene class when GL version is 3.3
        if (glVersion == qMakePair(3,3)) {
            m_scene = new Scene(getFilepath(), ModelLoader::RelativePath);
        }
        // just use GL ES scene for any other version
        else {
            qCritical() << "Your computer must support OpenGL 3.3";
            exit(1);
            m_scene = new Scene_GLES(getFilepath(), ModelLoader::RelativePath);
        }
        return m_scene;
    }
    SceneBase* getScene() {
        return m_scene;
    }

    SceneSelect() : m_scene(0) {}
private:
    SceneBase *m_scene;
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    SceneSelect sceneSelect;

    OpenGLWindow w1(&sceneSelect, 40, 3, 3);

    w1.show();

    return app.exec();
}
