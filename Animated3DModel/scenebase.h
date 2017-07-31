#ifndef SCENEBASE_H
#define SCENEBASE_H

#include <QString>
#include <QMatrix4x4>

class SceneCamera {
public:
    SceneCamera()
    {
        m_camera.setToIdentity();
        m_camera.lookAt(
                    QVector3D(0.0f, 0.0f, 1.2f),    // Camera Position
                    QVector3D(0.0f, 0.0f, 0.0f),    // Point camera looks towards
                    QVector3D(0.0f, 1.0f, 0.0f));   // Up vector
    }
    void translate(float x, float y, float z) { m_camera.translate(x, y, z); }
    void rotate(float angle, float x, float y, float z) { m_camera.rotate(angle, x, y, z); }
    QMatrix4x4 matrix() { return m_camera; }

private:
    QMatrix4x4 m_camera;
};

class SceneBase
{
public:
    SceneBase() : m_camera(new SceneCamera) {}
    virtual void initialize() = 0;
    virtual void resize(int w, int h) = 0;
    virtual void update() = 0;
    virtual void cleanup() = 0;

    SceneCamera *getCamera() { return m_camera; }

    virtual ~SceneBase() {}

private:
    SceneCamera *m_camera;
};

class SceneBase;

class SceneSelector {
public:
    virtual SceneBase* createScene(QPair<int,int> glVersion) = 0;
};

#endif // SCENEBASE_H
