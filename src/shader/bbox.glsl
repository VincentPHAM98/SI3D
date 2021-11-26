//! \file tuto9_color.glsl

#version 330

#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
layout(location= 3) in vec4 color;

uniform mat4 mvpMatrix;

out vec4 vertexColor;
void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
    vertexColor = color;
}

#endif


#ifdef FRAGMENT_SHADER
in vec4 vertexColor;
out vec4 fragment_color;


void main( )
{
    fragment_color= vertexColor;
}

#endif
