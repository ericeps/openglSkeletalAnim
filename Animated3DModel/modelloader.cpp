#include "modelloader.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <QDebug>
#include <set>

#define MAX_BONES_PER_VERTEX 4

ModelLoader::ModelLoader() :
      m_nodeHierarchyLevel(0)
    , m_transformToUnitCoordinates(false)
{

}

// look for file using relative path
QString findFile(QString relativeFilePath, int scanDepth)
{
    QString str = relativeFilePath;
    for(int ii=-1; ii<scanDepth; ++ii)
    {
        if(QFile::exists(str))
        {
            return str;
        }
        str.prepend("../");
    }
    return "";
}

bool ModelLoader::Load(QString filePath, PathType pathType)
{
    QString l_filePath;
    if (pathType == RelativePath)
        l_filePath = findFile(filePath, 5);
    else
        l_filePath = filePath;

    Assimp::Importer importer;

    importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, MAX_BONES_PER_VERTEX);

    const aiScene* scene = importer.ReadFile( l_filePath.toStdString(),
            aiProcess_GenSmoothNormals          |
            aiProcess_CalcTangentSpace          |
            aiProcess_Triangulate               |
            aiProcess_JoinIdenticalVertices     |
            aiProcess_SortByPType               |
            aiProcess_LimitBoneWeights
                                              );

    if( !scene)
    {
        qDebug() << "Error loading file: (assimp:) " << importer.GetErrorString();
        return false;
    }

    if(scene->HasMaterials())
    {
        for(unsigned int ii=0; ii<scene->mNumMaterials; ++ii)
        {
            QSharedPointer<MaterialInfo> mater = processMaterial(scene->mMaterials[ii]);
            m_materials.push_back(mater);
        }
    }

    if(scene->HasMeshes())
    {
        for(unsigned int ii=0; ii<scene->mNumMeshes; ++ii)
        {
            m_meshes.push_back(processMesh(scene->mMeshes[ii]));
        }
    }
    else
    {
        qDebug() << "Error: No meshes found";
        return false;
    }

    if(scene->HasLights())
    {
        qDebug() << "Has Lights";
    }

    if(scene->mRootNode != NULL)
    {
        aiNode *aiRootNode = findRootNode(scene->mRootNode);
        QString nodename = aiRootNode->mName.length != 0 ? aiRootNode->mName.C_Str() : "";
        qDebug() << "RootNode Is" << nodename;
        qDebug() << aiRootNode;
        Node *rootNode = new Node;
        m_nodeHierarchyLevel = 0;
        processNode(scene, aiRootNode, 0, *rootNode);
        m_rootNode.reset(rootNode);
    }
    else
    {
        qDebug() << "Error loading model";
        return false;
    }

    if (scene->HasAnimations()) {
        qDebug() << "Num Animations" << scene->mNumAnimations;
        for (int ii=0; ii<scene->mNumAnimations; ++ii) {
            AnimationType anim = processAnimation(scene->mAnimations[ii]);
            m_animations.append(anim.first);

            qDebug() << "ANIMATION" << ii;
            qDebug() <<
                        "\n    Name" << scene->mAnimations[ii]->mName.C_Str() <<
                        "\n    NumChannels" << scene->mAnimations[ii]->mNumChannels <<
                        "\n    mNumMeshChannels" << scene->mAnimations[ii]->mNumMeshChannels <<
                        "\n    Ticks" << scene->mAnimations[ii]->mTicksPerSecond <<
                        "\n    Duration" << scene->mAnimations[ii]->mDuration;
            for (int ian=0; ian<anim.second.size(); ++ian) {
                qDebug() << "AnimSecond" << anim.second[ian].first;
                findSetNodeAnimation(ii, anim.second[ian], m_rootNode.data());
            }
        }
    }

    // This will transform the model to unit coordinates, so a model of any size or shape will fit on screen
    if (m_transformToUnitCoordinates)
        transformToUnitCoordinates();

    return true;
}

