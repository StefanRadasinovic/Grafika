#version 330 core

struct PositionalLight {
	vec3 Position;
	vec3 Ka;
	vec3 Kd;
	vec3 Ks;
	float Kc;
	float Kl;
	float Kq;
};

struct DirectionalLight {
	vec3 Position;
	vec3 Direction;
	vec3 Ka;
	vec3 Kd;
	vec3 Ks;
	float InnerCutOff;
	float OuterCutOff;
	float Kc;
	float Kl;
	float Kq;
};

struct Material {
	// NOTE(Jovan): Diffuse is used as ambient as well since the light source
	// defines the ambient colour
	sampler2D Kd;
	sampler2D Ks;
	float Shininess;
};

uniform PositionalLight uPointLight;
uniform DirectionalLight uSpotlight;
uniform DirectionalLight uSpotlight1;
uniform DirectionalLight uSpotlight2;
uniform DirectionalLight uSpotlight3;
uniform DirectionalLight uSpotlight4;
uniform DirectionalLight uSpotlight5;
uniform DirectionalLight uDirLight;
uniform Material uMaterial;
uniform vec3 uViewPos;

in vec2 UV;
in vec3 vWorldSpaceFragment;
in vec3 vWorldSpaceNormal;

out vec4 FragColor;

vec3 SpotlightRender(DirectionalLight uSpotlight, vec3 vWorldSpaceFragment, vec3 vWorldSpaceNormal, vec3 ViewDirection, Material uMaterial, vec2 UV){
	vec3 SpotlightVector = normalize(uSpotlight.Position - vWorldSpaceFragment);

	float SpotDiffuse = max(dot(vWorldSpaceNormal, SpotlightVector), 0.0f);
	vec3 SpotReflectDirection = reflect(-SpotlightVector, vWorldSpaceNormal);
	float SpotSpecular = pow(max(dot(ViewDirection, SpotReflectDirection), 0.0f), uMaterial.Shininess);

	vec3 SpotAmbientColor = uSpotlight.Ka * vec3(texture(uMaterial.Kd, UV));
	vec3 SpotDiffuseColor = SpotDiffuse * uSpotlight.Kd * vec3(texture(uMaterial.Kd, UV));
	vec3 SpotSpecularColor = SpotSpecular * uSpotlight.Ks * vec3(texture(uMaterial.Ks, UV));

	float SpotlightDistance = length(uSpotlight.Position - vWorldSpaceFragment);
	float SpotAttenuation = 1.0f / (uSpotlight.Kc + uSpotlight.Kl * SpotlightDistance + uSpotlight.Kq * (SpotlightDistance * SpotlightDistance));

	float Theta = dot(SpotlightVector, normalize(-uSpotlight.Direction));
	float Epsilon = uSpotlight.InnerCutOff - uSpotlight.OuterCutOff;
	float SpotIntensity = clamp((Theta - uSpotlight.OuterCutOff) / Epsilon, 0.0f, 1.0f);
	vec3 SpotColor = SpotIntensity * SpotAttenuation * (SpotAmbientColor + SpotDiffuseColor + SpotSpecularColor);

	return SpotColor;
}

void main() {
	vec3 ViewDirection = normalize(uViewPos - vWorldSpaceFragment);
	// NOTE(Jovan): Directional light
	vec3 DirLightVector = normalize(-uDirLight.Direction);
	float DirDiffuse = max(dot(vWorldSpaceNormal, DirLightVector), 0.0f);
	vec3 DirReflectDirection = reflect(-DirLightVector, vWorldSpaceNormal);
	// NOTE(Jovan): 32 is the specular shininess factor. Hardcoded for now
	float DirSpecular = pow(max(dot(ViewDirection, DirReflectDirection), 0.0f), uMaterial.Shininess);

	vec3 DirAmbientColor = uDirLight.Ka * vec3(texture(uMaterial.Kd, UV));
	vec3 DirDiffuseColor = uDirLight.Kd * DirDiffuse * vec3(texture(uMaterial.Kd, UV));
	vec3 DirSpecularColor = uDirLight.Ks * DirSpecular * vec3(texture(uMaterial.Ks, UV));
	vec3 DirColor = DirAmbientColor + DirDiffuseColor + DirSpecularColor;

	// NOTE(Jovan): Point light
	vec3 PtLightVector = normalize(uPointLight.Position - vWorldSpaceFragment);
	float PtDiffuse = max(dot(vWorldSpaceNormal, PtLightVector), 0.0f);
	vec3 PtReflectDirection = reflect(-PtLightVector, vWorldSpaceNormal);
	float PtSpecular = pow(max(dot(ViewDirection, PtReflectDirection), 0.0f), uMaterial.Shininess);

	vec3 PtAmbientColor = uPointLight.Ka * vec3(texture(uMaterial.Kd, UV));
	vec3 PtDiffuseColor = PtDiffuse * uPointLight.Kd * vec3(texture(uMaterial.Kd, UV));
	vec3 PtSpecularColor = PtSpecular * uPointLight.Ks * vec3(texture(uMaterial.Ks, UV));

	float PtLightDistance = length(uPointLight.Position - vWorldSpaceFragment);
	float PtAttenuation = 1.0f / (uPointLight.Kc + uPointLight.Kl * PtLightDistance + uPointLight.Kq * (PtLightDistance * PtLightDistance));
	vec3 PtColor = PtAttenuation * (PtAmbientColor + PtDiffuseColor + PtSpecularColor);

	// NOTE(Jovan): Spotlight
	vec3 SpotColor = SpotlightRender(uSpotlight, vWorldSpaceFragment, vWorldSpaceNormal, ViewDirection, uMaterial, UV);
	vec3 SpotColor1 = SpotlightRender(uSpotlight1, vWorldSpaceFragment, vWorldSpaceNormal, ViewDirection, uMaterial, UV);
	vec3 SpotColor2 = SpotlightRender(uSpotlight2, vWorldSpaceFragment, vWorldSpaceNormal, ViewDirection, uMaterial, UV);
	vec3 SpotColor3 = SpotlightRender(uSpotlight3, vWorldSpaceFragment, vWorldSpaceNormal, ViewDirection, uMaterial, UV);
	vec3 SpotColor4 = SpotlightRender(uSpotlight4, vWorldSpaceFragment, vWorldSpaceNormal, ViewDirection, uMaterial, UV);
	vec3 SpotColor5 = SpotlightRender(uSpotlight5, vWorldSpaceFragment, vWorldSpaceNormal, ViewDirection, uMaterial, UV);
	
	vec3 FinalColor = DirColor + PtColor + SpotColor + SpotColor1 + SpotColor2 + SpotColor3 + SpotColor4 + SpotColor5;
	FragColor = vec4(FinalColor, 1.0f);
}
