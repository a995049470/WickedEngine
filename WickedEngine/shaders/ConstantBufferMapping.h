#ifndef WI_CONSTANTBUFFER_MAPPING_H
#define WI_CONSTANTBUFFER_MAPPING_H

// Slot matchings:
////////////////////////////////////////////////////

// Common buffers:
// These are usable by all shaders
#define CBSLOT_RENDERER_FRAME					0
#define CBSLOT_RENDERER_CAMERA					1



// On demand buffers:
// These are bound on demand and alive until another is bound at the same slot

#define CBSLOT_RENDERER_MISC					5
#define CBSLOT_RENDERER_FORWARD_LIGHTMASK		7
#define CBSLOT_RENDERER_VOLUMELIGHT				7
#define CBSLOT_RENDERER_VOXELIZER				7
#define CBSLOT_RENDERER_TRACED					7
#define CBSLOT_RENDERER_BVH						7
#define CBSLOT_RENDERER_CUBEMAPRENDER			8

#define CBSLOT_OTHER_EMITTEDPARTICLE			7
#define CBSLOT_OTHER_HAIRPARTICLE				7
#define CBSLOT_OTHER_FFTGENERATOR				7
#define CBSLOT_OTHER_OCEAN_SIMULATION_IMMUTABLE	7
#define CBSLOT_OTHER_OCEAN_SIMULATION_PERFRAME	8
#define CBSLOT_OTHER_OCEAN_RENDER				7
#define CBSLOT_OTHER_CLOUDGENERATOR				7
#define CBSLOT_OTHER_GPUSORTLIB					8



#endif // WI_CONSTANTBUFFER_MAPPING_H
