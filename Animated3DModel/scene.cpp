#include "scene.h"

Scene::Scene(QString filepath, ModelLoader::PathType pathType, QString texturePath) :
    m_indexBuffer(QOpenGLBuffer::IndexBuffer)
  , m_filepath(filepath)
  , m_pathType(pathType)
  , m_texturePath(texturePath)
  , m_error(false)
  , m_currentAnimation(0)
  , m_currentAnimationTick(0.0f)
{

}

void Scene::initialize()
{
    this->initializeOpenGLFunctions();

    createShaderProgram(":/ads_fragment.vert", ":/ads_fragment.frag");

    createBuffers();
    createAttributes();
    setupLightingAndMatrices();

    glEnable(GL_DEPTH_TEST);
    glClearColor(.5, .5, .5 ,1.0);
}

void Scene::createShaderProgram(QString vShader, QString fShader)
{
    // Compile vertex shader
    if ( !m_shaderProgram.addShaderFromSourceFile( QOpenGLShader::Vertex, vShader.toUtf8() ) ) {
        qCritical() << "Unable to compile vertex shader. Log:" << m_shaderProgram.log();
        m_error = true;
    }

    // Compile fragment shader
    if ( m_error || !m_shaderProgram.addShaderFromSourceFile( QOpenGLShader::Fragment, fShader.toUtf8() ) ) {
        qCritical() << "Unable to compile fragment shader. Log:" << m_shaderProgram.log();
        m_error = true;
    }

    // Link the shaders together into a program
    if ( m_error || !m_shaderProgram.link() ) {
        qCritical() << "Unable to link shader program. Log:" << m_shaderProgram.log();
        m_error = true;
    }
}

void Scene::createBuffers()
{
    ModelLoader model;
    model.setTransformToUnitCoordinates(true);

    if(!model.Load(m_filepath, m_pathType))
    {
        m_error = true;
        return;
    }

    QVector<float> *vertices;
    QVector<float> *normals;
    QVector<QVector<float> > *textureUV;
    QVector<unsigned int> *indices;

    model.getBufferData(&vertices, &normals, &indices);
    model.getTextureData(&textureUV, 0, 0);

    // Create a vertex array object
    m_vao.create();
    m_vao.bind();

    // Create a buffer and copy the vertex data to it
    m_vertexBuffer.create();
    m_vertexBuffer.setUsagePattern( QOpenGLBuffer::StaticDraw );
    m_vertexBuffer.bind();
    m_vertexBuffer.allocate( &(*vertices)[0], vertices->size() * sizeof( float ) );

    // Create a buffer and copy the vertex data to it
    m_normalBuffer.create();
    m_normalBuffer.setUsagePattern( QOpenGLBuffer::StaticDraw );
    m_normalBuffer.bind();
    m_normalBuffer.allocate( &(*normals)[0], normals->size() * sizeof( float ) );

    if(textureUV != 0 && textureUV->size() != 0)
    {
        // Create a buffer and copy the vertex data to it
        m_textureUVBuffer.create();
        m_textureUVBuffer.setUsagePattern( QOpenGLBuffer::StaticDraw );
        m_textureUVBuffer.bind();
        int texSize = 0;
        for(int ii=0; ii<textureUV->size(); ++ii)
            texSize += textureUV->at(ii).size();

        m_textureUVBuffer.allocate( &(*textureUV)[0][0], texSize * sizeof( float ) );
    }

    // Create a buffer and copy the index data to it
    m_indexBuffer.create();
    m_indexBuffer.setUsagePattern( QOpenGLBuffer::StaticDraw );
    m_indexBuffer.bind();
    m_indexBuffer.allocate( &(*indices)[0], indices->size() * sizeof( unsigned int ) );

    m_rootNode = model.getNodeData();
    m_inverseRootMatrix = m_rootNode->transformation.inverted();
    m_meshes = model.getMeshes();

    QVector<int> *vertexBoneIndexes;
    QVector<float> *vertexBoneWeights;

    model.getBoneData(&vertexBoneIndexes, &vertexBoneWeights);

    QVector<float> vbi;
    for (int ii=0; ii<vertexBoneIndexes->size(); ++ii)
        vbi.append(vertexBoneIndexes->at(ii));

    m_vertexBoneIndexBuffer.create();
    m_vertexBoneIndexBuffer.setUsagePattern( QOpenGLBuffer::StaticDraw );
    m_vertexBoneIndexBuffer.bind();
    m_vertexBoneIndexBuffer.allocate( &vbi[0], vbi.size() * sizeof( float ) );

    m_vertexBoneWeightBuffer.create();
    m_vertexBoneWeightBuffer.setUsagePattern( QOpenGLBuffer::StaticDraw );
    m_vertexBoneWeightBuffer.bind();
    m_vertexBoneWeightBuffer.allocate( &(*vertexBoneWeights)[0], vertexBoneWeights->size() * sizeof( float ) );

    qDebug() << "Vertices" << vertices->size();

    m_meshes = model.getMeshes();
    m_animations = model.getNodeAnimations();
}

