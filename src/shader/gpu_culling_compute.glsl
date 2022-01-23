#version 430

#ifdef COMPUTE_SHADER

struct Box {
    vec3 pmin;
    vec3 pmax;
};

bool isPointInsideBox(vec3 p, vec3 pmin, vec3 pmax) {
    return (
        (p.x >= pmin.x && p.x <= pmax.x) &&
        (p.y >= pmin.y && p.y <= pmax.y) &&
        (p.z >= pmin.z && p.z <= pmax.z));
}

struct Frustum {
    mat4 view, projection;
};

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

struct Draw
{
    uint vertex_count;
    uint instance_count;
    uint vertex_base;
    uint instance_base;
};
    
layout(binding= 0, std430) writeonly buffer paramData
{
    Draw params[];
};


layout(local_size_x= 256) in;
void main( )
{
    uint id= gl_GlobalInvocationID.x;
    if(id >= params.length())
        return;

    if (id > params.length() / 2) 
        params[id].vertex_count= 0;
}

#endif
