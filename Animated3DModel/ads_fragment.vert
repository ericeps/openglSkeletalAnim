#version 330 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;

layout (location = 3) in vec4 boneIndexes;
layout (location = 4) in vec4 boneWeights;

uniform mat4 boneModelMatrix[100];

uniform mat4 MV;
uniform mat3 N;
uniform mat4 MVP;

out vec3 normal;
out vec3 position;

void main()
{
    mat4 boneTransform = mat4(1.0);

    if (boneIndexes[0] != -1) {
        boneTransform = boneModelMatrix[int(boneIndexes[0])] * boneWeights[0];
        //boneTransform = boneModelMatrix[0] * boneWeights[0];
        //boneTransform = mat4(1.0) * boneWeights[0];
    }

    for (int ii=1; ii<4; ++ii) {
        if (boneIndexes[ii] != -1) {
            boneTransform += boneModelMatrix[int(boneIndexes[ii])] * boneWeights[ii];
            //boneTransform += boneModelMatrix[1] * boneWeights[ii];
            //boneTransform += mat4(1.0) * boneWeights[ii];
        }
    }

    //normal = normalize( N * norm.xyz );
    normal = normalize((MV * boneTransform * vec4(vertexNormal, 0.0)).xyz);
    position = vec3( MV * boneTransform * vec4( vertexPosition, 1.0 ) );

    gl_Position = MVP * boneTransform * vec4( vertexPosition, 1.0 );

//    normal = normalize( N * vertexNormal );
//    position = vec3( MV * vec4( vertexPosition, 1.0 ) );

//    gl_Position = MVP * vec4( vertexPosition, 1.0 );
}