void ModelLoader::getBufferData( QVector<float> **vertices, QVector<float> **normals, QVector<unsigned int> **indices)
{
    if(vertices != 0)
        *vertices = &m_vertices;

    if(normals != 0)
        *normals = &m_normals;

    if(indices != 0)
        *indices = &m_indices;
}

void ModelLoader::getTextureData(QVector<QVector<float> > **textureUV, QVector<float> **tangents, QVector<float> **bitangents)
{
    if(textureUV != 0)
        *textureUV = &m_textureUV;

    if(tangents != 0)
        *tangents = &m_tangents;

    if(bitangents != 0)
        *bitangents = &m_bitangents;
}

void ModelLoader::getBoneData(QVector<int> **vertexBoneIndexes, QVector<float> **vertexBoneWeights)
{
    if (vertexBoneIndexes)
        *vertexBoneIndexes = &m_vertexBoneIndices;

    if (vertexBoneWeights)
        *vertexBoneWeights = &m_vertexBoneWeights;
}

QSharedPointer<Node> ModelLoader::getNodeData()
{
    return m_rootNode;
}

QSharedPointer<MaterialInfo> ModelLoader::processMaterial(aiMaterial *material)
{
    QSharedPointer<MaterialInfo> mater(new MaterialInfo);

    aiString mname;
    material->Get( AI_MATKEY_NAME, mname);
    if(mname.length > 0)
        mater->Name = mname.C_Str();

    int shadingModel;
    material->Get( AI_MATKEY_SHADING_MODEL, shadingModel );

    if(shadingModel != aiShadingMode_Phong && shadingModel != aiShadingMode_Gouraud)
    {
        qDebug() << "This mesh's shading model (" << shadingModel << ") is not implemented in this loader, setting to default material";
        mater->Name = "DefaultMaterial";
    }
    else
    {
        aiColor3D dif (0.f,0.f,0.f);
        aiColor3D amb (0.f,0.f,0.f);
        aiColor3D spec (0.f,0.f,0.f);
        float shine = 0.0;

        material->Get( AI_MATKEY_COLOR_AMBIENT, amb);
        material->Get( AI_MATKEY_COLOR_DIFFUSE, dif); //->Get(<material-key>,<where-to-store>))
        material->Get( AI_MATKEY_COLOR_SPECULAR, spec);
        material->Get( AI_MATKEY_SHININESS, shine);

        mater->Ambient = QVector3D(amb.r, amb.g, amb.b);
        mater->Diffuse = QVector3D(dif.r, dif.g, dif.b);
        mater->Specular = QVector3D(spec.r, spec.g, spec.b);
        mater->Shininess = shine;

        mater->Ambient *= .2f;
        if( mater->Shininess == 0.0) mater->Shininess = 30;
    }

    qDebug() << "Material Name " << mater->Name << mater->Ambient << mater->Diffuse << mater->Specular << mater->Shininess;

    return mater;
}

