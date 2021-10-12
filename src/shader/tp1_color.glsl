#version 330

#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
layout(location= 1) in vec2 texcoord;
layout(location= 2) in vec3 normal;
layout(location= 4) in uint material;

uniform mat4 mvpMatrix;
uniform mat4 mvMatrix;

out vec3 vertex_position;
out vec2 vertex_texcoord;
out vec3 vertex_normal;
flat out uint vertex_material;
/* decoration flat : le varying est un entier, donc pas vraiment interpolable... il faut le declarer explicitement */

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
    
    // position et normale dans le repere camera
    vertex_position= vec3(mvMatrix * vec4(position, 1));
    vertex_texcoord= texcoord;
    vertex_normal= mat3(mvMatrix) * normal;
    // ... comme d'habitude
    
    // et transmet aussi l'indice de la matiere au fragment shader...
    vertex_material= material;
}

#endif


#ifdef FRAGMENT_SHADER
out vec4 fragment_color;

in vec3 vertex_position;
in vec2 vertex_texcoord;
in vec3 vertex_normal;
flat in uint vertex_material;	// !! decoration flat, le varying est marque explicitement comme non interpolable  !!

#define MAX_MATERIALS 256 
uniform int materials[MAX_MATERIALS];
uniform int textures[MAX_MATERIALS];

uniform sampler2DArray texture_array;    //< acces au tableau de textures...


void main( )
{
    vec3 l= normalize(-vertex_position);        // la camera est la source de lumiere.
    vec3 n= normalize(vertex_normal);
    float cos_theta= max(0, dot(n, l));
    int tex_idx = textures[vertex_material];
    // recupere la couleur de la matiere du triangle, en fonction de son indice.
    // vec4 color= materials[vertex_material];
    vec4 color= texture(texture_array, vec3(vertex_texcoord, tex_idx));
    // color = (color + materials[vertex_material]) * 0.5;
    if(color.a < 0.3)
        discard;


    fragment_color= color * pow(cos_theta, 0.5);
}

#endif