void Scene::createAttributes()
{
    if(m_error)
        return;

    m_vao.bind();
    // Set up the vertex array state
    m_shaderProgram.bind();

    // Map vertex data to the vertex shader's layout location '0'
    m_vertexBuffer.bind();
    m_shaderProgram.enableAttributeArray( 0 );      // layout location
    m_shaderProgram.setAttributeBuffer( 0,          // layout location
                                        GL_FLOAT,   // data's type
                                        0,          // Offset to data in buffer
                                        3);         // number of components (3 for x,y,z)

    // Map normal data to the vertex shader's layout location '1'
    m_normalBuffer.bind();
    m_shaderProgram.enableAttributeArray( 1 );      // layout location
    m_shaderProgram.setAttributeBuffer( 1,          // layout location
                                        GL_FLOAT,   // data's type
                                        0,          // Offset to data in buffer
                                        3);         // number of components (3 for x,y,z)

    if(m_textureUVBuffer.isCreated()) {
        m_textureUVBuffer.bind();
        m_shaderProgram.enableAttributeArray( 2 );      // layout location
        m_shaderProgram.setAttributeBuffer( 2,          // layout location
                                            GL_FLOAT,   // data's type
                                            0,          // Offset to data in buffer
                                            2);         // number of components (2 for u,v)
    }

    m_vertexBoneIndexBuffer.bind();
    m_shaderProgram.enableAttributeArray( 3 );      // layout location
    m_shaderProgram.setAttributeBuffer( 3,          // layout location
                                        GL_FLOAT,   // data's type
                                        0,          // Offset to data in buffer
                                        4);         // number of components (3 for x,y,z)

    m_vertexBoneWeightBuffer.bind();
    m_shaderProgram.enableAttributeArray( 4 );      // layout location
    m_shaderProgram.setAttributeBuffer( 4,          // layout location
                                        GL_FLOAT,   // data's type
                                        0,          // Offset to data in buffer
                                        4);         // number of components (3 for x,y,z)

}

void Scene::setupLightingAndMatrices()
{
    float aspect = 4.0f/3.0f;
    m_projection.setToIdentity();
    m_projection.perspective(
                60.0f,          // field of vision
                aspect,         // aspect ratio
                0.3f,           // near clipping plane
                1000.0f);       // far clipping plane

    m_lightInfo.Position = QVector4D( 2.0f, -2.0f, 3.0f, 1.0f );
    //m_lightInfo.Intensity = QVector3D( .5f, .5f, .f5);
    m_lightInfo.Intensity = QVector3D( 1.0f, 1.0f, 1.0f);

    m_materialInfo.Ambient = QVector3D( 0.1f, 0.05f, 0.0f );
    m_materialInfo.Diffuse = QVector3D( .9f, .6f, .2f );
    m_materialInfo.Specular = QVector3D( .2f, .2f, .2f );
    m_materialInfo.Shininess = 50.0f;
}