QSharedPointer<Mesh> ModelLoader::processMesh(aiMesh *mesh)
{
    QSharedPointer<Mesh> newMesh(new Mesh);
    newMesh->name = mesh->mName.length != 0 ? mesh->mName.C_Str() : "";
    newMesh->indexOffset = m_indices.size();
    unsigned int indexCountBefore = m_indices.size();
    int vertindexoffset = m_vertices.size()/3;

    // Get Vertices
    if(mesh->mNumVertices > 0)
    {
        for(uint ii=0; ii<mesh->mNumVertices; ++ii)
        {
            aiVector3D &vec = mesh->mVertices[ii];

            m_vertices.push_back(vec.x);
            m_vertices.push_back(vec.y);
            m_vertices.push_back(vec.z);
        }
    }

    int boneIndexStart = m_vertexBoneIndices.size();

    if (mesh->HasBones()) {
        qDebug() << "MeshName" << newMesh->name << "Has Bones" << mesh->mNumBones;

        for (int ii=boneIndexStart; ii<(m_vertices.size()/3)*MAX_BONES_PER_VERTEX; ++ii) {
            m_vertexBoneIndices.append(-1);
            m_vertexBoneWeights.append(0.0);
        }

        for (int ii=0; ii<mesh->mNumBones; ++ii) {
            qDebug() << "    BoneName" << mesh->mBones[ii]->mName.C_Str();
            newMesh->boneNames.append(mesh->mBones[ii]->mName.length != 0 ? mesh->mBones[ii]->mName.C_Str() : "");

            QMatrix4x4 matrixOffset(mesh->mBones[ii]->mOffsetMatrix[0]);
            newMesh->boneOffsets.append(matrixOffset);

            for (int ib=0; ib<mesh->mBones[ii]->mNumWeights; ++ib) {
                int vertexBoneIndex = boneIndexStart + (mesh->mBones[ii]->mWeights[ib].mVertexId * MAX_BONES_PER_VERTEX);
                //int vertexBoneIndex = (vertindexoffset * MAX_BONES_PER_VERTEX) + mesh->mBones[ii]->mWeights[ib].mVertexId;
                while (m_vertexBoneIndices[vertexBoneIndex] != -1)
                    ++vertexBoneIndex;
                m_vertexBoneIndices[vertexBoneIndex] = newMesh->boneOffsets.size()-1;
                m_vertexBoneWeights[vertexBoneIndex] = mesh->mBones[ii]->mWeights[ib].mWeight;
            }
        }
    }
    else
        qDebug() << "MeshName" << newMesh->name;

    // Get Normals
    if(mesh->HasNormals())
    {
        for(uint ii=0; ii<mesh->mNumVertices; ++ii)
        {
            aiVector3D &vec = mesh->mNormals[ii];
            m_normals.push_back(vec.x);
            m_normals.push_back(vec.y);
            m_normals.push_back(vec.z);
        };
    }

    // Get Texture coordinates
    if(mesh->GetNumUVChannels() > 0)
    {
        if((unsigned int)m_textureUV.size() < mesh->GetNumUVChannels()) // Caution, assumes all meshes in this model have same number of uv channels
        {
            m_textureUV.resize(mesh->GetNumUVChannels());
            m_textureUVComponents.resize(mesh->GetNumUVChannels());
        }

        for( unsigned int ich = 0; ich < mesh->GetNumUVChannels(); ++ich)
        {
            m_textureUVComponents[ich] = mesh->mNumUVComponents[ich];

            for(uint iind = 0; iind<mesh->mNumVertices; ++iind)
            {
                m_textureUV[ich].push_back(    mesh->mTextureCoords[ich][iind].x);
                if(mesh->mNumUVComponents[ich] > 1)
                {
                    m_textureUV[ich].push_back(mesh->mTextureCoords[ich][iind].y);
                    if(mesh->mNumUVComponents[ich] > 2)
                    {
                        m_textureUV[ich].push_back(    mesh->mTextureCoords[ich][iind].z);
                    }
                }
            }
        }
    }

    // Get Tangents and bitangents
    if(mesh->HasTangentsAndBitangents())
    {
        //addTangentsAndBitangents(mesh, m_tangents, m_bitangents);
        for(uint ii=0; ii<mesh->mNumVertices; ++ii)
        {
            aiVector3D &vec = mesh->mTangents[ii];
            m_tangents.push_back(vec.x);
            m_tangents.push_back(vec.y);
            m_tangents.push_back(vec.z);

            aiVector3D &vec2 = mesh->mBitangents[ii];
            m_bitangents.push_back(vec2.x);
            m_bitangents.push_back(vec2.y);
            m_bitangents.push_back(vec2.z);
        };
    }

    // Get mesh indexes
    for(uint t = 0; t<mesh->mNumFaces; ++t)
    {
        aiFace* face = &mesh->mFaces[t];
        if(face->mNumIndices != 3)
        {
            qDebug() << "Warning: Mesh face with not exactly 3 indices, ignoring this primitive.";
            continue;
        }

        m_indices.push_back(face->mIndices[0]+vertindexoffset);
        m_indices.push_back(face->mIndices[1]+vertindexoffset);
        m_indices.push_back(face->mIndices[2]+vertindexoffset);
    }

    newMesh->indexCount = m_indices.size() - indexCountBefore;
    newMesh->material = m_materials.at(mesh->mMaterialIndex);

    return newMesh;
}

