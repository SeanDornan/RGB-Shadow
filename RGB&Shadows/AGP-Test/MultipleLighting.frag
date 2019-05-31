
// Code sourced from B00298673's Individual Project

//-----------------------------------------------------------------//

// This was originally phong-tex.frag
// It has been modified to allow more than one light to be implented


#version 330

// Some drivers require the following
precision highp float;

struct lightStruct
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;

	float attConst;
	float attLinear;
	float attQuadratic;
};

struct materialStruct
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

#define LIGHT_SOURCES 3

uniform lightStruct light[LIGHT_SOURCES];
uniform materialStruct material;
uniform sampler2D textureUnit0;

in vec3 ex_N;
in vec3 ex_V;
in vec3 ex_L[3];
in vec2 ex_TexCoord;
in float ex_D[3];
layout(location = 0) out vec4 out_Color;

vec4 PointLightCalculation(int number);
 
void main(void) 
{
    vec4 result;
	for(int i = 0; i < (LIGHT_SOURCES); i++)
	{
		result = (result + PointLightCalculation(i));
	}
	out_Color = result;
	
}

vec4 PointLightCalculation(int number)
{
		// Ambient intensity
		vec4 ambientI = light[number].ambient * material.ambient;

		// Diffuse intensity
		vec4 diffuseI = light[number].diffuse * material.diffuse;
		diffuseI = diffuseI * max(dot(normalize(ex_N),normalize(ex_L[number])),0);

		// Specular intensity
		// Calculate R - reflection of light
		vec3 R = normalize(reflect(normalize(-ex_L[number]),normalize(ex_N)));
		vec4 specularI = light[number].specular * material.specular;
		specularI = specularI * pow(max(dot(R,ex_V),0), material.shininess);

		float attenuation = ((light[number].attConst + light[number].attLinear * ex_D[number] + light[number].attQuadratic * (ex_D[number]*ex_D[number]))); 
		vec4 tmp_Color = (((diffuseI + specularI + ambientI)/attenuation)); 
		vec4 litColour = vec4(tmp_Color.rgb, 1.0)* texture(textureUnit0, ex_TexCoord);
		return litColour;
}