void Scene::updateAnimationData(const Mesh &mesh, const Node *node, QMatrix4x4 objectMatrix)
{
    // Prepare matrices
    if (m_currentAnimation != -1 && node->animationList[m_currentAnimation].isValid()) {

        const NodeAnimation &nodeAnim = node->animationList[m_currentAnimation];

        if (m_currentAnimationTick == 0.0f) {
            nodeAnim.scalingIndex = nodeAnim.rotationIndex = nodeAnim.positionIndex = 0;
        }

        while (nodeAnim.scalingKeys.size()-1 > nodeAnim.scalingIndex
               && nodeAnim.scalingKeys[nodeAnim.scalingIndex+1].first < m_currentAnimationTick) {
            ++nodeAnim.scalingIndex;
        }

        while (nodeAnim.rotationKeys.size()-1 > nodeAnim.rotationIndex
               && nodeAnim.rotationKeys[nodeAnim.rotationIndex+1].first < m_currentAnimationTick) {
            ++nodeAnim.rotationIndex;
        }

        while (nodeAnim.positionKeys.size()-1 > nodeAnim.positionIndex
               && nodeAnim.positionKeys[nodeAnim.positionIndex+1].first < m_currentAnimationTick) {
            ++nodeAnim.positionIndex;
        }

        QMatrix4x4 transformationScale;
        QMatrix4x4 transformationRotate;
        QMatrix4x4 transformationTranslate;

        if (nodeAnim.positionKeys.size() > 0)
            transformationTranslate.translate(nodeAnim.positionKeys[nodeAnim.positionIndex].second);
        if (nodeAnim.rotationKeys.size() > 0)
            transformationRotate.rotate(nodeAnim.rotationKeys[nodeAnim.rotationIndex].second);
        if (nodeAnim.scalingKeys.size() > 0)
            transformationScale.scale(nodeAnim.scalingKeys[nodeAnim.scalingIndex].second);

        objectMatrix *= transformationTranslate * transformationRotate * transformationScale;
    }
    else {
        objectMatrix *= node->transformation;
    }

    const int boneIndex = mesh.boneNames.indexOf(node->name);

    if (boneIndex != -1)
        m_nodeModelMatrices[node->name] = m_inverseRootMatrix * objectMatrix * mesh.boneOffsets[boneIndex];
    else
        m_nodeModelMatrices[node->name] = m_inverseRootMatrix * objectMatrix;

    // Recursively draw this nodes children nodes
    for(int inn = 0; inn<node->nodes.size(); ++inn)
        updateAnimationData(mesh, &node->nodes[inn], objectMatrix);
}

void Scene::drawMesh(const Mesh &mesh)
{
    QVector<QMatrix4x4> modelMatrices;
    modelMatrices.resize(mesh.boneNames.size()/*100*/);
    for (int ii=0; ii<mesh.boneNames.size()/*100*/; ++ii) {
        //modelMatrices[ii] = QMatrix4x4();
        modelMatrices[ii] = m_nodeModelMatrices.value(mesh.boneNames[ii], QMatrix4x4());
    }

    m_shaderProgram.setUniformValueArray("boneModelMatrix", modelMatrices.data(), modelMatrices.size());

    if(mesh.material->Name == QString("DefaultMaterial"))
        setMaterialUniforms(m_materialInfo);
    else
        setMaterialUniforms(*mesh.material);

    // Set bone matrix offset array uniform
    // Set node matrix M array
    // set P, V matrix uniforms

    glDrawElements( GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT
                        , (const void*)(mesh.indexOffset * sizeof(unsigned int)) );
}

void Scene::resize(int w, int h)
{
    glViewport( 0, 0, w, h );

    m_projection.setToIdentity();
    m_projection.perspective(60.0f, (float)w/h, .3f, 1000);
}

void Scene::update()
{
    if(m_error)
        return;

    // Clear color and depth buffers
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind shader program
    m_shaderProgram.bind();

    m_model.setToIdentity();

    // Set shader uniforms for light information
    m_shaderProgram.setUniformValue( "lightPosition", m_lightInfo.Position );
    m_shaderProgram.setUniformValue( "lightIntensity", m_lightInfo.Intensity );

    QMatrix4x4 modelMatrix = m_rootNode->transformation;
    QMatrix4x4 modelViewMatrix = this->getCamera()->matrix() * modelMatrix;
    QMatrix3x3 normalMatrix = modelViewMatrix.normalMatrix();
    QMatrix4x4 mvp = m_projection * modelViewMatrix;

    m_shaderProgram.setUniformValue( "MV", modelViewMatrix );// Transforming to eye space
    m_shaderProgram.setUniformValue( "N", normalMatrix );    // Transform normal to Eye space
    m_shaderProgram.setUniformValue( "MVP", mvp );           // Matrix for transforming to Clip space

    // Bind VAO and draw everything
    m_vao.bind();
    for (int ii=0; ii<m_meshes.size(); ++ii) {
        updateAnimationData(*m_meshes.at(ii).data(), m_rootNode.data(), QMatrix4x4());
        drawMesh(*m_meshes.at(ii).data());
    }
    m_vao.release();


    if (m_currentAnimation != -1) {
        m_currentAnimationTick += m_animations[m_currentAnimation]->ticksPerSecond != 0 ? m_animations[m_currentAnimation]->ticksPerSecond / 25.0 : 1.0;
        //m_currentAnimationTick += .001;//m_animations[m_currentAnimation]->ticksPerSecond / 25.0;
        if (m_currentAnimationTick > m_animations[m_currentAnimation]->duration)
            m_currentAnimationTick = 0.0f;
        //qDebug() << "CurrentAnimationTick = " << m_currentAnimationTick;
    }
}