aiNode *ModelLoader::findRootNode(aiNode *node)
{
    return node;

    QString nodename = node->mName.length != 0 ? node->mName.C_Str() : "";

    //qDebug () << "Matrix" << nodename << QMatrix4x4(node->mTransformation[0]);

    if (QString(nodename) == QString("root"))
        return node;
    else
    {
        for (int ii=0; ii<node->mNumChildren; ++ii) {
            aiNode *n = findRootNode(node->mChildren[ii]);
            if (n != 0)
                return n;
        }
    }
    return 0;
}

void ModelLoader::processNode(const aiScene *scene, aiNode *node, Node *parentNode, Node &newNode)
{
    newNode.name = node->mName.length != 0 ? node->mName.C_Str() : "";

    if (scene->HasAnimations())
        newNode.animationList.resize(scene->mNumAnimations);

    QString indentation("");
    for (int ii=0; ii<m_nodeHierarchyLevel; ++ii)
        indentation.append("    ");
    qDebug() << QString("%1NodeName: %2, NumMeshes:%3").arg(indentation).arg(newNode.name).arg(node->mNumMeshes);

    newNode.transformation = QMatrix4x4(node->mTransformation[0]);

    newNode.meshes.resize(node->mNumMeshes);
    for(uint imesh = 0; imesh < node->mNumMeshes; ++imesh)
    {
        QSharedPointer<Mesh> mesh = m_meshes[node->mMeshes[imesh]];
        newNode.meshes[imesh] = mesh;
    }

    m_nodeHierarchyLevel += 1;
    for(uint ich = 0; ich < node->mNumChildren; ++ich)
    {
        newNode.nodes.push_back(Node());
        processNode(scene, node->mChildren[ich], parentNode, newNode.nodes[ich]);
    }
    m_nodeHierarchyLevel -= 1;
}

AnimationType ModelLoader::processAnimation(aiAnimation *anim)
{
    QSharedPointer<Animation> animation(new Animation);
    animation->name = anim->mName.length != 0 ? anim->mName.C_Str() : "";
    animation->duration = anim->mDuration;
    animation->ticksPerSecond = anim->mTicksPerSecond;

    QVector<NodeAnimationPair> nodeAnimations;

    for (int ii=0; ii<anim->mNumChannels; ++ii) {
        aiNodeAnim *nodeAnim = anim->mChannels[ii];

        nodeAnimations.append(qMakePair(QString(), NodeAnimation()));
        nodeAnimations.last().first = nodeAnim->mNodeName.length != 0 ? nodeAnim->mNodeName.C_Str() : "";
        NodeAnimation &nodeAnimation = nodeAnimations.last().second;

        nodeAnimation.preState = AnimState_Invalid;

        if (nodeAnim->mPreState == aiAnimBehaviour_DEFAULT)
            nodeAnimation.preState = AnimState_Default;
        else if (nodeAnim->mPreState == aiAnimBehaviour_CONSTANT)
            nodeAnimation.preState = AnimState_Constant;
        else if (nodeAnim->mPreState == aiAnimBehaviour_LINEAR)
            nodeAnimation.preState = AnimState_Linear;
        else if (nodeAnim->mPreState == aiAnimBehaviour_REPEAT)
            nodeAnimation.preState = AnimState_Repeat;

        nodeAnimation.postState = AnimState_Invalid;

        if (nodeAnim->mPostState == aiAnimBehaviour_DEFAULT)
            nodeAnimation.postState = AnimState_Default;
        else if (nodeAnim->mPostState == aiAnimBehaviour_CONSTANT)
            nodeAnimation.postState = AnimState_Constant;
        else if (nodeAnim->mPostState == aiAnimBehaviour_LINEAR)
            nodeAnimation.postState = AnimState_Linear;
        else if (nodeAnim->mPostState == aiAnimBehaviour_REPEAT)
            nodeAnimation.postState = AnimState_Repeat;

        for (int ip=0; ip<nodeAnim->mNumScalingKeys; ++ip) {
            aiVectorKey vk = nodeAnim->mScalingKeys[ip];
            nodeAnimation.scalingKeys.append(qMakePair(vk.mTime, QVector3D(vk.mValue.x, vk.mValue.y, vk.mValue.z)));
        }
        for (int ip=0; ip<nodeAnim->mNumRotationKeys; ++ip) {
            aiQuatKey vk = nodeAnim->mRotationKeys[ip];
            nodeAnimation.rotationKeys.append(qMakePair(vk.mTime, QQuaternion(vk.mValue.w, vk.mValue.x, vk.mValue.y, vk.mValue.z)));
        }
        for (int ip=0; ip<nodeAnim->mNumPositionKeys; ++ip) {
            aiVectorKey vk = nodeAnim->mPositionKeys[ip];
            nodeAnimation.positionKeys.append(qMakePair(vk.mTime, QVector3D(vk.mValue.x, vk.mValue.y, vk.mValue.z)));
        }
    }

    return qMakePair(animation, nodeAnimations);
}

