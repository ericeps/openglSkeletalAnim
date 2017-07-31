#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <string>
#include <QMatrix4x4>
#include <vector>
#include <QFile>
#include <QSharedPointer>
#include <QDir>

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;
struct aiAnimation;

struct MaterialInfo
{
    QString Name;
    QVector3D Ambient;
    QVector3D Diffuse;
    QVector3D Specular;
    float Shininess;
};

struct LightInfo
{
    QVector4D Position;
    QVector3D Intensity;
};

struct Mesh
{
    QString name;
    unsigned int indexCount;
    unsigned int indexOffset;
    QSharedPointer<MaterialInfo> material;
    QVector<QMatrix4x4> boneOffsets;
    QVector<QString> boneNames;
};

enum AnimState {
    AnimState_Invalid,
    AnimState_Default,
    AnimState_Constant,
    AnimState_Linear,
    AnimState_Repeat
};

struct NodeAnimation {
    NodeAnimation() :
        positionIndex(0)
      , rotationIndex(0)
      , scalingIndex(0)
      , preState(AnimState_Invalid)
      , postState(AnimState_Invalid)
    {}

    QVector<QPair<double, QVector3D> > positionKeys;
    QVector<QPair<double, QQuaternion> > rotationKeys;
    QVector<QPair<double, QVector3D> > scalingKeys;

    mutable int positionIndex;
    mutable int rotationIndex;
    mutable int scalingIndex;

    AnimState preState;
    AnimState postState;

    bool isValid() const {
        return (preState != AnimState_Invalid && postState != AnimState_Invalid) && (!positionKeys.empty() || !rotationKeys.empty() || !scalingKeys.empty());
    }
};

struct Node
{
    QString name;

    QMatrix4x4 transformation;
    QVector<QSharedPointer<Mesh> > meshes;
    QVector<NodeAnimation> animationList;
    QVector<Node> nodes;
};

struct Animation {
    QString name;
    double duration;
    double ticksPerSecond;
};

typedef QPair<QString, NodeAnimation> NodeAnimationPair;
typedef QPair<QSharedPointer<Animation>, QVector<NodeAnimationPair> > AnimationType;

class ModelLoader
{
public:
    enum PathType {
        RelativePath,
        AbsolutePath
    };

    ModelLoader();
    void setTransformToUnitCoordinates(bool arg) { m_transformToUnitCoordinates = arg; }
    bool Load(QString filePath, PathType pathType);
    void getBufferData( QVector<float> **vertices, QVector<float> **normals,
                        QVector<unsigned int> **indices);

    void getTextureData( QVector<QVector<float> > **textureUV,                   // For texture mapping
                         QVector<float> **tangents, QVector<float> **bitangents);// For normal mapping

    void getBoneData( QVector<int> **vertexBoneIndexes, QVector<float> **vertexBoneWeights );

    QSharedPointer<Node> getNodeData();
    QVector<QSharedPointer<Mesh> > getMeshes() { return m_meshes; }
    QVector<QSharedPointer<Animation> > getNodeAnimations() { return m_animations; }

    // Texture information
    int numUVChannels() { return m_textureUV.size(); }
    int numUVComponents(int channel) { return m_textureUVComponents.at(channel); }
private:
    QSharedPointer<MaterialInfo> processMaterial(aiMaterial *mater);
    QSharedPointer<Mesh> processMesh(aiMesh *mesh);
    aiNode* findRootNode(aiNode *node);
    void processNode(const aiScene *scene, aiNode *node, Node *parentNode, Node &newNode);
    AnimationType processAnimation(aiAnimation *anim);
    int m_nodeHierarchyLevel;

    void transformToUnitCoordinates();
    void findObjectDimensions(Node *node, QMatrix4x4 transformation, QVector3D &minDimension, QVector3D &maxDimension);
    void findSetNodeAnimation(int animationIndex, NodeAnimationPair &anim, Node *node);

    QVector<float> m_vertices;
    QVector<float> m_normals;
    QVector<unsigned int> m_indices;

    QVector<QVector<float> > m_textureUV; // multiple channels
    QVector<float> m_tangents;
    QVector<float> m_bitangents;
    QVector<unsigned int > m_textureUVComponents; // multiple channels

    QVector<QSharedPointer<MaterialInfo> > m_materials;
    QVector<QSharedPointer<Mesh> > m_meshes;
    QSharedPointer<Node> m_rootNode;
    bool m_transformToUnitCoordinates;

    QVector<QSharedPointer<Animation> > m_animations;

    //QVector<QMatrix4x4> m_boneMatrices;
    QVector<int> m_vertexBoneIndices;
    QVector<float> m_vertexBoneWeights;
};

#endif // MODELLOADER_H
