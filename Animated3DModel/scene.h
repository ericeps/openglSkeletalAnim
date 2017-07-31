#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLFunctions>
#include "modelloader.h"
#include "scenebase.h"

class Scene : public QOpenGLFunctions_3_3_Core, public SceneBase
{
public:
    Scene(QString filepath, ModelLoader::PathType pathType, QString texturePath="");
    void initialize();
    void resize(int w, int h);
    void update();
    void cleanup();

private:
    void createShaderProgram( QString vShader, QString fShader);
    void createBuffers();
    void createAttributes();
    void setupLightingAndMatrices();

    void updateAnimationData(const Mesh &mesh, const Node *node, QMatrix4x4 objectMatrix);
    //void drawNode(const Node *node, QMatrix4x4 objectMatrix);
    void drawMesh(const Mesh &mesh);
    void setMaterialUniforms(MaterialInfo &mater);

    QOpenGLShaderProgram m_shaderProgram;

    QOpenGLVertexArrayObject m_vao;

    QOpenGLBuffer m_vertexBuffer;
    QOpenGLBuffer m_normalBuffer;
    QOpenGLBuffer m_textureUVBuffer;
    QOpenGLBuffer m_indexBuffer;

    QOpenGLBuffer m_vertexBoneIndexBuffer;
    QOpenGLBuffer m_vertexBoneWeightBuffer;

    QSharedPointer<Node> m_rootNode;
    QVector<QSharedPointer<Mesh> > m_meshes;

    QMatrix4x4 m_projection, m_model;

    QString m_filepath;
    ModelLoader::PathType m_pathType;
    QString m_texturePath;

    LightInfo m_lightInfo;
    MaterialInfo m_materialInfo;

    bool m_error;

    QVector<QSharedPointer<Animation> > m_animations;
    int m_currentAnimation;
    double m_currentAnimationTick;

    QHash<QString, QMatrix4x4> m_nodeModelMatrices;

    QMatrix4x4 m_inverseRootMatrix;
};

#endif // SCENE_H