//void Scene::drawNode(const Node *node, QMatrix4x4 objectMatrix)
//{
//    // Prepare matrices
//    if (m_currentAnimation != -1 && node->animationList[m_currentAnimation].isValid()) {
//        // TODO calculate animated transformation matrix
//        // TODO make sure to reset NodeAnimation tick indexes
//        QMatrix4x4 transformation;
//        const NodeAnimation &nodeAnim = node->animationList[m_currentAnimation];

//        if (m_currentAnimationTick == 0.0f) {
//            //qDebug() << "Reset";
//            nodeAnim.scalingIndex = nodeAnim.rotationIndex = nodeAnim.positionIndex = 0;
//        }

//        if (nodeAnim.scalingKeys.size()-1 > nodeAnim.scalingIndex &&
//                nodeAnim.scalingKeys[nodeAnim.scalingIndex].first >= m_currentAnimationTick)
//            ++nodeAnim.scalingIndex;

//        if (nodeAnim.rotationKeys.size()-1 > nodeAnim.rotationIndex &&
//                nodeAnim.rotationKeys[nodeAnim.rotationIndex].first >= m_currentAnimationTick)
//            ++nodeAnim.rotationIndex;

//        if (nodeAnim.positionKeys.size()-1 > nodeAnim.positionIndex &&
//                nodeAnim.positionKeys[nodeAnim.positionIndex].first >= m_currentAnimationTick)
//            ++nodeAnim.positionIndex;

//        //qDebug() << node->name << nodeAnim.positionIndex << nodeAnim.rotationIndex << nodeAnim.scalingIndex;

//        if (nodeAnim.positionKeys.size() > 0)
//            transformation.translate(nodeAnim.positionKeys[nodeAnim.positionIndex].second);
//        if (nodeAnim.rotationKeys.size() > 0)
//            transformation.rotate(nodeAnim.rotationKeys[nodeAnim.rotationIndex].second);
//        if (nodeAnim.scalingKeys.size() > 0)
//            transformation.scale(nodeAnim.scalingKeys[nodeAnim.scalingIndex].second);

//        objectMatrix *= transformation;
//    }
//    else {
//        objectMatrix *= node->transformation;
//    }

//    QMatrix4x4 modelMatrix = m_model * objectMatrix;
//    QMatrix4x4 modelViewMatrix = this->getCamera()->matrix() * modelMatrix;
//    QMatrix3x3 normalMatrix = modelViewMatrix.normalMatrix();
//    QMatrix4x4 mvp = m_projection * modelViewMatrix;

//    m_shaderProgram.setUniformValue( "MV", modelViewMatrix );// Transforming to eye space
//    m_shaderProgram.setUniformValue( "N", normalMatrix );    // Transform normal to Eye space
//    m_shaderProgram.setUniformValue( "MVP", mvp );           // Matrix for transforming to Clip space

//    // Draw each mesh in this node
//    for(int imm = 0; imm<node->meshes.size(); ++imm)
//    {
//        if(node->meshes[imm]->material->Name == QString("DefaultMaterial"))
//            setMaterialUniforms(m_materialInfo);
//        else
//            setMaterialUniforms(*node->meshes[imm]->material);

//        glDrawElements( GL_TRIANGLES, node->meshes[imm]->indexCount, GL_UNSIGNED_INT
//                            , (const void*)(node->meshes[imm]->indexOffset * sizeof(unsigned int)) );
//    }

//    // Recursively draw this nodes children nodes
//    for(int inn = 0; inn<node->nodes.size(); ++inn)
//        drawNode(&node->nodes[inn], objectMatrix);
//}

void Scene::setMaterialUniforms(MaterialInfo &mater)
{
    m_shaderProgram.setUniformValue( "Ka", mater.Ambient );
    m_shaderProgram.setUniformValue( "Kd", mater.Diffuse );
    m_shaderProgram.setUniformValue( "Ks", mater.Specular );
    m_shaderProgram.setUniformValue( "shininess", mater.Shininess );
}

void Scene::cleanup()
{
}
