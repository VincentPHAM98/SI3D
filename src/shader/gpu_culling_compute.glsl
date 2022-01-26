#version 430

#ifdef COMPUTE_SHADER

bool isPointInsideBox(vec3 p, vec3 pmin, vec3 pmax) {
    return (
        (p.x >= pmin.x && p.x <= pmax.x) &&
        (p.y >= pmin.y && p.y <= pmax.y) &&
        (p.z >= pmin.z && p.z <= pmax.z));
}

// p point repère projectif
bool isVertexVisible(vec4 p) {
    float arr[3] = { p.x, p.y, p.z };
    for (uint i = 0; i < 3u; i++) {
        if (arr[i] <= -p.w || arr[i] >= p.w)
            return false;
    }
    return true;
}

// is box inside frustum
bool isBoxVisible(mat4 world2projection, mat4 projection2world, 
vec3 pmin, vec3 pmax) {
    vec4 cornerPoints[8] = {
        vec4(pmin, 1.f),
        vec4(pmax.x, pmin.y, pmin.z, 1.f),
        vec4(pmax.x, pmax.y, pmin.z, 1.f),
        vec4(pmin.x, pmax.y, pmin.z, 1.f),
        vec4(pmin.x, pmax.y, pmax.z, 1.f),
        vec4(pmin.x, pmin.y, pmax.z, 1.f),
        vec4(pmax.x, pmin.y, pmax.z, 1.f),
        vec4(pmax, 1.f)
    };

    // verifier si box dans frustum
    for (int i = 0; i < 8; ++i) {
        // on applique la transformation projective avant de tester
        // les points de la box sont dans le repère monde
        vec4 pBoxProj = world2projection * cornerPoints[i];
        if (isVertexVisible(pBoxProj))
            return true;
    }

    // verifier si corner du frustum sont dans box
    vec4 frustumWorldPoints[8] = {
        projection2world * vec4(-1, 1, -1, 1),
        projection2world * vec4(1, 1, -1, 1),
        projection2world * vec4(1, -1, -1, 1),
        projection2world * vec4(-1, -1, -1, 1),
        projection2world * vec4(-1, 1, 1, 1),
        projection2world * vec4(1, 1, 1, 1),
        projection2world * vec4(1, -1, 1, 1),
        projection2world * vec4(-1, -1, 1, 1)
    };
    for (int i = 0; i < 8; i++) {
        if (isPointInsideBox(frustumWorldPoints[i].xyz, pmin, pmax))
            return true;
    }
    return false;
}

struct Object {
    vec3 pmin;
    uint vertex_count;
    vec3 pmax;
    uint vertex_base;
};

struct Draw {
    uint vertex_count;
    uint instance_count;
    uint vertex_base;
    uint instance_base;
};


layout (binding = 0, std430) readonly 
buffer regionData {
    Object objects[];
};

layout(binding= 1, std430) writeonly 
buffer remapData {
    uint remap[];
};

layout(binding = 2, std430) writeonly 
buffer paramData {
    Draw params[];
};

layout(binding = 3) 
buffer counterData {
    uint count;
};

uniform mat4 world2projection;
uniform mat4 projection2world;

shared uint groupCounter;
shared uint groupOffset; // endroit ou doit ecrire le groupe dans le buffer

layout(local_size_x= 256) in;
void main( )
{
    // recupere l'indice du thread
    const uint globalID= gl_GlobalInvocationID.x;
    // Le tout premier thread global met a 0 le compteur global. 1 seule fois.
    if (globalID == 0)
        count = 0;
    // uint id= gl_GlobalInvocationID.x;

    if(globalID >= objects.length())
        return;

    const uint localID = gl_LocalInvocationID.x;
    if (localID == 0) {
        groupCounter = 0;
        groupOffset = 0;
    }

    barrier();

    // recupere la bbox du ieme objet...
    vec3 pmin= objects[globalID].pmin;
    vec3 pmax= objects[globalID].pmax;

    // endroit ou on va ecrire la variable si elle valide le prédicat.
    int threadOffset = -1;
    if (isBoxVisible(world2projection, projection2world, pmin, pmax))
        threadOffset = int(atomicAdd(groupCounter, 1));

    barrier();
    if (localID == 0)
        groupOffset = atomicAdd(count, groupCounter);
    barrier();

    if (threadOffset != -1) {
        uint index = groupOffset + threadOffset;
        params[index].vertex_count= objects[globalID].vertex_count;
        params[index].instance_count= 1;
        params[index].vertex_base= objects[globalID].vertex_base;
        params[index].instance_base= 0;

        remap[index]= globalID;
    }
}

#endif
