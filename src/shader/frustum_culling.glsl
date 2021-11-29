//! \file frustum_culling.glsl

#version 330

#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
layout(location= 2) in vec3 normal;
layout(location= 4) in uint material;

uniform mat4 mvMatrix;
uniform mat4 mvpMatrix;

out vec3 vertex_position;
out vec3 vertex_normal;
flat out uint vertex_material;

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);

    vertex_position= vec3(mvMatrix * vec4(position, 1));
    vertex_normal= mat3(mvMatrix) * normal;

    vertex_material= material;
}

#endif


#ifdef FRAGMENT_SHADER
out vec4 fragment_color;

in vec3 vertex_position;
in vec3 vertex_normal;
flat in uint vertex_material;	// !! decoration flat, le varying est marque explicitement comme non interpolable  !!

#define MAX_MATERIALS 16
uniform vec4 materials[MAX_MATERIALS];

void main( )
{
    vec3 l= normalize(-vertex_position);        // la camera est la source de lumiere.
    vec3 n= normalize(vertex_normal);
    float cos_theta= max(0, dot(n, l));
    vec3 r = normalize(2. * cos_theta * n - l);
    float spec = dot(r, l);

    vec3 ambient = 0.1 * vec3(1.,1.,1.);
    
    // recupere la couleur de la matiere du triangle, en fonction de son indice.
    vec4 color= materials[vertex_material];
    // fragment_color= color * (cos_theta + spec);
    fragment_color= vec4(ambient, 1.) + color * (cos_theta + spec);
}

#endif