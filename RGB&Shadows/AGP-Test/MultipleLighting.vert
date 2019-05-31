
// Code sourced from B00298673's Individual Project

//-----------------------------------------------------------------//

// This was originally phong-tex.vert
// It has been modified to allow more than one light to be implented

#version 330

uniform mat4 modelview;
uniform mat4 projection;
uniform vec4 lightPosition[3]; //Positions for point lights
//uniform mat3 normalmatrix;

in  vec3 in_Position;
in  vec3 in_Normal;
out vec3 ex_N;
out vec3 ex_V;
out vec3 ex_L[3];

in vec2 in_TexCoord;
out vec2 ex_TexCoord;
out float ex_D[3];

// multiply each vertex position by the MVP matrix
// and find V, L, N vectors for the fragment shader
void main(void) {

	for(int i = 0; i < 3; i++)
	{
		// vertex into eye coordinates
		vec4 vertexPosition = modelview * vec4(in_Position,1.0);
		ex_D[i] = distance(vertexPosition,lightPosition[i]);
		// Find V - in eye coordinates, eye is at (0,0,0)
		ex_V = normalize(-vertexPosition).xyz;

		// surface normal in eye coordinates
		// taking the rotation part of the modelview matrix to generate the normal matrix
		// (if scaling is includes, should use transpose inverse modelview matrix!)
		// this is somewhat wasteful in compute time and should really be part of the cpu program,
		// giving an additional uniform input
		mat3 normalmatrix = transpose(inverse(mat3(modelview)));
		ex_N = normalize(normalmatrix * in_Normal);

		// L - to light source from vertex
		ex_L[i] = normalize(lightPosition[i].xyz - vertexPosition.xyz);

		ex_TexCoord = in_TexCoord;

		gl_Position = projection * vertexPosition;
	} 
}