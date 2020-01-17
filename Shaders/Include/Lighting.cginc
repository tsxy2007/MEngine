#ifndef __VOXELLIGHT_INCLUDE__
#define __VOXELLIGHT_INCLUDE__

#define XRES 32
#define YRES 16
#define ZRES 64
#define VOXELZ 64
#define MAXLIGHTPERCLUSTER 128
#define FROXELRATE 1.35
#define CLUSTERRATE 1.5
#define VOXELSIZE uint3(XRES, YRES, ZRES)

struct LightCommand{
    float3 direction;
	int shadowmapIndex;
	//
	float3 lightColor;
	uint lightType;
	//Align
	float3 position;
	float spotAngle;
	//Align
	float shadowSoftValue;
	float shadowBias;
	float shadowNormalBias;
	float range;
	//Align
	float spotRadius;
	float3 __align;
};
inline uint GetIndex(uint3 id, const uint3 size, const int multiply){
    const uint3 multiValue = uint3(1, size.x, size.x * size.y) * multiply;
    return dot(id, multiValue);
}

#endif