void ModelLoader::transformToUnitCoordinates()
{
    // This will transform the model to unit coordinates, so a model of any size or shape will fit on screen

    double amin = std::numeric_limits<double>::max();
    double amax = std::numeric_limits<double>::min();
    QVector3D minDimension(amin,amin,amin);
    QVector3D maxDimension(amax,amax,amax);

    // Get the minimum and maximum x,y,z values for the model
    findObjectDimensions(m_rootNode.data(), QMatrix4x4(), minDimension, maxDimension);

    // Calculate scale and translation needed to center and fit on screen
    float dist = qMax(maxDimension.x() - minDimension.x(), qMax(maxDimension.y()-minDimension.y(), maxDimension.z() - minDimension.z()));
    float sc = 1.0/dist;
    QVector3D center = (maxDimension - minDimension)/2;
    QVector3D trans = -(maxDimension - center);

    qDebug() << "Min" <<minDimension << maxDimension;

    // Apply the scale and translation to a matrix
    QMatrix4x4 transformation;
    transformation.setToIdentity();
    transformation.scale(sc);
    transformation.translate(trans);

    // Multiply the transformation to the root node transformation matrix
    m_rootNode.data()->transformation = transformation * m_rootNode.data()->transformation;
}

void ModelLoader::findObjectDimensions(Node *node, QMatrix4x4 transformation, QVector3D &minDimension, QVector3D &maxDimension)
{
    transformation *= node->transformation;

    for (int ii=0; ii<node->meshes.size(); ++ii) {
        int start = node->meshes[ii]->indexOffset;
        int end = start + node->meshes[ii]->indexCount;
        for(int ii=start; ii<end; ++ii)
        {
            int ind = m_indices[ii] * 3;
            QVector4D vec(m_vertices[ind], m_vertices[ind+1], m_vertices[ind+2], 1.0);
            vec = transformation * vec;

            if(vec.x() < minDimension.x())
                minDimension.setX(vec.x());
            if(vec.y() < minDimension.y())
                minDimension.setY(vec.y());
            if(vec.z() < minDimension.z())
                minDimension.setZ(vec.z());
            if(vec.x() > maxDimension.x())
                maxDimension.setX(vec.x());
            if(vec.y() > maxDimension.y())
                maxDimension.setY(vec.y());
            if(vec.z() > maxDimension.z())
                maxDimension.setZ(vec.z());
        }
    }

    for (int ii=0; ii<node->nodes.size(); ++ii) {
        findObjectDimensions(&(node->nodes[ii]), transformation, minDimension, maxDimension);
    }
}

void ModelLoader::findSetNodeAnimation(int animationIndex, NodeAnimationPair &anim, Node *node)
{
    if (anim.first == node->name)
        node->animationList[animationIndex] = anim.second;

    for (int ii=0; ii<node->nodes.size(); ++ii)
        findSetNodeAnimation(animationIndex, anim, &(node->nodes[ii]));
}
