﻿//
//  ShadingInfo.h
//
//  Written for CSE4170
//  Department of Computer Science and Engineering
//  Copyright © 2023 Sogang University. All rights reserved.
//

// light and material definitions
#pragma once

typedef struct _Light_Parameters {
	float position[4];
	float ambient_color[3];
} Light_Parameters;

typedef struct _loc_LIGHT_Parameters {
	GLint position;
	GLint color;
} loc_light_Parameters;

typedef struct _Material_Parameters {
	/*int  diffuseTex, normalTex, specularTex, emissiveTex;*/
	float ambient_color[4], diffuse_color[4], specular_color[4], emissive_color[4];
	float specular_exponent;
} Material_Parameters;

typedef struct _loc_Material_Parameters {
	GLint diffuseTex, normalTex, specularTex, emissiveTex;
} loc_Material_Parameters;