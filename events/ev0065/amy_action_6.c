/* Amy's ev0065_amy_action_6 and its motion, then the bird's VER1_WING texlist,
 * model and seven actions, and its VER2_WING texlist. the two texlists list
 * the same twelve texture names. the last item, dat_0cb99194, is the material
 * of the first model in the data after this object. that model is a part of
 * the bird that ev0065_bird_action_7 moves */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0065/amy_action_6.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 7
#define TEXT_12_LENGTH 9
#include "ev0065/amy_action_6/text.inc"

extern NJS_MDATA2 dat_0cb8d470[];
extern NJS_OBJECT amy_object;

static NJS_MOTION ev0065_amy_motion_6 = {
#include "ev0065/amy_action_6/ev0065_amy_motion_6.inc"
};
NJS_ACTION ev0065_amy_action_6 = {
#include "ev0065/amy_action_6/ev0065_amy_action_6.inc"
};
static NJS_TEXNAME ev0065_ver1_wing_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0065_ver1_wing_texlist = {
#include "ev0065/amy_action_6/ev0065_ver1_wing_texlist.inc"
};
static NJS_MATERIAL dat_0cb8d9fc[1] = {
#include "ev0065/amy_action_6/dat_0cb8d9fc.inc"
};
static Sint16 dat_0cb8da10[6] = {
#include "ev0065/amy_action_6/dat_0cb8da10.inc"
};
static NJS_COLOR dat_0cb8da1c[4] = {
#include "ev0065/amy_action_6/dat_0cb8da1c.inc"
};
static NJS_MESHSET dat_0cb8da2c[1] = {
#include "ev0065/amy_action_6/dat_0cb8da2c.inc"
};
static NJS_POINT3 dat_0cb8da44[4] = {
#include "ev0065/amy_action_6/dat_0cb8da44.inc"
};
static NJS_VECTOR dat_0cb8da74[4] = {
#include "ev0065/amy_action_6/dat_0cb8da74.inc"
};
static NJS_MODEL dat_0cb8daa4 = {
#include "ev0065/amy_action_6/dat_0cb8daa4.inc"
};
static NJS_OBJECT dat_0cb8dacc = {
#include "ev0065/amy_action_6/dat_0cb8dacc.inc"
};
static NJS_MATERIAL dat_0cb8db00[1] = {
#include "ev0065/amy_action_6/dat_0cb8db00.inc"
};
static Sint16 dat_0cb8db14[12] = {
#include "ev0065/amy_action_6/dat_0cb8db14.inc"
};
static NJS_COLOR dat_0cb8db2c[9] = {
#include "ev0065/amy_action_6/dat_0cb8db2c.inc"
};
static NJS_MESHSET dat_0cb8db50[1] = {
#include "ev0065/amy_action_6/dat_0cb8db50.inc"
};
static NJS_POINT3 dat_0cb8db68[7] = {
#include "ev0065/amy_action_6/dat_0cb8db68.inc"
};
static NJS_VECTOR dat_0cb8dbbc[7] = {
#include "ev0065/amy_action_6/dat_0cb8dbbc.inc"
};
static NJS_MODEL dat_0cb8dc10 = {
#include "ev0065/amy_action_6/dat_0cb8dc10.inc"
};
static NJS_OBJECT dat_0cb8dc38 = {
#include "ev0065/amy_action_6/dat_0cb8dc38.inc"
};
static NJS_OBJECT dat_0cb8dc6c = {
#include "ev0065/amy_action_6/dat_0cb8dc6c.inc"
};
static NJS_OBJECT dat_0cb8dca0 = {
#include "ev0065/amy_action_6/dat_0cb8dca0.inc"
};
static NJS_OBJECT dat_0cb8dcd4 = {
#include "ev0065/amy_action_6/dat_0cb8dcd4.inc"
};
static NJS_OBJECT dat_0cb8dd08 = {
#include "ev0065/amy_action_6/dat_0cb8dd08.inc"
};
static NJS_OBJECT dat_0cb8dd3c = {
#include "ev0065/amy_action_6/dat_0cb8dd3c.inc"
};
static NJS_OBJECT dat_0cb8dd70 = {
#include "ev0065/amy_action_6/dat_0cb8dd70.inc"
};
static NJS_MATERIAL dat_0cb8dda4[1] = {
#include "ev0065/amy_action_6/dat_0cb8dda4.inc"
};
static Sint16 dat_0cb8ddb8[6] = {
#include "ev0065/amy_action_6/dat_0cb8ddb8.inc"
};
static NJS_COLOR dat_0cb8ddc4[4] = {
#include "ev0065/amy_action_6/dat_0cb8ddc4.inc"
};
static NJS_MESHSET dat_0cb8ddd4[1] = {
#include "ev0065/amy_action_6/dat_0cb8ddd4.inc"
};
static NJS_POINT3 dat_0cb8ddec[4] = {
#include "ev0065/amy_action_6/dat_0cb8ddec.inc"
};
static NJS_VECTOR dat_0cb8de1c[4] = {
#include "ev0065/amy_action_6/dat_0cb8de1c.inc"
};
static NJS_MODEL dat_0cb8de4c = {
#include "ev0065/amy_action_6/dat_0cb8de4c.inc"
};
static NJS_OBJECT dat_0cb8de74 = {
#include "ev0065/amy_action_6/dat_0cb8de74.inc"
};
static NJS_MATERIAL dat_0cb8dea8[1] = {
#include "ev0065/amy_action_6/dat_0cb8dea8.inc"
};
static Sint16 dat_0cb8debc[12] = {
#include "ev0065/amy_action_6/dat_0cb8debc.inc"
};
static NJS_COLOR dat_0cb8ded4[9] = {
#include "ev0065/amy_action_6/dat_0cb8ded4.inc"
};
static NJS_MESHSET dat_0cb8def8[1] = {
#include "ev0065/amy_action_6/dat_0cb8def8.inc"
};
static NJS_POINT3 dat_0cb8df10[7] = {
#include "ev0065/amy_action_6/dat_0cb8df10.inc"
};
static NJS_VECTOR dat_0cb8df64[7] = {
#include "ev0065/amy_action_6/dat_0cb8df64.inc"
};
static NJS_MODEL dat_0cb8dfb8 = {
#include "ev0065/amy_action_6/dat_0cb8dfb8.inc"
};
static NJS_OBJECT dat_0cb8dfe0 = {
#include "ev0065/amy_action_6/dat_0cb8dfe0.inc"
};
static NJS_OBJECT dat_0cb8e014 = {
#include "ev0065/amy_action_6/dat_0cb8e014.inc"
};
static NJS_OBJECT dat_0cb8e048 = {
#include "ev0065/amy_action_6/dat_0cb8e048.inc"
};
static NJS_OBJECT dat_0cb8e07c = {
#include "ev0065/amy_action_6/dat_0cb8e07c.inc"
};
static NJS_OBJECT dat_0cb8e0b0 = {
#include "ev0065/amy_action_6/dat_0cb8e0b0.inc"
};
static NJS_OBJECT dat_0cb8e0e4 = {
#include "ev0065/amy_action_6/dat_0cb8e0e4.inc"
};
static NJS_OBJECT dat_0cb8e118 = {
#include "ev0065/amy_action_6/dat_0cb8e118.inc"
};
static NJS_OBJECT dat_0cb8e14c = {
#include "ev0065/amy_action_6/dat_0cb8e14c.inc"
};
static NJS_OBJECT dat_0cb8e180 = {
#include "ev0065/amy_action_6/dat_0cb8e180.inc"
};
static NJS_OBJECT dat_0cb8e1b4 = {
#include "ev0065/amy_action_6/dat_0cb8e1b4.inc"
};
static NJS_OBJECT dat_0cb8e1e8 = {
#include "ev0065/amy_action_6/dat_0cb8e1e8.inc"
};
static NJS_OBJECT dat_0cb8e21c = {
#include "ev0065/amy_action_6/dat_0cb8e21c.inc"
};
static NJS_MATERIAL dat_0cb8e250[1] = {
#include "ev0065/amy_action_6/dat_0cb8e250.inc"
};
static Sint16 dat_0cb8e264[18] = {
#include "ev0065/amy_action_6/dat_0cb8e264.inc"
};
static NJS_COLOR dat_0cb8e288[15] = {
#include "ev0065/amy_action_6/dat_0cb8e288.inc"
};
static NJS_MESHSET dat_0cb8e2c4[1] = {
#include "ev0065/amy_action_6/dat_0cb8e2c4.inc"
};
static NJS_POINT3 dat_0cb8e2dc[7] = {
#include "ev0065/amy_action_6/dat_0cb8e2dc.inc"
};
static NJS_VECTOR dat_0cb8e330[7] = {
#include "ev0065/amy_action_6/dat_0cb8e330.inc"
};
static NJS_MODEL dat_0cb8e384 = {
#include "ev0065/amy_action_6/dat_0cb8e384.inc"
};
static NJS_OBJECT dat_0cb8e3ac = {
#include "ev0065/amy_action_6/dat_0cb8e3ac.inc"
};
static NJS_OBJECT dat_0cb8e3e0 = {
#include "ev0065/amy_action_6/dat_0cb8e3e0.inc"
};
static NJS_OBJECT dat_0cb8e414 = {
#include "ev0065/amy_action_6/dat_0cb8e414.inc"
};
static NJS_OBJECT dat_0cb8e448 = {
#include "ev0065/amy_action_6/dat_0cb8e448.inc"
};
static NJS_MATERIAL dat_0cb8e47c[1] = {
#include "ev0065/amy_action_6/dat_0cb8e47c.inc"
};
static Sint16 dat_0cb8e490[10] = {
#include "ev0065/amy_action_6/dat_0cb8e490.inc"
};
static NJS_COLOR dat_0cb8e4a4[8] = {
#include "ev0065/amy_action_6/dat_0cb8e4a4.inc"
};
static NJS_MESHSET dat_0cb8e4c4[1] = {
#include "ev0065/amy_action_6/dat_0cb8e4c4.inc"
};
static NJS_POINT3 dat_0cb8e4dc[8] = {
#include "ev0065/amy_action_6/dat_0cb8e4dc.inc"
};
static NJS_VECTOR dat_0cb8e53c[8] = {
#include "ev0065/amy_action_6/dat_0cb8e53c.inc"
};
static NJS_MODEL dat_0cb8e59c = {
#include "ev0065/amy_action_6/dat_0cb8e59c.inc"
};
static NJS_OBJECT dat_0cb8e5c4 = {
#include "ev0065/amy_action_6/dat_0cb8e5c4.inc"
};
static NJS_MATERIAL dat_0cb8e5f8[1] = {
#include "ev0065/amy_action_6/dat_0cb8e5f8.inc"
};
static Sint16 dat_0cb8e60c[10] = {
#include "ev0065/amy_action_6/dat_0cb8e60c.inc"
};
static NJS_COLOR dat_0cb8e620[8] = {
#include "ev0065/amy_action_6/dat_0cb8e620.inc"
};
static NJS_MESHSET dat_0cb8e640[1] = {
#include "ev0065/amy_action_6/dat_0cb8e640.inc"
};
static NJS_POINT3 dat_0cb8e658[8] = {
#include "ev0065/amy_action_6/dat_0cb8e658.inc"
};
static NJS_VECTOR dat_0cb8e6b8[8] = {
#include "ev0065/amy_action_6/dat_0cb8e6b8.inc"
};
static NJS_MODEL dat_0cb8e718 = {
#include "ev0065/amy_action_6/dat_0cb8e718.inc"
};
static NJS_OBJECT dat_0cb8e740 = {
#include "ev0065/amy_action_6/dat_0cb8e740.inc"
};
static NJS_MATERIAL dat_0cb8e774[1] = {
#include "ev0065/amy_action_6/dat_0cb8e774.inc"
};
static Sint16 dat_0cb8e788[16] = {
#include "ev0065/amy_action_6/dat_0cb8e788.inc"
};
static NJS_COLOR dat_0cb8e7a8[12] = {
#include "ev0065/amy_action_6/dat_0cb8e7a8.inc"
};
static NJS_MESHSET dat_0cb8e7d8[1] = {
#include "ev0065/amy_action_6/dat_0cb8e7d8.inc"
};
static NJS_POINT3 dat_0cb8e7f0[12] = {
#include "ev0065/amy_action_6/dat_0cb8e7f0.inc"
};
static NJS_VECTOR dat_0cb8e880[12] = {
#include "ev0065/amy_action_6/dat_0cb8e880.inc"
};
static NJS_MODEL dat_0cb8e910 = {
#include "ev0065/amy_action_6/dat_0cb8e910.inc"
};
static NJS_OBJECT dat_0cb8e938 = {
#include "ev0065/amy_action_6/dat_0cb8e938.inc"
};
static NJS_MATERIAL dat_0cb8e96c[1] = {
#include "ev0065/amy_action_6/dat_0cb8e96c.inc"
};
static Sint16 dat_0cb8e980[16] = {
#include "ev0065/amy_action_6/dat_0cb8e980.inc"
};
static NJS_COLOR dat_0cb8e9a0[12] = {
#include "ev0065/amy_action_6/dat_0cb8e9a0.inc"
};
static NJS_MESHSET dat_0cb8e9d0[1] = {
#include "ev0065/amy_action_6/dat_0cb8e9d0.inc"
};
static NJS_POINT3 dat_0cb8e9e8[12] = {
#include "ev0065/amy_action_6/dat_0cb8e9e8.inc"
};
static NJS_VECTOR dat_0cb8ea78[12] = {
#include "ev0065/amy_action_6/dat_0cb8ea78.inc"
};
static NJS_MODEL dat_0cb8eb08 = {
#include "ev0065/amy_action_6/dat_0cb8eb08.inc"
};
static NJS_OBJECT dat_0cb8eb30 = {
#include "ev0065/amy_action_6/dat_0cb8eb30.inc"
};
static NJS_MATERIAL dat_0cb8eb64[4] = {
#include "ev0065/amy_action_6/dat_0cb8eb64.inc"
};
static Sint16 dat_0cb8ebb4[239] = {
#include "ev0065/amy_action_6/dat_0cb8ebb4.inc"
};
static Sint16 dat_0cb8ed92[22] = {
#include "ev0065/amy_action_6/dat_0cb8ed92.inc"
};
static Sint16 dat_0cb8edbe[21] = {
#include "ev0065/amy_action_6/dat_0cb8edbe.inc"
};
static Sint16 dat_0cb8ede8[22] = {
#include "ev0065/amy_action_6/dat_0cb8ede8.inc"
};
static NJS_COLOR dat_0cb8ee14[209] = {
#include "ev0065/amy_action_6/dat_0cb8ee14.inc"
};
static NJS_COLOR dat_0cb8f158[20] = {
#include "ev0065/amy_action_6/dat_0cb8f158.inc"
};
static NJS_COLOR dat_0cb8f1a8[16] = {
#include "ev0065/amy_action_6/dat_0cb8f1a8.inc"
};
static NJS_COLOR dat_0cb8f1e8[18] = {
#include "ev0065/amy_action_6/dat_0cb8f1e8.inc"
};
static NJS_MESHSET dat_0cb8f230[4] = {
#include "ev0065/amy_action_6/dat_0cb8f230.inc"
};
static NJS_POINT3 dat_0cb8f290[127] = {
#include "ev0065/amy_action_6/dat_0cb8f290.inc"
};
static NJS_VECTOR dat_0cb8f884[127] = {
#include "ev0065/amy_action_6/dat_0cb8f884.inc"
};
static NJS_MODEL dat_0cb8fe78 = {
#include "ev0065/amy_action_6/dat_0cb8fe78.inc"
};
static NJS_OBJECT dat_0cb8fea0 = {
#include "ev0065/amy_action_6/dat_0cb8fea0.inc"
};
static NJS_OBJECT dat_0cb8fed4 = {
#include "ev0065/amy_action_6/dat_0cb8fed4.inc"
};
static NJS_OBJECT dat_0cb8ff08 = {
#include "ev0065/amy_action_6/dat_0cb8ff08.inc"
};
static NJS_OBJECT dat_0cb8ff3c = {
#include "ev0065/amy_action_6/dat_0cb8ff3c.inc"
};
static NJS_MATERIAL dat_0cb8ff70[1] = {
#include "ev0065/amy_action_6/dat_0cb8ff70.inc"
};
static Sint16 dat_0cb8ff84[8] = {
#include "ev0065/amy_action_6/dat_0cb8ff84.inc"
};
static NJS_COLOR dat_0cb8ff94[6] = {
#include "ev0065/amy_action_6/dat_0cb8ff94.inc"
};
static NJS_MESHSET dat_0cb8ffac[1] = {
#include "ev0065/amy_action_6/dat_0cb8ffac.inc"
};
static NJS_POINT3 dat_0cb8ffc4[6] = {
#include "ev0065/amy_action_6/dat_0cb8ffc4.inc"
};
static NJS_VECTOR dat_0cb9000c[6] = {
#include "ev0065/amy_action_6/dat_0cb9000c.inc"
};
static NJS_MODEL dat_0cb90054 = {
#include "ev0065/amy_action_6/dat_0cb90054.inc"
};
static NJS_OBJECT dat_0cb9007c = {
#include "ev0065/amy_action_6/dat_0cb9007c.inc"
};
static NJS_MATERIAL dat_0cb900b0[1] = {
#include "ev0065/amy_action_6/dat_0cb900b0.inc"
};
static Sint16 dat_0cb900c4[22] = {
#include "ev0065/amy_action_6/dat_0cb900c4.inc"
};
static NJS_COLOR dat_0cb900f0[16] = {
#include "ev0065/amy_action_6/dat_0cb900f0.inc"
};
static NJS_MESHSET dat_0cb90130[1] = {
#include "ev0065/amy_action_6/dat_0cb90130.inc"
};
static NJS_POINT3 dat_0cb90148[5] = {
#include "ev0065/amy_action_6/dat_0cb90148.inc"
};
static NJS_VECTOR dat_0cb90184[5] = {
#include "ev0065/amy_action_6/dat_0cb90184.inc"
};
static NJS_MODEL dat_0cb901c0 = {
#include "ev0065/amy_action_6/dat_0cb901c0.inc"
};
static NJS_OBJECT dat_0cb901e8 = {
#include "ev0065/amy_action_6/dat_0cb901e8.inc"
};
static NJS_MATERIAL dat_0cb9021c[1] = {
#include "ev0065/amy_action_6/dat_0cb9021c.inc"
};
static Sint16 dat_0cb90230[30] = {
#include "ev0065/amy_action_6/dat_0cb90230.inc"
};
static NJS_COLOR dat_0cb9026c[28] = {
#include "ev0065/amy_action_6/dat_0cb9026c.inc"
};
static NJS_MESHSET dat_0cb902dc[1] = {
#include "ev0065/amy_action_6/dat_0cb902dc.inc"
};
static NJS_POINT3 dat_0cb902f4[18] = {
#include "ev0065/amy_action_6/dat_0cb902f4.inc"
};
static NJS_VECTOR dat_0cb903cc[18] = {
#include "ev0065/amy_action_6/dat_0cb903cc.inc"
};
static NJS_MODEL dat_0cb904a4 = {
#include "ev0065/amy_action_6/dat_0cb904a4.inc"
};
static NJS_OBJECT dat_0cb904cc = {
#include "ev0065/amy_action_6/dat_0cb904cc.inc"
};
static NJS_MATERIAL dat_0cb90500[1] = {
#include "ev0065/amy_action_6/dat_0cb90500.inc"
};
static Sint16 dat_0cb90514[20] = {
#include "ev0065/amy_action_6/dat_0cb90514.inc"
};
static NJS_COLOR dat_0cb9053c[16] = {
#include "ev0065/amy_action_6/dat_0cb9053c.inc"
};
static NJS_MESHSET dat_0cb9057c[1] = {
#include "ev0065/amy_action_6/dat_0cb9057c.inc"
};
static NJS_POINT3 dat_0cb90594[12] = {
#include "ev0065/amy_action_6/dat_0cb90594.inc"
};
static NJS_VECTOR dat_0cb90624[12] = {
#include "ev0065/amy_action_6/dat_0cb90624.inc"
};
static NJS_MODEL dat_0cb906b4 = {
#include "ev0065/amy_action_6/dat_0cb906b4.inc"
};
static NJS_OBJECT dat_0cb906dc = {
#include "ev0065/amy_action_6/dat_0cb906dc.inc"
};
static NJS_MATERIAL dat_0cb90710[1] = {
#include "ev0065/amy_action_6/dat_0cb90710.inc"
};
static Sint16 dat_0cb90724[10] = {
#include "ev0065/amy_action_6/dat_0cb90724.inc"
};
static NJS_COLOR dat_0cb90738[8] = {
#include "ev0065/amy_action_6/dat_0cb90738.inc"
};
static NJS_MESHSET dat_0cb90758[1] = {
#include "ev0065/amy_action_6/dat_0cb90758.inc"
};
static NJS_POINT3 dat_0cb90770[5] = {
#include "ev0065/amy_action_6/dat_0cb90770.inc"
};
static NJS_VECTOR dat_0cb907ac[5] = {
#include "ev0065/amy_action_6/dat_0cb907ac.inc"
};
static NJS_MODEL dat_0cb907e8 = {
#include "ev0065/amy_action_6/dat_0cb907e8.inc"
};
static NJS_OBJECT dat_0cb90810 = {
#include "ev0065/amy_action_6/dat_0cb90810.inc"
};
static NJS_MATERIAL dat_0cb90844[1] = {
#include "ev0065/amy_action_6/dat_0cb90844.inc"
};
static Sint16 dat_0cb90858[10] = {
#include "ev0065/amy_action_6/dat_0cb90858.inc"
};
static NJS_COLOR dat_0cb9086c[8] = {
#include "ev0065/amy_action_6/dat_0cb9086c.inc"
};
static NJS_MESHSET dat_0cb9088c[1] = {
#include "ev0065/amy_action_6/dat_0cb9088c.inc"
};
static NJS_POINT3 dat_0cb908a4[5] = {
#include "ev0065/amy_action_6/dat_0cb908a4.inc"
};
static NJS_VECTOR dat_0cb908e0[5] = {
#include "ev0065/amy_action_6/dat_0cb908e0.inc"
};
static NJS_MODEL dat_0cb9091c = {
#include "ev0065/amy_action_6/dat_0cb9091c.inc"
};
static NJS_OBJECT dat_0cb90944 = {
#include "ev0065/amy_action_6/dat_0cb90944.inc"
};
static NJS_MATERIAL dat_0cb90978[1] = {
#include "ev0065/amy_action_6/dat_0cb90978.inc"
};
static Sint16 dat_0cb9098c[24] = {
#include "ev0065/amy_action_6/dat_0cb9098c.inc"
};
static NJS_COLOR dat_0cb909bc[20] = {
#include "ev0065/amy_action_6/dat_0cb909bc.inc"
};
static NJS_MESHSET dat_0cb90a0c[1] = {
#include "ev0065/amy_action_6/dat_0cb90a0c.inc"
};
static NJS_POINT3 dat_0cb90a24[12] = {
#include "ev0065/amy_action_6/dat_0cb90a24.inc"
};
static NJS_VECTOR dat_0cb90ab4[12] = {
#include "ev0065/amy_action_6/dat_0cb90ab4.inc"
};
static NJS_MODEL dat_0cb90b44 = {
#include "ev0065/amy_action_6/dat_0cb90b44.inc"
};
static NJS_OBJECT dat_0cb90b6c = {
#include "ev0065/amy_action_6/dat_0cb90b6c.inc"
};
static NJS_MATERIAL dat_0cb90ba0[1] = {
#include "ev0065/amy_action_6/dat_0cb90ba0.inc"
};
static Sint16 dat_0cb90bb4[36] = {
#include "ev0065/amy_action_6/dat_0cb90bb4.inc"
};
static NJS_COLOR dat_0cb90bfc[30] = {
#include "ev0065/amy_action_6/dat_0cb90bfc.inc"
};
static NJS_MESHSET dat_0cb90c74[1] = {
#include "ev0065/amy_action_6/dat_0cb90c74.inc"
};
static NJS_POINT3 dat_0cb90c8c[12] = {
#include "ev0065/amy_action_6/dat_0cb90c8c.inc"
};
static NJS_VECTOR dat_0cb90d1c[12] = {
#include "ev0065/amy_action_6/dat_0cb90d1c.inc"
};
static NJS_MODEL dat_0cb90dac = {
#include "ev0065/amy_action_6/dat_0cb90dac.inc"
};
static NJS_OBJECT dat_0cb90dd4 = {
#include "ev0065/amy_action_6/dat_0cb90dd4.inc"
};
static NJS_OBJECT dat_0cb90e08 = {
#include "ev0065/amy_action_6/dat_0cb90e08.inc"
};
static NJS_OBJECT dat_0cb90e3c = {
#include "ev0065/amy_action_6/dat_0cb90e3c.inc"
};
static NJS_OBJECT dat_0cb90e70 = {
#include "ev0065/amy_action_6/dat_0cb90e70.inc"
};
static NJS_MATERIAL dat_0cb90ea4[1] = {
#include "ev0065/amy_action_6/dat_0cb90ea4.inc"
};
static Sint16 dat_0cb90eb8[48] = {
#include "ev0065/amy_action_6/dat_0cb90eb8.inc"
};
static NJS_COLOR dat_0cb90f18[38] = {
#include "ev0065/amy_action_6/dat_0cb90f18.inc"
};
static NJS_MESHSET dat_0cb90fb0[1] = {
#include "ev0065/amy_action_6/dat_0cb90fb0.inc"
};
static NJS_POINT3 dat_0cb90fc8[12] = {
#include "ev0065/amy_action_6/dat_0cb90fc8.inc"
};
static NJS_VECTOR dat_0cb91058[12] = {
#include "ev0065/amy_action_6/dat_0cb91058.inc"
};
static NJS_MODEL dat_0cb910e8 = {
#include "ev0065/amy_action_6/dat_0cb910e8.inc"
};
static NJS_OBJECT dat_0cb91110 = {
#include "ev0065/amy_action_6/dat_0cb91110.inc"
};
static NJS_OBJECT dat_0cb91144 = {
#include "ev0065/amy_action_6/dat_0cb91144.inc"
};
static NJS_OBJECT dat_0cb91178 = {
#include "ev0065/amy_action_6/dat_0cb91178.inc"
};
static NJS_OBJECT dat_0cb911ac = {
#include "ev0065/amy_action_6/dat_0cb911ac.inc"
};
static NJS_MATERIAL dat_0cb911e0[1] = {
#include "ev0065/amy_action_6/dat_0cb911e0.inc"
};
static Sint16 dat_0cb911f4[48] = {
#include "ev0065/amy_action_6/dat_0cb911f4.inc"
};
static NJS_COLOR dat_0cb91254[38] = {
#include "ev0065/amy_action_6/dat_0cb91254.inc"
};
static NJS_MESHSET dat_0cb912ec[1] = {
#include "ev0065/amy_action_6/dat_0cb912ec.inc"
};
static NJS_POINT3 dat_0cb91304[12] = {
#include "ev0065/amy_action_6/dat_0cb91304.inc"
};
static NJS_VECTOR dat_0cb91394[12] = {
#include "ev0065/amy_action_6/dat_0cb91394.inc"
};
static NJS_MODEL dat_0cb91424 = {
#include "ev0065/amy_action_6/dat_0cb91424.inc"
};
static NJS_OBJECT dat_0cb9144c = {
#include "ev0065/amy_action_6/dat_0cb9144c.inc"
};
static NJS_OBJECT dat_0cb91480 = {
#include "ev0065/amy_action_6/dat_0cb91480.inc"
};
static NJS_OBJECT dat_0cb914b4 = {
#include "ev0065/amy_action_6/dat_0cb914b4.inc"
};
static NJS_OBJECT dat_0cb914e8 = {
#include "ev0065/amy_action_6/dat_0cb914e8.inc"
};
static NJS_MATERIAL dat_0cb9151c[2] = {
#include "ev0065/amy_action_6/dat_0cb9151c.inc"
};
static Sint16 dat_0cb91544[54] = {
#include "ev0065/amy_action_6/dat_0cb91544.inc"
};
static Sint16 dat_0cb915b0[22] = {
#include "ev0065/amy_action_6/dat_0cb915b0.inc"
};
static NJS_COLOR dat_0cb915dc[48] = {
#include "ev0065/amy_action_6/dat_0cb915dc.inc"
};
static NJS_COLOR dat_0cb9169c[18] = {
#include "ev0065/amy_action_6/dat_0cb9169c.inc"
};
static NJS_MESHSET dat_0cb916e4[2] = {
#include "ev0065/amy_action_6/dat_0cb916e4.inc"
};
static NJS_POINT3 dat_0cb91714[26] = {
#include "ev0065/amy_action_6/dat_0cb91714.inc"
};
static NJS_VECTOR dat_0cb9184c[26] = {
#include "ev0065/amy_action_6/dat_0cb9184c.inc"
};
static NJS_MODEL dat_0cb91984 = {
#include "ev0065/amy_action_6/dat_0cb91984.inc"
};
static NJS_OBJECT dat_0cb919ac = {
#include "ev0065/amy_action_6/dat_0cb919ac.inc"
};
static NJS_OBJECT dat_0cb919e0 = {
#include "ev0065/amy_action_6/dat_0cb919e0.inc"
};
static NJS_OBJECT dat_0cb91a14 = {
#include "ev0065/amy_action_6/dat_0cb91a14.inc"
};
static NJS_OBJECT dat_0cb91a48 = {
#include "ev0065/amy_action_6/dat_0cb91a48.inc"
};
static NJS_OBJECT dat_0cb91a7c = {
#include "ev0065/amy_action_6/dat_0cb91a7c.inc"
};
static NJS_MKEY_F dat_0cb91ab0[2] = {
#include "ev0065/amy_action_6/dat_0cb91ab0.inc"
};
static NJS_MKEY_A dat_0cb91ad0[2] = {
#include "ev0065/amy_action_6/dat_0cb91ad0.inc"
};
static NJS_MKEY_F dat_0cb91af0[2] = {
#include "ev0065/amy_action_6/dat_0cb91af0.inc"
};
static NJS_MKEY_F dat_0cb91b10[2] = {
#include "ev0065/amy_action_6/dat_0cb91b10.inc"
};
static NJS_MKEY_A dat_0cb91b30[2] = {
#include "ev0065/amy_action_6/dat_0cb91b30.inc"
};
static NJS_MKEY_A dat_0cb91b50[2] = {
#include "ev0065/amy_action_6/dat_0cb91b50.inc"
};
static NJS_MKEY_F dat_0cb91b70[2] = {
#include "ev0065/amy_action_6/dat_0cb91b70.inc"
};
static NJS_MKEY_F dat_0cb91b90[2] = {
#include "ev0065/amy_action_6/dat_0cb91b90.inc"
};
static NJS_MKEY_A dat_0cb91bb0[2] = {
#include "ev0065/amy_action_6/dat_0cb91bb0.inc"
};
static NJS_MKEY_A dat_0cb91bd0[2] = {
#include "ev0065/amy_action_6/dat_0cb91bd0.inc"
};
static NJS_MKEY_F dat_0cb91bf0[2] = {
#include "ev0065/amy_action_6/dat_0cb91bf0.inc"
};
static NJS_MKEY_F dat_0cb91c10[2] = {
#include "ev0065/amy_action_6/dat_0cb91c10.inc"
};
static NJS_MKEY_A dat_0cb91c30[2] = {
#include "ev0065/amy_action_6/dat_0cb91c30.inc"
};
static NJS_MKEY_A dat_0cb91c50[2] = {
#include "ev0065/amy_action_6/dat_0cb91c50.inc"
};
static NJS_MKEY_F dat_0cb91c70[2] = {
#include "ev0065/amy_action_6/dat_0cb91c70.inc"
};
static NJS_MKEY_F dat_0cb91c90[2] = {
#include "ev0065/amy_action_6/dat_0cb91c90.inc"
};
static NJS_MKEY_A dat_0cb91cb0[2] = {
#include "ev0065/amy_action_6/dat_0cb91cb0.inc"
};
static NJS_MKEY_F dat_0cb91cd0[2] = {
#include "ev0065/amy_action_6/dat_0cb91cd0.inc"
};
static NJS_MKEY_A dat_0cb91cf0[2] = {
#include "ev0065/amy_action_6/dat_0cb91cf0.inc"
};
static NJS_MKEY_F dat_0cb91d10[2] = {
#include "ev0065/amy_action_6/dat_0cb91d10.inc"
};
static NJS_MKEY_A dat_0cb91d30[2] = {
#include "ev0065/amy_action_6/dat_0cb91d30.inc"
};
static NJS_MKEY_F dat_0cb91d50[2] = {
#include "ev0065/amy_action_6/dat_0cb91d50.inc"
};
static NJS_MKEY_A dat_0cb91d70[2] = {
#include "ev0065/amy_action_6/dat_0cb91d70.inc"
};
static NJS_MKEY_A dat_0cb91d90[2] = {
#include "ev0065/amy_action_6/dat_0cb91d90.inc"
};
static NJS_MKEY_F dat_0cb91db0[2] = {
#include "ev0065/amy_action_6/dat_0cb91db0.inc"
};
static NJS_MKEY_F dat_0cb91dd0[2] = {
#include "ev0065/amy_action_6/dat_0cb91dd0.inc"
};
static NJS_MKEY_A dat_0cb91df0[2] = {
#include "ev0065/amy_action_6/dat_0cb91df0.inc"
};
static NJS_MKEY_F dat_0cb91e10[2] = {
#include "ev0065/amy_action_6/dat_0cb91e10.inc"
};
static NJS_MKEY_A dat_0cb91e30[2] = {
#include "ev0065/amy_action_6/dat_0cb91e30.inc"
};
static NJS_MKEY_F dat_0cb91e50[2] = {
#include "ev0065/amy_action_6/dat_0cb91e50.inc"
};
static NJS_MKEY_A dat_0cb91e70[2] = {
#include "ev0065/amy_action_6/dat_0cb91e70.inc"
};
static NJS_MKEY_F dat_0cb91e90[2] = {
#include "ev0065/amy_action_6/dat_0cb91e90.inc"
};
static NJS_MKEY_A dat_0cb91eb0[2] = {
#include "ev0065/amy_action_6/dat_0cb91eb0.inc"
};
static NJS_MKEY_F dat_0cb91ed0[2] = {
#include "ev0065/amy_action_6/dat_0cb91ed0.inc"
};
static NJS_MKEY_A dat_0cb91ef0[2] = {
#include "ev0065/amy_action_6/dat_0cb91ef0.inc"
};
static NJS_MKEY_A dat_0cb91f10[2] = {
#include "ev0065/amy_action_6/dat_0cb91f10.inc"
};
static NJS_MKEY_F dat_0cb91f30[2] = {
#include "ev0065/amy_action_6/dat_0cb91f30.inc"
};
static NJS_MKEY_F dat_0cb91f50[2] = {
#include "ev0065/amy_action_6/dat_0cb91f50.inc"
};
static NJS_MKEY_A dat_0cb91f70[2] = {
#include "ev0065/amy_action_6/dat_0cb91f70.inc"
};
static NJS_MKEY_F dat_0cb91f90[2] = {
#include "ev0065/amy_action_6/dat_0cb91f90.inc"
};
static NJS_MKEY_A dat_0cb91fb0[2] = {
#include "ev0065/amy_action_6/dat_0cb91fb0.inc"
};
static NJS_MKEY_F dat_0cb91fd0[2] = {
#include "ev0065/amy_action_6/dat_0cb91fd0.inc"
};
static NJS_MKEY_A dat_0cb91ff0[2] = {
#include "ev0065/amy_action_6/dat_0cb91ff0.inc"
};
static NJS_MKEY_F dat_0cb92010[2] = {
#include "ev0065/amy_action_6/dat_0cb92010.inc"
};
static NJS_MKEY_A dat_0cb92030[2] = {
#include "ev0065/amy_action_6/dat_0cb92030.inc"
};
static NJS_MKEY_F dat_0cb92050[2] = {
#include "ev0065/amy_action_6/dat_0cb92050.inc"
};
static NJS_MKEY_A dat_0cb92070[2] = {
#include "ev0065/amy_action_6/dat_0cb92070.inc"
};
static NJS_MKEY_F dat_0cb92090[2] = {
#include "ev0065/amy_action_6/dat_0cb92090.inc"
};
static NJS_MKEY_A dat_0cb920b0[2] = {
#include "ev0065/amy_action_6/dat_0cb920b0.inc"
};
static NJS_MKEY_F dat_0cb920d0[2] = {
#include "ev0065/amy_action_6/dat_0cb920d0.inc"
};
static NJS_MKEY_A dat_0cb920f0[2] = {
#include "ev0065/amy_action_6/dat_0cb920f0.inc"
};
static NJS_MKEY_F dat_0cb92110[2] = {
#include "ev0065/amy_action_6/dat_0cb92110.inc"
};
static NJS_MKEY_F dat_0cb92130[2] = {
#include "ev0065/amy_action_6/dat_0cb92130.inc"
};
static NJS_MKEY_F dat_0cb92150[2] = {
#include "ev0065/amy_action_6/dat_0cb92150.inc"
};
static NJS_MKEY_A dat_0cb92170[2] = {
#include "ev0065/amy_action_6/dat_0cb92170.inc"
};
static NJS_MKEY_F dat_0cb92190[2] = {
#include "ev0065/amy_action_6/dat_0cb92190.inc"
};
static NJS_MKEY_A dat_0cb921b0[2] = {
#include "ev0065/amy_action_6/dat_0cb921b0.inc"
};
static NJS_MKEY_F dat_0cb921d0[2] = {
#include "ev0065/amy_action_6/dat_0cb921d0.inc"
};
static NJS_MDATA2 dat_0cb921f0[57] = {
#include "ev0065/amy_action_6/dat_0cb921f0.inc"
};
static NJS_MOTION ev0065_bird_motion_0 = {
#include "ev0065/amy_action_6/ev0065_bird_motion_0.inc"
};
NJS_ACTION ev0065_bird_action_0 = {
#include "ev0065/amy_action_6/ev0065_bird_action_0.inc"
};
static NJS_MKEY_F dat_0cb92594[2] = {
#include "ev0065/amy_action_6/dat_0cb92594.inc"
};
static NJS_MKEY_A dat_0cb925b4[2] = {
#include "ev0065/amy_action_6/dat_0cb925b4.inc"
};
static NJS_MKEY_F dat_0cb925d4[2] = {
#include "ev0065/amy_action_6/dat_0cb925d4.inc"
};
static NJS_MKEY_F dat_0cb925f4[2] = {
#include "ev0065/amy_action_6/dat_0cb925f4.inc"
};
static NJS_MKEY_A dat_0cb92614[2] = {
#include "ev0065/amy_action_6/dat_0cb92614.inc"
};
static NJS_MKEY_A dat_0cb92634[2] = {
#include "ev0065/amy_action_6/dat_0cb92634.inc"
};
static NJS_MKEY_F dat_0cb92654[2] = {
#include "ev0065/amy_action_6/dat_0cb92654.inc"
};
static NJS_MKEY_F dat_0cb92674[2] = {
#include "ev0065/amy_action_6/dat_0cb92674.inc"
};
static NJS_MKEY_A dat_0cb92694[2] = {
#include "ev0065/amy_action_6/dat_0cb92694.inc"
};
static NJS_MKEY_A dat_0cb926b4[2] = {
#include "ev0065/amy_action_6/dat_0cb926b4.inc"
};
static NJS_MKEY_F dat_0cb926d4[2] = {
#include "ev0065/amy_action_6/dat_0cb926d4.inc"
};
static NJS_MKEY_F dat_0cb926f4[2] = {
#include "ev0065/amy_action_6/dat_0cb926f4.inc"
};
static NJS_MKEY_A dat_0cb92714[2] = {
#include "ev0065/amy_action_6/dat_0cb92714.inc"
};
static NJS_MKEY_A dat_0cb92734[2] = {
#include "ev0065/amy_action_6/dat_0cb92734.inc"
};
static NJS_MKEY_F dat_0cb92754[2] = {
#include "ev0065/amy_action_6/dat_0cb92754.inc"
};
static NJS_MKEY_F dat_0cb92774[2] = {
#include "ev0065/amy_action_6/dat_0cb92774.inc"
};
static NJS_MKEY_A dat_0cb92794[2] = {
#include "ev0065/amy_action_6/dat_0cb92794.inc"
};
static NJS_MKEY_F dat_0cb927b4[2] = {
#include "ev0065/amy_action_6/dat_0cb927b4.inc"
};
static NJS_MKEY_A dat_0cb927d4[2] = {
#include "ev0065/amy_action_6/dat_0cb927d4.inc"
};
static NJS_MKEY_F dat_0cb927f4[2] = {
#include "ev0065/amy_action_6/dat_0cb927f4.inc"
};
static NJS_MKEY_A dat_0cb92814[2] = {
#include "ev0065/amy_action_6/dat_0cb92814.inc"
};
static NJS_MKEY_F dat_0cb92834[2] = {
#include "ev0065/amy_action_6/dat_0cb92834.inc"
};
static NJS_MKEY_A dat_0cb92854[2] = {
#include "ev0065/amy_action_6/dat_0cb92854.inc"
};
static NJS_MKEY_A dat_0cb92874[29] = {
#include "ev0065/amy_action_6/dat_0cb92874.inc"
};
static NJS_MKEY_F dat_0cb92a44[2] = {
#include "ev0065/amy_action_6/dat_0cb92a44.inc"
};
static NJS_MKEY_F dat_0cb92a64[2] = {
#include "ev0065/amy_action_6/dat_0cb92a64.inc"
};
static NJS_MKEY_A dat_0cb92a84[2] = {
#include "ev0065/amy_action_6/dat_0cb92a84.inc"
};
static NJS_MKEY_F dat_0cb92aa4[2] = {
#include "ev0065/amy_action_6/dat_0cb92aa4.inc"
};
static NJS_MKEY_A dat_0cb92ac4[2] = {
#include "ev0065/amy_action_6/dat_0cb92ac4.inc"
};
static NJS_MKEY_F dat_0cb92ae4[2] = {
#include "ev0065/amy_action_6/dat_0cb92ae4.inc"
};
static NJS_MKEY_A dat_0cb92b04[2] = {
#include "ev0065/amy_action_6/dat_0cb92b04.inc"
};
static NJS_MKEY_F dat_0cb92b24[2] = {
#include "ev0065/amy_action_6/dat_0cb92b24.inc"
};
static NJS_MKEY_A dat_0cb92b44[2] = {
#include "ev0065/amy_action_6/dat_0cb92b44.inc"
};
static NJS_MKEY_F dat_0cb92b64[2] = {
#include "ev0065/amy_action_6/dat_0cb92b64.inc"
};
static NJS_MKEY_A dat_0cb92b84[2] = {
#include "ev0065/amy_action_6/dat_0cb92b84.inc"
};
static NJS_MKEY_A dat_0cb92ba4[2] = {
#include "ev0065/amy_action_6/dat_0cb92ba4.inc"
};
static NJS_MKEY_F dat_0cb92bc4[2] = {
#include "ev0065/amy_action_6/dat_0cb92bc4.inc"
};
static NJS_MKEY_F dat_0cb92be4[2] = {
#include "ev0065/amy_action_6/dat_0cb92be4.inc"
};
static NJS_MKEY_A dat_0cb92c04[2] = {
#include "ev0065/amy_action_6/dat_0cb92c04.inc"
};
static NJS_MKEY_F dat_0cb92c24[2] = {
#include "ev0065/amy_action_6/dat_0cb92c24.inc"
};
static NJS_MKEY_A dat_0cb92c44[2] = {
#include "ev0065/amy_action_6/dat_0cb92c44.inc"
};
static NJS_MKEY_F dat_0cb92c64[2] = {
#include "ev0065/amy_action_6/dat_0cb92c64.inc"
};
static NJS_MKEY_A dat_0cb92c84[2] = {
#include "ev0065/amy_action_6/dat_0cb92c84.inc"
};
static NJS_MKEY_F dat_0cb92ca4[29] = {
#include "ev0065/amy_action_6/dat_0cb92ca4.inc"
};
static NJS_MKEY_A dat_0cb92e74[2] = {
#include "ev0065/amy_action_6/dat_0cb92e74.inc"
};
static NJS_MKEY_F dat_0cb92e94[2] = {
#include "ev0065/amy_action_6/dat_0cb92e94.inc"
};
static NJS_MKEY_A dat_0cb92eb4[2] = {
#include "ev0065/amy_action_6/dat_0cb92eb4.inc"
};
static NJS_MKEY_F dat_0cb92ed4[2] = {
#include "ev0065/amy_action_6/dat_0cb92ed4.inc"
};
static NJS_MKEY_A dat_0cb92ef4[2] = {
#include "ev0065/amy_action_6/dat_0cb92ef4.inc"
};
static NJS_MKEY_F dat_0cb92f14[2] = {
#include "ev0065/amy_action_6/dat_0cb92f14.inc"
};
static NJS_MKEY_A dat_0cb92f34[2] = {
#include "ev0065/amy_action_6/dat_0cb92f34.inc"
};
static NJS_MKEY_F dat_0cb92f54[2] = {
#include "ev0065/amy_action_6/dat_0cb92f54.inc"
};
static NJS_MKEY_F dat_0cb92f74[2] = {
#include "ev0065/amy_action_6/dat_0cb92f74.inc"
};
static NJS_MKEY_F dat_0cb92f94[2] = {
#include "ev0065/amy_action_6/dat_0cb92f94.inc"
};
static NJS_MKEY_A dat_0cb92fb4[2] = {
#include "ev0065/amy_action_6/dat_0cb92fb4.inc"
};
static NJS_MKEY_F dat_0cb92fd4[2] = {
#include "ev0065/amy_action_6/dat_0cb92fd4.inc"
};
static NJS_MKEY_A dat_0cb92ff4[2] = {
#include "ev0065/amy_action_6/dat_0cb92ff4.inc"
};
static NJS_MKEY_F dat_0cb93014[2] = {
#include "ev0065/amy_action_6/dat_0cb93014.inc"
};
static NJS_MDATA2 dat_0cb93034[57] = {
#include "ev0065/amy_action_6/dat_0cb93034.inc"
};
static NJS_MOTION ev0065_bird_motion_1 = {
#include "ev0065/amy_action_6/ev0065_bird_motion_1.inc"
};
NJS_ACTION ev0065_bird_action_1 = {
#include "ev0065/amy_action_6/ev0065_bird_action_1.inc"
};
static NJS_MKEY_F dat_0cb933d8[21] = {
#include "ev0065/amy_action_6/dat_0cb933d8.inc"
};
static NJS_MKEY_A dat_0cb93528[21] = {
#include "ev0065/amy_action_6/dat_0cb93528.inc"
};
static NJS_MKEY_A dat_0cb93678[21] = {
#include "ev0065/amy_action_6/dat_0cb93678.inc"
};
static NJS_MKEY_F dat_0cb937c8[2] = {
#include "ev0065/amy_action_6/dat_0cb937c8.inc"
};
static NJS_MKEY_F dat_0cb937e8[2] = {
#include "ev0065/amy_action_6/dat_0cb937e8.inc"
};
static NJS_MKEY_A dat_0cb93808[2] = {
#include "ev0065/amy_action_6/dat_0cb93808.inc"
};
static NJS_MKEY_A dat_0cb93828[21] = {
#include "ev0065/amy_action_6/dat_0cb93828.inc"
};
static NJS_MKEY_F dat_0cb93978[2] = {
#include "ev0065/amy_action_6/dat_0cb93978.inc"
};
static NJS_MKEY_F dat_0cb93998[2] = {
#include "ev0065/amy_action_6/dat_0cb93998.inc"
};
static NJS_MKEY_A dat_0cb939b8[2] = {
#include "ev0065/amy_action_6/dat_0cb939b8.inc"
};
static NJS_MKEY_A dat_0cb939d8[21] = {
#include "ev0065/amy_action_6/dat_0cb939d8.inc"
};
static NJS_MKEY_F dat_0cb93b28[2] = {
#include "ev0065/amy_action_6/dat_0cb93b28.inc"
};
static NJS_MKEY_F dat_0cb93b48[2] = {
#include "ev0065/amy_action_6/dat_0cb93b48.inc"
};
static NJS_MKEY_A dat_0cb93b68[2] = {
#include "ev0065/amy_action_6/dat_0cb93b68.inc"
};
static NJS_MKEY_A dat_0cb93b88[2] = {
#include "ev0065/amy_action_6/dat_0cb93b88.inc"
};
static NJS_MKEY_F dat_0cb93ba8[2] = {
#include "ev0065/amy_action_6/dat_0cb93ba8.inc"
};
static NJS_MKEY_F dat_0cb93bc8[2] = {
#include "ev0065/amy_action_6/dat_0cb93bc8.inc"
};
static NJS_MKEY_A dat_0cb93be8[2] = {
#include "ev0065/amy_action_6/dat_0cb93be8.inc"
};
static NJS_MKEY_F dat_0cb93c08[2] = {
#include "ev0065/amy_action_6/dat_0cb93c08.inc"
};
static NJS_MKEY_A dat_0cb93c28[2] = {
#include "ev0065/amy_action_6/dat_0cb93c28.inc"
};
static NJS_MKEY_F dat_0cb93c48[2] = {
#include "ev0065/amy_action_6/dat_0cb93c48.inc"
};
static NJS_MKEY_A dat_0cb93c68[2] = {
#include "ev0065/amy_action_6/dat_0cb93c68.inc"
};
static NJS_MKEY_F dat_0cb93c88[2] = {
#include "ev0065/amy_action_6/dat_0cb93c88.inc"
};
static NJS_MKEY_A dat_0cb93ca8[2] = {
#include "ev0065/amy_action_6/dat_0cb93ca8.inc"
};
static NJS_MKEY_A dat_0cb93cc8[21] = {
#include "ev0065/amy_action_6/dat_0cb93cc8.inc"
};
static NJS_MKEY_F dat_0cb93e18[2] = {
#include "ev0065/amy_action_6/dat_0cb93e18.inc"
};
static NJS_MKEY_F dat_0cb93e38[2] = {
#include "ev0065/amy_action_6/dat_0cb93e38.inc"
};
static NJS_MKEY_A dat_0cb93e58[2] = {
#include "ev0065/amy_action_6/dat_0cb93e58.inc"
};
static NJS_MKEY_F dat_0cb93e78[2] = {
#include "ev0065/amy_action_6/dat_0cb93e78.inc"
};
static NJS_MKEY_A dat_0cb93e98[2] = {
#include "ev0065/amy_action_6/dat_0cb93e98.inc"
};
static NJS_MKEY_F dat_0cb93eb8[2] = {
#include "ev0065/amy_action_6/dat_0cb93eb8.inc"
};
static NJS_MKEY_A dat_0cb93ed8[2] = {
#include "ev0065/amy_action_6/dat_0cb93ed8.inc"
};
static NJS_MKEY_F dat_0cb93ef8[2] = {
#include "ev0065/amy_action_6/dat_0cb93ef8.inc"
};
static NJS_MKEY_A dat_0cb93f18[2] = {
#include "ev0065/amy_action_6/dat_0cb93f18.inc"
};
static NJS_MKEY_F dat_0cb93f38[2] = {
#include "ev0065/amy_action_6/dat_0cb93f38.inc"
};
static NJS_MKEY_A dat_0cb93f58[2] = {
#include "ev0065/amy_action_6/dat_0cb93f58.inc"
};
static NJS_MKEY_A dat_0cb93f78[2] = {
#include "ev0065/amy_action_6/dat_0cb93f78.inc"
};
static NJS_MKEY_F dat_0cb93f98[2] = {
#include "ev0065/amy_action_6/dat_0cb93f98.inc"
};
static NJS_MKEY_F dat_0cb93fb8[21] = {
#include "ev0065/amy_action_6/dat_0cb93fb8.inc"
};
static NJS_MKEY_A dat_0cb94108[2] = {
#include "ev0065/amy_action_6/dat_0cb94108.inc"
};
static NJS_MKEY_F dat_0cb94128[21] = {
#include "ev0065/amy_action_6/dat_0cb94128.inc"
};
static NJS_MKEY_A dat_0cb94278[2] = {
#include "ev0065/amy_action_6/dat_0cb94278.inc"
};
static NJS_MKEY_F dat_0cb94298[2] = {
#include "ev0065/amy_action_6/dat_0cb94298.inc"
};
static NJS_MKEY_A dat_0cb942b8[2] = {
#include "ev0065/amy_action_6/dat_0cb942b8.inc"
};
static NJS_MKEY_F dat_0cb942d8[21] = {
#include "ev0065/amy_action_6/dat_0cb942d8.inc"
};
static NJS_MKEY_A dat_0cb94428[2] = {
#include "ev0065/amy_action_6/dat_0cb94428.inc"
};
static NJS_MKEY_F dat_0cb94448[2] = {
#include "ev0065/amy_action_6/dat_0cb94448.inc"
};
static NJS_MKEY_A dat_0cb94468[2] = {
#include "ev0065/amy_action_6/dat_0cb94468.inc"
};
static NJS_MKEY_F dat_0cb94488[2] = {
#include "ev0065/amy_action_6/dat_0cb94488.inc"
};
static NJS_MKEY_A dat_0cb944a8[2] = {
#include "ev0065/amy_action_6/dat_0cb944a8.inc"
};
static NJS_MKEY_F dat_0cb944c8[2] = {
#include "ev0065/amy_action_6/dat_0cb944c8.inc"
};
static NJS_MKEY_A dat_0cb944e8[2] = {
#include "ev0065/amy_action_6/dat_0cb944e8.inc"
};
static NJS_MKEY_F dat_0cb94508[2] = {
#include "ev0065/amy_action_6/dat_0cb94508.inc"
};
static NJS_MKEY_F dat_0cb94528[2] = {
#include "ev0065/amy_action_6/dat_0cb94528.inc"
};
static NJS_MKEY_F dat_0cb94548[2] = {
#include "ev0065/amy_action_6/dat_0cb94548.inc"
};
static NJS_MKEY_A dat_0cb94568[2] = {
#include "ev0065/amy_action_6/dat_0cb94568.inc"
};
static NJS_MKEY_F dat_0cb94588[2] = {
#include "ev0065/amy_action_6/dat_0cb94588.inc"
};
static NJS_MKEY_A dat_0cb945a8[2] = {
#include "ev0065/amy_action_6/dat_0cb945a8.inc"
};
static NJS_MKEY_F dat_0cb945c8[2] = {
#include "ev0065/amy_action_6/dat_0cb945c8.inc"
};
static NJS_MDATA2 dat_0cb945e8[57] = {
#include "ev0065/amy_action_6/dat_0cb945e8.inc"
};
static NJS_MOTION ev0065_bird_motion_2 = {
#include "ev0065/amy_action_6/ev0065_bird_motion_2.inc"
};
NJS_ACTION ev0065_bird_action_2 = {
#include "ev0065/amy_action_6/ev0065_bird_action_2.inc"
};
static NJS_MKEY_F dat_0cb9498c[2] = {
#include "ev0065/amy_action_6/dat_0cb9498c.inc"
};
static NJS_MKEY_A dat_0cb949ac[2] = {
#include "ev0065/amy_action_6/dat_0cb949ac.inc"
};
static NJS_MKEY_A dat_0cb949cc[2] = {
#include "ev0065/amy_action_6/dat_0cb949cc.inc"
};
static NJS_MKEY_F dat_0cb949ec[2] = {
#include "ev0065/amy_action_6/dat_0cb949ec.inc"
};
static NJS_MKEY_F dat_0cb94a0c[2] = {
#include "ev0065/amy_action_6/dat_0cb94a0c.inc"
};
static NJS_MKEY_A dat_0cb94a2c[2] = {
#include "ev0065/amy_action_6/dat_0cb94a2c.inc"
};
static NJS_MKEY_A dat_0cb94a4c[2] = {
#include "ev0065/amy_action_6/dat_0cb94a4c.inc"
};
static NJS_MKEY_F dat_0cb94a6c[2] = {
#include "ev0065/amy_action_6/dat_0cb94a6c.inc"
};
static NJS_MKEY_F dat_0cb94a8c[2] = {
#include "ev0065/amy_action_6/dat_0cb94a8c.inc"
};
static NJS_MKEY_A dat_0cb94aac[2] = {
#include "ev0065/amy_action_6/dat_0cb94aac.inc"
};
static NJS_MKEY_A dat_0cb94acc[2] = {
#include "ev0065/amy_action_6/dat_0cb94acc.inc"
};
static NJS_MKEY_F dat_0cb94aec[2] = {
#include "ev0065/amy_action_6/dat_0cb94aec.inc"
};
static NJS_MKEY_F dat_0cb94b0c[2] = {
#include "ev0065/amy_action_6/dat_0cb94b0c.inc"
};
static NJS_MKEY_A dat_0cb94b2c[2] = {
#include "ev0065/amy_action_6/dat_0cb94b2c.inc"
};
static NJS_MKEY_A dat_0cb94b4c[2] = {
#include "ev0065/amy_action_6/dat_0cb94b4c.inc"
};
static NJS_MKEY_F dat_0cb94b6c[2] = {
#include "ev0065/amy_action_6/dat_0cb94b6c.inc"
};
static NJS_MKEY_F dat_0cb94b8c[2] = {
#include "ev0065/amy_action_6/dat_0cb94b8c.inc"
};
static NJS_MKEY_A dat_0cb94bac[2] = {
#include "ev0065/amy_action_6/dat_0cb94bac.inc"
};
static NJS_MKEY_F dat_0cb94bcc[2] = {
#include "ev0065/amy_action_6/dat_0cb94bcc.inc"
};
static NJS_MKEY_A dat_0cb94bec[2] = {
#include "ev0065/amy_action_6/dat_0cb94bec.inc"
};
static NJS_MKEY_F dat_0cb94c0c[2] = {
#include "ev0065/amy_action_6/dat_0cb94c0c.inc"
};
static NJS_MKEY_A dat_0cb94c2c[2] = {
#include "ev0065/amy_action_6/dat_0cb94c2c.inc"
};
static NJS_MKEY_F dat_0cb94c4c[2] = {
#include "ev0065/amy_action_6/dat_0cb94c4c.inc"
};
static NJS_MKEY_A dat_0cb94c6c[2] = {
#include "ev0065/amy_action_6/dat_0cb94c6c.inc"
};
static NJS_MKEY_A dat_0cb94c8c[2] = {
#include "ev0065/amy_action_6/dat_0cb94c8c.inc"
};
static NJS_MKEY_F dat_0cb94cac[2] = {
#include "ev0065/amy_action_6/dat_0cb94cac.inc"
};
static NJS_MKEY_F dat_0cb94ccc[2] = {
#include "ev0065/amy_action_6/dat_0cb94ccc.inc"
};
static NJS_MKEY_A dat_0cb94cec[2] = {
#include "ev0065/amy_action_6/dat_0cb94cec.inc"
};
static NJS_MKEY_F dat_0cb94d0c[2] = {
#include "ev0065/amy_action_6/dat_0cb94d0c.inc"
};
static NJS_MKEY_A dat_0cb94d2c[2] = {
#include "ev0065/amy_action_6/dat_0cb94d2c.inc"
};
static NJS_MKEY_F dat_0cb94d4c[2] = {
#include "ev0065/amy_action_6/dat_0cb94d4c.inc"
};
static NJS_MKEY_A dat_0cb94d6c[2] = {
#include "ev0065/amy_action_6/dat_0cb94d6c.inc"
};
static NJS_MKEY_F dat_0cb94d8c[2] = {
#include "ev0065/amy_action_6/dat_0cb94d8c.inc"
};
static NJS_MKEY_A dat_0cb94dac[2] = {
#include "ev0065/amy_action_6/dat_0cb94dac.inc"
};
static NJS_MKEY_F dat_0cb94dcc[2] = {
#include "ev0065/amy_action_6/dat_0cb94dcc.inc"
};
static NJS_MKEY_A dat_0cb94dec[2] = {
#include "ev0065/amy_action_6/dat_0cb94dec.inc"
};
static NJS_MKEY_A dat_0cb94e0c[2] = {
#include "ev0065/amy_action_6/dat_0cb94e0c.inc"
};
static NJS_MKEY_F dat_0cb94e2c[2] = {
#include "ev0065/amy_action_6/dat_0cb94e2c.inc"
};
static NJS_MKEY_F dat_0cb94e4c[2] = {
#include "ev0065/amy_action_6/dat_0cb94e4c.inc"
};
static NJS_MKEY_A dat_0cb94e6c[2] = {
#include "ev0065/amy_action_6/dat_0cb94e6c.inc"
};
static NJS_MKEY_F dat_0cb94e8c[2] = {
#include "ev0065/amy_action_6/dat_0cb94e8c.inc"
};
static NJS_MKEY_A dat_0cb94eac[2] = {
#include "ev0065/amy_action_6/dat_0cb94eac.inc"
};
static NJS_MKEY_F dat_0cb94ecc[2] = {
#include "ev0065/amy_action_6/dat_0cb94ecc.inc"
};
static NJS_MKEY_A dat_0cb94eec[2] = {
#include "ev0065/amy_action_6/dat_0cb94eec.inc"
};
static NJS_MKEY_F dat_0cb94f0c[2] = {
#include "ev0065/amy_action_6/dat_0cb94f0c.inc"
};
static NJS_MKEY_A dat_0cb94f2c[2] = {
#include "ev0065/amy_action_6/dat_0cb94f2c.inc"
};
static NJS_MKEY_F dat_0cb94f4c[2] = {
#include "ev0065/amy_action_6/dat_0cb94f4c.inc"
};
static NJS_MKEY_A dat_0cb94f6c[2] = {
#include "ev0065/amy_action_6/dat_0cb94f6c.inc"
};
static NJS_MKEY_F dat_0cb94f8c[2] = {
#include "ev0065/amy_action_6/dat_0cb94f8c.inc"
};
static NJS_MKEY_A dat_0cb94fac[2] = {
#include "ev0065/amy_action_6/dat_0cb94fac.inc"
};
static NJS_MKEY_F dat_0cb94fcc[2] = {
#include "ev0065/amy_action_6/dat_0cb94fcc.inc"
};
static NJS_MKEY_A dat_0cb94fec[2] = {
#include "ev0065/amy_action_6/dat_0cb94fec.inc"
};
static NJS_MKEY_F dat_0cb9500c[2] = {
#include "ev0065/amy_action_6/dat_0cb9500c.inc"
};
static NJS_MKEY_F dat_0cb9502c[2] = {
#include "ev0065/amy_action_6/dat_0cb9502c.inc"
};
static NJS_MKEY_F dat_0cb9504c[2] = {
#include "ev0065/amy_action_6/dat_0cb9504c.inc"
};
static NJS_MKEY_A dat_0cb9506c[2] = {
#include "ev0065/amy_action_6/dat_0cb9506c.inc"
};
static NJS_MKEY_F dat_0cb9508c[2] = {
#include "ev0065/amy_action_6/dat_0cb9508c.inc"
};
static NJS_MKEY_A dat_0cb950ac[2] = {
#include "ev0065/amy_action_6/dat_0cb950ac.inc"
};
static NJS_MKEY_F dat_0cb950cc[2] = {
#include "ev0065/amy_action_6/dat_0cb950cc.inc"
};
static NJS_MDATA2 dat_0cb950ec[57] = {
#include "ev0065/amy_action_6/dat_0cb950ec.inc"
};
static NJS_MOTION ev0065_bird_motion_3 = {
#include "ev0065/amy_action_6/ev0065_bird_motion_3.inc"
};
NJS_ACTION ev0065_bird_action_3 = {
#include "ev0065/amy_action_6/ev0065_bird_action_3.inc"
};
static NJS_MKEY_F dat_0cb95490[31] = {
#include "ev0065/amy_action_6/dat_0cb95490.inc"
};
static NJS_MKEY_A dat_0cb95680[31] = {
#include "ev0065/amy_action_6/dat_0cb95680.inc"
};
static NJS_MKEY_A dat_0cb95870[2] = {
#include "ev0065/amy_action_6/dat_0cb95870.inc"
};
static NJS_MKEY_F dat_0cb95890[2] = {
#include "ev0065/amy_action_6/dat_0cb95890.inc"
};
static NJS_MKEY_F dat_0cb958b0[2] = {
#include "ev0065/amy_action_6/dat_0cb958b0.inc"
};
static NJS_MKEY_A dat_0cb958d0[2] = {
#include "ev0065/amy_action_6/dat_0cb958d0.inc"
};
static NJS_MKEY_A dat_0cb958f0[31] = {
#include "ev0065/amy_action_6/dat_0cb958f0.inc"
};
static NJS_MKEY_F dat_0cb95ae0[2] = {
#include "ev0065/amy_action_6/dat_0cb95ae0.inc"
};
static NJS_MKEY_F dat_0cb95b00[2] = {
#include "ev0065/amy_action_6/dat_0cb95b00.inc"
};
static NJS_MKEY_A dat_0cb95b20[2] = {
#include "ev0065/amy_action_6/dat_0cb95b20.inc"
};
static NJS_MKEY_A dat_0cb95b40[31] = {
#include "ev0065/amy_action_6/dat_0cb95b40.inc"
};
static NJS_MKEY_F dat_0cb95d30[2] = {
#include "ev0065/amy_action_6/dat_0cb95d30.inc"
};
static NJS_MKEY_F dat_0cb95d50[2] = {
#include "ev0065/amy_action_6/dat_0cb95d50.inc"
};
static NJS_MKEY_A dat_0cb95d70[2] = {
#include "ev0065/amy_action_6/dat_0cb95d70.inc"
};
static NJS_MKEY_A dat_0cb95d90[2] = {
#include "ev0065/amy_action_6/dat_0cb95d90.inc"
};
static NJS_MKEY_F dat_0cb95db0[2] = {
#include "ev0065/amy_action_6/dat_0cb95db0.inc"
};
static NJS_MKEY_F dat_0cb95dd0[2] = {
#include "ev0065/amy_action_6/dat_0cb95dd0.inc"
};
static NJS_MKEY_A dat_0cb95df0[2] = {
#include "ev0065/amy_action_6/dat_0cb95df0.inc"
};
static NJS_MKEY_F dat_0cb95e10[2] = {
#include "ev0065/amy_action_6/dat_0cb95e10.inc"
};
static NJS_MKEY_A dat_0cb95e30[2] = {
#include "ev0065/amy_action_6/dat_0cb95e30.inc"
};
static NJS_MKEY_F dat_0cb95e50[2] = {
#include "ev0065/amy_action_6/dat_0cb95e50.inc"
};
static NJS_MKEY_A dat_0cb95e70[2] = {
#include "ev0065/amy_action_6/dat_0cb95e70.inc"
};
static NJS_MKEY_F dat_0cb95e90[2] = {
#include "ev0065/amy_action_6/dat_0cb95e90.inc"
};
static NJS_MKEY_A dat_0cb95eb0[2] = {
#include "ev0065/amy_action_6/dat_0cb95eb0.inc"
};
static NJS_MKEY_A dat_0cb95ed0[31] = {
#include "ev0065/amy_action_6/dat_0cb95ed0.inc"
};
static NJS_MKEY_F dat_0cb960c0[2] = {
#include "ev0065/amy_action_6/dat_0cb960c0.inc"
};
static NJS_MKEY_F dat_0cb960e0[2] = {
#include "ev0065/amy_action_6/dat_0cb960e0.inc"
};
static NJS_MKEY_A dat_0cb96100[2] = {
#include "ev0065/amy_action_6/dat_0cb96100.inc"
};
static NJS_MKEY_F dat_0cb96120[2] = {
#include "ev0065/amy_action_6/dat_0cb96120.inc"
};
static NJS_MKEY_A dat_0cb96140[2] = {
#include "ev0065/amy_action_6/dat_0cb96140.inc"
};
static NJS_MKEY_F dat_0cb96160[2] = {
#include "ev0065/amy_action_6/dat_0cb96160.inc"
};
static NJS_MKEY_A dat_0cb96180[2] = {
#include "ev0065/amy_action_6/dat_0cb96180.inc"
};
static NJS_MKEY_F dat_0cb961a0[2] = {
#include "ev0065/amy_action_6/dat_0cb961a0.inc"
};
static NJS_MKEY_A dat_0cb961c0[2] = {
#include "ev0065/amy_action_6/dat_0cb961c0.inc"
};
static NJS_MKEY_F dat_0cb961e0[2] = {
#include "ev0065/amy_action_6/dat_0cb961e0.inc"
};
static NJS_MKEY_A dat_0cb96200[2] = {
#include "ev0065/amy_action_6/dat_0cb96200.inc"
};
static NJS_MKEY_A dat_0cb96220[2] = {
#include "ev0065/amy_action_6/dat_0cb96220.inc"
};
static NJS_MKEY_F dat_0cb96240[2] = {
#include "ev0065/amy_action_6/dat_0cb96240.inc"
};
static NJS_MKEY_F dat_0cb96260[31] = {
#include "ev0065/amy_action_6/dat_0cb96260.inc"
};
static NJS_MKEY_A dat_0cb96450[2] = {
#include "ev0065/amy_action_6/dat_0cb96450.inc"
};
static NJS_MKEY_F dat_0cb96470[31] = {
#include "ev0065/amy_action_6/dat_0cb96470.inc"
};
static NJS_MKEY_A dat_0cb96660[2] = {
#include "ev0065/amy_action_6/dat_0cb96660.inc"
};
static NJS_MKEY_F dat_0cb96680[2] = {
#include "ev0065/amy_action_6/dat_0cb96680.inc"
};
static NJS_MKEY_A dat_0cb966a0[2] = {
#include "ev0065/amy_action_6/dat_0cb966a0.inc"
};
static NJS_MKEY_F dat_0cb966c0[31] = {
#include "ev0065/amy_action_6/dat_0cb966c0.inc"
};
static NJS_MKEY_A dat_0cb968b0[2] = {
#include "ev0065/amy_action_6/dat_0cb968b0.inc"
};
static NJS_MKEY_F dat_0cb968d0[2] = {
#include "ev0065/amy_action_6/dat_0cb968d0.inc"
};
static NJS_MKEY_A dat_0cb968f0[2] = {
#include "ev0065/amy_action_6/dat_0cb968f0.inc"
};
static NJS_MKEY_F dat_0cb96910[2] = {
#include "ev0065/amy_action_6/dat_0cb96910.inc"
};
static NJS_MKEY_A dat_0cb96930[2] = {
#include "ev0065/amy_action_6/dat_0cb96930.inc"
};
static NJS_MKEY_F dat_0cb96950[2] = {
#include "ev0065/amy_action_6/dat_0cb96950.inc"
};
static NJS_MKEY_A dat_0cb96970[2] = {
#include "ev0065/amy_action_6/dat_0cb96970.inc"
};
static NJS_MKEY_F dat_0cb96990[2] = {
#include "ev0065/amy_action_6/dat_0cb96990.inc"
};
static NJS_MKEY_F dat_0cb969b0[2] = {
#include "ev0065/amy_action_6/dat_0cb969b0.inc"
};
static NJS_MKEY_F dat_0cb969d0[2] = {
#include "ev0065/amy_action_6/dat_0cb969d0.inc"
};
static NJS_MKEY_A dat_0cb969f0[2] = {
#include "ev0065/amy_action_6/dat_0cb969f0.inc"
};
static NJS_MKEY_F dat_0cb96a10[2] = {
#include "ev0065/amy_action_6/dat_0cb96a10.inc"
};
static NJS_MKEY_A dat_0cb96a30[2] = {
#include "ev0065/amy_action_6/dat_0cb96a30.inc"
};
static NJS_MKEY_F dat_0cb96a50[2] = {
#include "ev0065/amy_action_6/dat_0cb96a50.inc"
};
static NJS_MDATA2 dat_0cb96a70[57] = {
#include "ev0065/amy_action_6/dat_0cb96a70.inc"
};
static NJS_MOTION ev0065_bird_motion_4 = {
#include "ev0065/amy_action_6/ev0065_bird_motion_4.inc"
};
NJS_ACTION ev0065_bird_action_4 = {
#include "ev0065/amy_action_6/ev0065_bird_action_4.inc"
};
static NJS_MKEY_F dat_0cb96e14[2] = {
#include "ev0065/amy_action_6/dat_0cb96e14.inc"
};
static NJS_MKEY_A dat_0cb96e34[2] = {
#include "ev0065/amy_action_6/dat_0cb96e34.inc"
};
static NJS_MKEY_F dat_0cb96e54[2] = {
#include "ev0065/amy_action_6/dat_0cb96e54.inc"
};
static NJS_MKEY_F dat_0cb96e74[2] = {
#include "ev0065/amy_action_6/dat_0cb96e74.inc"
};
static NJS_MKEY_A dat_0cb96e94[2] = {
#include "ev0065/amy_action_6/dat_0cb96e94.inc"
};
static NJS_MKEY_A dat_0cb96eb4[2] = {
#include "ev0065/amy_action_6/dat_0cb96eb4.inc"
};
static NJS_MKEY_F dat_0cb96ed4[2] = {
#include "ev0065/amy_action_6/dat_0cb96ed4.inc"
};
static NJS_MKEY_F dat_0cb96ef4[2] = {
#include "ev0065/amy_action_6/dat_0cb96ef4.inc"
};
static NJS_MKEY_A dat_0cb96f14[2] = {
#include "ev0065/amy_action_6/dat_0cb96f14.inc"
};
static NJS_MKEY_A dat_0cb96f34[2] = {
#include "ev0065/amy_action_6/dat_0cb96f34.inc"
};
static NJS_MKEY_F dat_0cb96f54[2] = {
#include "ev0065/amy_action_6/dat_0cb96f54.inc"
};
static NJS_MKEY_F dat_0cb96f74[2] = {
#include "ev0065/amy_action_6/dat_0cb96f74.inc"
};
static NJS_MKEY_A dat_0cb96f94[2] = {
#include "ev0065/amy_action_6/dat_0cb96f94.inc"
};
static NJS_MKEY_A dat_0cb96fb4[2] = {
#include "ev0065/amy_action_6/dat_0cb96fb4.inc"
};
static NJS_MKEY_F dat_0cb96fd4[2] = {
#include "ev0065/amy_action_6/dat_0cb96fd4.inc"
};
static NJS_MKEY_F dat_0cb96ff4[2] = {
#include "ev0065/amy_action_6/dat_0cb96ff4.inc"
};
static NJS_MKEY_A dat_0cb97014[2] = {
#include "ev0065/amy_action_6/dat_0cb97014.inc"
};
static NJS_MKEY_F dat_0cb97034[2] = {
#include "ev0065/amy_action_6/dat_0cb97034.inc"
};
static NJS_MKEY_A dat_0cb97054[2] = {
#include "ev0065/amy_action_6/dat_0cb97054.inc"
};
static NJS_MKEY_F dat_0cb97074[2] = {
#include "ev0065/amy_action_6/dat_0cb97074.inc"
};
static NJS_MKEY_A dat_0cb97094[2] = {
#include "ev0065/amy_action_6/dat_0cb97094.inc"
};
static NJS_MKEY_F dat_0cb970b4[2] = {
#include "ev0065/amy_action_6/dat_0cb970b4.inc"
};
static NJS_MKEY_A dat_0cb970d4[2] = {
#include "ev0065/amy_action_6/dat_0cb970d4.inc"
};
static NJS_MKEY_A dat_0cb970f4[2] = {
#include "ev0065/amy_action_6/dat_0cb970f4.inc"
};
static NJS_MKEY_F dat_0cb97114[2] = {
#include "ev0065/amy_action_6/dat_0cb97114.inc"
};
static NJS_MKEY_F dat_0cb97134[2] = {
#include "ev0065/amy_action_6/dat_0cb97134.inc"
};
static NJS_MKEY_A dat_0cb97154[2] = {
#include "ev0065/amy_action_6/dat_0cb97154.inc"
};
static NJS_MKEY_F dat_0cb97174[2] = {
#include "ev0065/amy_action_6/dat_0cb97174.inc"
};
static NJS_MKEY_A dat_0cb97194[2] = {
#include "ev0065/amy_action_6/dat_0cb97194.inc"
};
static NJS_MKEY_F dat_0cb971b4[2] = {
#include "ev0065/amy_action_6/dat_0cb971b4.inc"
};
static NJS_MKEY_A dat_0cb971d4[2] = {
#include "ev0065/amy_action_6/dat_0cb971d4.inc"
};
static NJS_MKEY_F dat_0cb971f4[2] = {
#include "ev0065/amy_action_6/dat_0cb971f4.inc"
};
static NJS_MKEY_A dat_0cb97214[2] = {
#include "ev0065/amy_action_6/dat_0cb97214.inc"
};
static NJS_MKEY_F dat_0cb97234[2] = {
#include "ev0065/amy_action_6/dat_0cb97234.inc"
};
static NJS_MKEY_A dat_0cb97254[2] = {
#include "ev0065/amy_action_6/dat_0cb97254.inc"
};
static NJS_MKEY_A dat_0cb97274[2] = {
#include "ev0065/amy_action_6/dat_0cb97274.inc"
};
static NJS_MKEY_F dat_0cb97294[2] = {
#include "ev0065/amy_action_6/dat_0cb97294.inc"
};
static NJS_MKEY_F dat_0cb972b4[2] = {
#include "ev0065/amy_action_6/dat_0cb972b4.inc"
};
static NJS_MKEY_A dat_0cb972d4[2] = {
#include "ev0065/amy_action_6/dat_0cb972d4.inc"
};
static NJS_MKEY_F dat_0cb972f4[2] = {
#include "ev0065/amy_action_6/dat_0cb972f4.inc"
};
static NJS_MKEY_A dat_0cb97314[2] = {
#include "ev0065/amy_action_6/dat_0cb97314.inc"
};
static NJS_MKEY_F dat_0cb97334[2] = {
#include "ev0065/amy_action_6/dat_0cb97334.inc"
};
static NJS_MKEY_A dat_0cb97354[2] = {
#include "ev0065/amy_action_6/dat_0cb97354.inc"
};
static NJS_MKEY_F dat_0cb97374[2] = {
#include "ev0065/amy_action_6/dat_0cb97374.inc"
};
static NJS_MKEY_A dat_0cb97394[2] = {
#include "ev0065/amy_action_6/dat_0cb97394.inc"
};
static NJS_MKEY_F dat_0cb973b4[2] = {
#include "ev0065/amy_action_6/dat_0cb973b4.inc"
};
static NJS_MKEY_A dat_0cb973d4[2] = {
#include "ev0065/amy_action_6/dat_0cb973d4.inc"
};
static NJS_MKEY_F dat_0cb973f4[2] = {
#include "ev0065/amy_action_6/dat_0cb973f4.inc"
};
static NJS_MKEY_A dat_0cb97414[2] = {
#include "ev0065/amy_action_6/dat_0cb97414.inc"
};
static NJS_MKEY_F dat_0cb97434[2] = {
#include "ev0065/amy_action_6/dat_0cb97434.inc"
};
static NJS_MKEY_A dat_0cb97454[2] = {
#include "ev0065/amy_action_6/dat_0cb97454.inc"
};
static NJS_MKEY_F dat_0cb97474[2] = {
#include "ev0065/amy_action_6/dat_0cb97474.inc"
};
static NJS_MKEY_F dat_0cb97494[2] = {
#include "ev0065/amy_action_6/dat_0cb97494.inc"
};
static NJS_MKEY_F dat_0cb974b4[2] = {
#include "ev0065/amy_action_6/dat_0cb974b4.inc"
};
static NJS_MKEY_A dat_0cb974d4[2] = {
#include "ev0065/amy_action_6/dat_0cb974d4.inc"
};
static NJS_MKEY_F dat_0cb974f4[2] = {
#include "ev0065/amy_action_6/dat_0cb974f4.inc"
};
static NJS_MKEY_A dat_0cb97514[2] = {
#include "ev0065/amy_action_6/dat_0cb97514.inc"
};
static NJS_MKEY_F dat_0cb97534[2] = {
#include "ev0065/amy_action_6/dat_0cb97534.inc"
};
static NJS_MDATA2 dat_0cb97554[57] = {
#include "ev0065/amy_action_6/dat_0cb97554.inc"
};
static NJS_MOTION ev0065_bird_motion_5 = {
#include "ev0065/amy_action_6/ev0065_bird_motion_5.inc"
};
NJS_ACTION ev0065_bird_action_5 = {
#include "ev0065/amy_action_6/ev0065_bird_action_5.inc"
};
static NJS_MKEY_F dat_0cb978f8[32] = {
#include "ev0065/amy_action_6/dat_0cb978f8.inc"
};
static NJS_MKEY_A dat_0cb97af8[2] = {
#include "ev0065/amy_action_6/dat_0cb97af8.inc"
};
static NJS_MKEY_F dat_0cb97b18[2] = {
#include "ev0065/amy_action_6/dat_0cb97b18.inc"
};
static NJS_MKEY_F dat_0cb97b38[2] = {
#include "ev0065/amy_action_6/dat_0cb97b38.inc"
};
static NJS_MKEY_A dat_0cb97b58[2] = {
#include "ev0065/amy_action_6/dat_0cb97b58.inc"
};
static NJS_MKEY_A dat_0cb97b78[32] = {
#include "ev0065/amy_action_6/dat_0cb97b78.inc"
};
static NJS_MKEY_F dat_0cb97d78[2] = {
#include "ev0065/amy_action_6/dat_0cb97d78.inc"
};
static NJS_MKEY_F dat_0cb97d98[2] = {
#include "ev0065/amy_action_6/dat_0cb97d98.inc"
};
static NJS_MKEY_A dat_0cb97db8[2] = {
#include "ev0065/amy_action_6/dat_0cb97db8.inc"
};
static NJS_MKEY_A dat_0cb97dd8[32] = {
#include "ev0065/amy_action_6/dat_0cb97dd8.inc"
};
static NJS_MKEY_F dat_0cb97fd8[2] = {
#include "ev0065/amy_action_6/dat_0cb97fd8.inc"
};
static NJS_MKEY_F dat_0cb97ff8[2] = {
#include "ev0065/amy_action_6/dat_0cb97ff8.inc"
};
static NJS_MKEY_A dat_0cb98018[2] = {
#include "ev0065/amy_action_6/dat_0cb98018.inc"
};
static NJS_MKEY_A dat_0cb98038[2] = {
#include "ev0065/amy_action_6/dat_0cb98038.inc"
};
static NJS_MKEY_F dat_0cb98058[2] = {
#include "ev0065/amy_action_6/dat_0cb98058.inc"
};
static NJS_MKEY_F dat_0cb98078[2] = {
#include "ev0065/amy_action_6/dat_0cb98078.inc"
};
static NJS_MKEY_A dat_0cb98098[2] = {
#include "ev0065/amy_action_6/dat_0cb98098.inc"
};
static NJS_MKEY_F dat_0cb980b8[2] = {
#include "ev0065/amy_action_6/dat_0cb980b8.inc"
};
static NJS_MKEY_A dat_0cb980d8[2] = {
#include "ev0065/amy_action_6/dat_0cb980d8.inc"
};
static NJS_MKEY_F dat_0cb980f8[2] = {
#include "ev0065/amy_action_6/dat_0cb980f8.inc"
};
static NJS_MKEY_A dat_0cb98118[2] = {
#include "ev0065/amy_action_6/dat_0cb98118.inc"
};
static NJS_MKEY_F dat_0cb98138[2] = {
#include "ev0065/amy_action_6/dat_0cb98138.inc"
};
static NJS_MKEY_A dat_0cb98158[2] = {
#include "ev0065/amy_action_6/dat_0cb98158.inc"
};
static NJS_MKEY_A dat_0cb98178[32] = {
#include "ev0065/amy_action_6/dat_0cb98178.inc"
};
static NJS_MKEY_F dat_0cb98378[2] = {
#include "ev0065/amy_action_6/dat_0cb98378.inc"
};
static NJS_MKEY_F dat_0cb98398[2] = {
#include "ev0065/amy_action_6/dat_0cb98398.inc"
};
static NJS_MKEY_A dat_0cb983b8[2] = {
#include "ev0065/amy_action_6/dat_0cb983b8.inc"
};
static NJS_MKEY_F dat_0cb983d8[2] = {
#include "ev0065/amy_action_6/dat_0cb983d8.inc"
};
static NJS_MKEY_A dat_0cb983f8[2] = {
#include "ev0065/amy_action_6/dat_0cb983f8.inc"
};
static NJS_MKEY_F dat_0cb98418[2] = {
#include "ev0065/amy_action_6/dat_0cb98418.inc"
};
static NJS_MKEY_A dat_0cb98438[2] = {
#include "ev0065/amy_action_6/dat_0cb98438.inc"
};
static NJS_MKEY_F dat_0cb98458[2] = {
#include "ev0065/amy_action_6/dat_0cb98458.inc"
};
static NJS_MKEY_A dat_0cb98478[2] = {
#include "ev0065/amy_action_6/dat_0cb98478.inc"
};
static NJS_MKEY_F dat_0cb98498[2] = {
#include "ev0065/amy_action_6/dat_0cb98498.inc"
};
static NJS_MKEY_A dat_0cb984b8[2] = {
#include "ev0065/amy_action_6/dat_0cb984b8.inc"
};
static NJS_MKEY_A dat_0cb984d8[2] = {
#include "ev0065/amy_action_6/dat_0cb984d8.inc"
};
static NJS_MKEY_F dat_0cb984f8[2] = {
#include "ev0065/amy_action_6/dat_0cb984f8.inc"
};
static NJS_MKEY_F dat_0cb98518[32] = {
#include "ev0065/amy_action_6/dat_0cb98518.inc"
};
static NJS_MKEY_A dat_0cb98718[2] = {
#include "ev0065/amy_action_6/dat_0cb98718.inc"
};
static NJS_MKEY_F dat_0cb98738[32] = {
#include "ev0065/amy_action_6/dat_0cb98738.inc"
};
static NJS_MKEY_A dat_0cb98938[2] = {
#include "ev0065/amy_action_6/dat_0cb98938.inc"
};
static NJS_MKEY_F dat_0cb98958[2] = {
#include "ev0065/amy_action_6/dat_0cb98958.inc"
};
static NJS_MKEY_A dat_0cb98978[2] = {
#include "ev0065/amy_action_6/dat_0cb98978.inc"
};
static NJS_MKEY_F dat_0cb98998[32] = {
#include "ev0065/amy_action_6/dat_0cb98998.inc"
};
static NJS_MKEY_A dat_0cb98b98[2] = {
#include "ev0065/amy_action_6/dat_0cb98b98.inc"
};
static NJS_MKEY_F dat_0cb98bb8[2] = {
#include "ev0065/amy_action_6/dat_0cb98bb8.inc"
};
static NJS_MKEY_A dat_0cb98bd8[2] = {
#include "ev0065/amy_action_6/dat_0cb98bd8.inc"
};
static NJS_MKEY_F dat_0cb98bf8[2] = {
#include "ev0065/amy_action_6/dat_0cb98bf8.inc"
};
static NJS_MKEY_A dat_0cb98c18[2] = {
#include "ev0065/amy_action_6/dat_0cb98c18.inc"
};
static NJS_MKEY_F dat_0cb98c38[2] = {
#include "ev0065/amy_action_6/dat_0cb98c38.inc"
};
static NJS_MKEY_A dat_0cb98c58[2] = {
#include "ev0065/amy_action_6/dat_0cb98c58.inc"
};
static NJS_MKEY_F dat_0cb98c78[2] = {
#include "ev0065/amy_action_6/dat_0cb98c78.inc"
};
static NJS_MKEY_F dat_0cb98c98[2] = {
#include "ev0065/amy_action_6/dat_0cb98c98.inc"
};
static NJS_MKEY_F dat_0cb98cb8[2] = {
#include "ev0065/amy_action_6/dat_0cb98cb8.inc"
};
static NJS_MKEY_A dat_0cb98cd8[2] = {
#include "ev0065/amy_action_6/dat_0cb98cd8.inc"
};
static NJS_MKEY_F dat_0cb98cf8[2] = {
#include "ev0065/amy_action_6/dat_0cb98cf8.inc"
};
static NJS_MKEY_A dat_0cb98d18[2] = {
#include "ev0065/amy_action_6/dat_0cb98d18.inc"
};
static NJS_MKEY_F dat_0cb98d38[2] = {
#include "ev0065/amy_action_6/dat_0cb98d38.inc"
};
static NJS_MDATA2 dat_0cb98d58[57] = {
#include "ev0065/amy_action_6/dat_0cb98d58.inc"
};
static NJS_MOTION ev0065_bird_motion_6 = {
#include "ev0065/amy_action_6/ev0065_bird_motion_6.inc"
};
NJS_ACTION ev0065_bird_action_6 = {
#include "ev0065/amy_action_6/ev0065_bird_action_6.inc"
};
static NJS_TEXNAME ev0065_ver2_wing_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0065_ver2_wing_texlist = {
#include "ev0065/amy_action_6/ev0065_ver2_wing_texlist.inc"
};
static NJS_MATERIAL dat_0cb99194[1] = {
#include "ev0065/amy_action_6/dat_0cb99194.inc"
};
