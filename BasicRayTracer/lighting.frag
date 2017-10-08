#version 330 core
out vec4 color;

uniform vec3 diffuseColor;

void main()
{
    
    color = vec4(diffuseColor, 0.5);
}
