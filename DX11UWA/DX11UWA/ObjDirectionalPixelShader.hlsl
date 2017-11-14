#define MAX_LIGHTS 8

#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

Texture2D Texture : register(t0);
sampler Sampler : register(s0);

struct _Material
{
	float4 Emissive; //emissive light component of material
	float4 Ambient; //ambient light component of material
	float4 Diffuse; //diffuse light component of material
	float4 Specular; //specular light component of material

	float SpecularPower; //how bright the specular highlight is
	bool useTexture; //whether or not to use a texture or simply the material color
	float2 Padding; //keeps memory usage at a multiple of 16
};

cbuffer MaterialProperties : register(b0)
{
	_Material Material;
}

struct Light
{
	float4 Position; //position of the light source
	float4 Direction; //where the light is pointing
	float4 Color; //color of the light source

	float SpotAngle; //angle in radians of spotlight cone
	float Radius; //radius of point light

	uint LightType; //0 - Directional, 1 - Point, 2 - Spotlight
	float Enabled; //whether this light should be included in calculations
	float useQuadraticAttenuation; //whether or not to apply attenuation quadratically
	float3 padding; //keeps the memory usage at a multiple of 16
};

cbuffer LightProperties : register(b1)
{
	float4 CameraPosition; //position of the view matrix
	float4 GlobalAmbient; //measure of ambient light applied to all objects in the scene
	Light Lights[MAX_LIGHTS];  //a container with all lights in the scene
}

float4 calculateDiffuse(Light light, float3 lightVector, float3 normalVector)
{
	return light.Color * saturate(dot(lightVector, normalVector)); //max used to guarantee that we dont get a negative dot product
}
float4 calculateSpecular(Light light, float3 lightVector, float3 normalVector, float3 viewVector)
{
	//phong lighting
	float3 reflectionVector = normalize(reflect(-lightVector, normalVector));
	float dotProductReflectionView = saturate( dot(reflectionVector, viewVector));

	return light.Color * pow(dotProductReflectionView, Material.SpecularPower);
}
float calculateAttenuation(Light light, float distance)
{
	float attenuation = 1 - saturate(distance / light.Radius);
	if (light.useQuadraticAttenuation == 1)
		return attenuation * attenuation;
	return attenuation;
}

float calculateSpotLightAttenuation(Light light, float3 lightVector)
{
	float minCos = cos(light.SpotAngle);
	float maxCos = (minCos + 1.0f) / 2.0f;
	float cosAngle = dot(normalize(light.Direction.xyz), -lightVector);
	return smoothstep(minCos, maxCos, cosAngle); //returns 1 if full light, 0 if no light, and somewhere inbetween if some light
}

struct LightingResult
{
	float4 diffuse;
	float4 specular;
};

//calculates diffused and specular light from a point source
LightingResult calculatePointLight(Light light, float3 viewVector, float4 Point, float3 normal)
{
	LightingResult result;

	float3 lightVector = (light.Position - Point).xyz; //get the vector between the light's position and the point's position
	float distance = length(lightVector); //find the distance between the light and the point
	lightVector = lightVector / distance; //normalize the light vector

	float attenuation = calculateAttenuation(light, distance);

	result.diffuse = calculateDiffuse(light, lightVector, normal) * attenuation; //find the amount of diffuse light and multiply by the attenuation ratio in order to account for distance
	result.specular = calculateSpecular(light, lightVector, normal, viewVector) * attenuation; 

	return result;
}

//calculates diffused and specular light from a directional source
LightingResult calculateDirectionalLight(Light light, float3 viewVector, float4 Point, float3 normal)
{
	LightingResult result;

	float3 lightVector = -light.Direction.xyz; //inverts the direction of the light so that calculations will work against the normal

	result.diffuse = calculateDiffuse(light, lightVector, normal); //no need to add attenuation for directional light
	result.specular = calculateSpecular(light, lightVector, normal, viewVector);

	return result;
}


//calculates diffused and specular light from a spotlight source
LightingResult calculateSpotLight(Light light, float3 viewVector, float4 Point, float3 normal)
{
	LightingResult result;

	float3 lightVector = normalize((light.Position - Point).xyz); //find the normalized vector between the point and light source

	float coneIntensity = calculateSpotLightAttenuation(light, lightVector);

	result.diffuse = calculateDiffuse(light, lightVector, normal) *coneIntensity; //find diffuse light while accounting for distance and intensity at its location in the spotlight's cone
	result.specular = calculateSpecular(light, lightVector, normal, viewVector) * coneIntensity;
	return result;
}

LightingResult computeLighting(float4 Point, float3 normal)
{
	float3 viewVector = normalize(CameraPosition - Point).xyz;
	LightingResult finalResult = { {0, 0, 0, 0}, {0, 0, 0, 0} };


	[unroll] //improves performance, if issues occur try swapping to [loop] instead
	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		LightingResult result = { { 0, 0, 0, 0 },{ 0, 0, 0, 0 } };
		//if light isnt enabled, skip it
		if (Lights[i].Enabled == 0)
			continue;

		switch (Lights[i].LightType)
		{
		case DIRECTIONAL_LIGHT:
			result = calculateDirectionalLight(Lights[i], viewVector, Point, normalize(normal));
			break;
		case POINT_LIGHT:
			result = calculatePointLight(Lights[i], viewVector, Point, normalize(normal));
			break;
		case SPOT_LIGHT:
			result = calculateSpotLight(Lights[i], viewVector, Point, normalize(normal));
			break;
		}
		finalResult.diffuse += result.diffuse;
		finalResult.specular += result.specular;
	}

	finalResult.diffuse = saturate(finalResult.diffuse);
	finalResult.specular = saturate(finalResult.specular);

	return finalResult;
}

struct PixelShaderInput
{
	float4 pos : TEXCOORD1;
	float3 norm : TEXCOORD2;
	float2 uv : TEXCOORD0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	LightingResult lit = computeLighting(float4(input.pos), normalize(input.norm));
	
	float4 emissive = Material.Emissive;
	float4 ambient = Material.Ambient * GlobalAmbient;
	float4 diffuse = Material.Diffuse * lit.diffuse;
	float4 specular = Material.Specular * lit.specular;

	float4 texColor = { 1, 1, 1, 1 };
	if (Material.useTexture)
	{
		texColor = Texture.Sample(Sampler, input.uv);
	}

	float4 finalColor = (emissive + ambient + diffuse + specular) * texColor;

	return finalColor;
}