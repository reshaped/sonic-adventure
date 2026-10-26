/* Ninja 3D library, as far as the sources use it */
#ifndef NINJA_H
#define NINJA_H
#include <sg_xpt.h>
/* njtypes.h uses sg_xpt.h's types, so it comes after sg_xpt.h */
#include <njtypes.h> /* IWYU pragma: export */

/* njCreateLight's types, which combine these bits into a light's attr.lsrc */
#define NJD_AMBIENT 0x01
#define NJD_DIR_LIGHT 0x02
#define NJD_POINT_LIGHT 0x04
#define NJD_SPOT_LIGHT 0x08
#define NJD_SPEC_DIR 0x10
#define NJD_SPEC_POINT 0x20
/* as a type alone, a light whose calculator is its attr.func */
#define NJD_USER_LIGHT 0x40
#define NJD_SIMPLE_LIGHT 0x80
#define NJD_PHONG_DIR (NJD_AMBIENT | NJD_DIR_LIGHT | NJD_SPEC_DIR)
#define NJD_PHONG_POINT (NJD_AMBIENT | NJD_POINT_LIGHT | NJD_SPEC_POINT)
/* add l to the created lights, after the others, and set it up as a light of
 * type, which attr.lsrc takes. with NJD_LIGHT_MAX lights created, it does
 * nothing. l stays created until njDeleteLight(l) or njLightSystemInit.
 *
 * the light is on, with stat 1. it takes pnt the origin, vctr (0, 0, -1) and
 * mtrx the unit matrix. its attr takes argb (0, 1, 1, 1) as a, r, g, b, idif
 * 1, iamb 0.2 and ispc 0.3, the last two rounded down to a Float, nrang 1,
 * frang 65535, iang NJM_DEG_ANG(10) and oang NJM_DEG_ANG(30). attr.func is
 * left as it was. from those values and _nj_light_material_.exponent, its
 * ltcal takes:
 * - lpnt the origin and lvctr (0, 0, -1);
 * - nrr and frr, nrang and frang squared;
 * - cosi and cose, the squares of iang's and oang's cosines;
 * - idev, 1 / (frang - nrang), and odev, frang * idev;
 * - reserve, oang in its low half and iang in its high;
 * - exp, the exponent truncated, and intns, 1;
 * - rate, 1 / |cosi - cose| for type 0x88 or 0xc8, and otherwise 1 / cosi^p
 *   for the power p that exp names, each as FSRRA approximates it;
 * - atten, argb times intns, and amb, dif and spc, atten times iamb, idif and
 *   ispc.
 * ltcal's ratten, ipd, lmvctr, mamb, mdif and mspc are left as they were.
 * njGetLightDistance sets the first three, and njCalcLightMaterials sets the
 * last three */
void njCreateLight(NJS_LIGHT *l, Int type);
void njDeleteLight(NJS_LIGHT *l);
void njSetLightDirection(NJS_LIGHT *l, Float x, Float y, Float z);
/* njSetLightColor sets attr.argb's r, g and b, njSetLightIntensity attr.ispc,
 * idif and iamb, and njSetLightAlpha attr.argb.a. each then sets ltcal.atten to
 * attr.argb times ltcal.intns, and ltcal.spc, dif and amb to atten times ispc,
 * idif and iamb */
void njSetLightColor(NJS_LIGHT *l, Float r, Float g, Float b);
void njSetLightIntensity(NJS_LIGHT *l, Float spc, Float dif, Float amb);
void njSetLightAlpha(NJS_LIGHT *l, Float a);
void njSetLightAngle(NJS_LIGHT *l, Angle iang, Angle oang);
/* set attr.nrang and attr.frang, ltcal.nrr and ltcal.frr to their squares,
 * ltcal.idev to 1 / (frang - nrang) and ltcal.odev to frang * ltcal.idev.
 * equal ranges divide by zero */
void njSetLightRange(NJS_LIGHT *l, Float nrang, Float frang);
void njSetLightLocation(NJS_LIGHT *l, Float x, Float y, Float z);
void njLightOn(NJS_LIGHT *l);
void njLightOff(NJS_LIGHT *l);
/* njUnitLightMatrix calls njUnitMatrix on l->mtrx, and njTranslateLight and
 * njTranslateLightV call njTranslate and njTranslateV on it */
void njUnitLightMatrix(NJS_LIGHT *l);
void njTranslateLight(NJS_LIGHT *l, Float x, Float y, Float z);
void njTranslateLightV(NJS_LIGHT *l, NJS_VECTOR *v);
/* njRotateLightX and njRotateLightY call njRotateX and njRotateY on
 * l->mtrx */
void njRotateLightX(NJS_LIGHT *l, Angle ang);
void njRotateLightY(NJS_LIGHT *l, Angle ang);
/* the light queue, _nj_light_queue_, holds the indices in _nj_light_control_
 * of the lights a draw lights with. njDrawModel fills it with njSetQueue, sets
 * its lights up with njSetLightMaterials and njCalcLightMatrices, draws with
 * njDrawModelSub, and empties it with njResetQueue. every function that walks
 * the queue requires each index in it to name a created light */

/* light point, with normal, by the queued lights. reverse negates the normal.
 * each light's calculator, which njSetQueue picks by the light's type, adds
 * the light's share to a diffuse and a specular color. then
 * _nj_light_material_.diffuse.a is added to the diffuse alpha. with
 * _nj_depth_enable_ set, that alpha is next scaled by
 * 1 - (z - _nj_depth_near_) * _nj_depth_range_ where the point's z lies past
 * _nj_depth_near_ and short of _nj_depth_far_, and is 0 from _nj_depth_far_
 * on.
 *
 * color holds the diffuse, then the specular. njCalculateLightsRGB writes the
 * light into color, and njCalculateLightsRGBVertex multiplies color by it;
 * both leave the specular alpha as it was. njCalculateLightsRGBPacked writes
 * it as two ARGB words, each channel times 255, and
 * njCalculateLightsRGBPackedVertex multiplies each channel of color's two
 * ARGB words by it. the packed forms truncate each channel, make one above 255
 * or below 0 255, and set the specular alpha to 255.
 *
 * the calls hand color, point and normal to the calculators through
 * _nj_ptr_rgb_lt_argb_, _nj_ptr_rgb_lt_pnt_ and _nj_ptr_rgb_lt_nml_, so none
 * can run inside another, as from an NJD_USER_LIGHT light's attr.func. each
 * light after a type 0x00 light in the queue adds its share, as Floats, to the
 * 32 bytes at _nj_ptr_rgb_lt_argb_'s own address in place of color: over that
 * pointer and the pointers after it */
void njCalculateLightsRGB(NJS_ARGB *color, NJS_POINT3 *point,
                          NJS_VECTOR *normal, Int reverse);
void njCalculateLightsRGBVertex(NJS_ARGB *color, NJS_POINT3 *point,
                                NJS_VECTOR *normal, Int reverse);
void njCalculateLightsRGBPacked(Uint32 *color, NJS_POINT3 *point,
                                NJS_VECTOR *normal, Int reverse);
void njCalculateLightsRGBPackedVertex(Uint32 *color, NJS_POINT3 *point,
                                      NJS_VECTOR *normal, Int reverse);
/* with bits 0 and 1 of flags set and bit 19 clear, set the r, g and b of each
 * queued light's ltcal.mamb to _nj_light_material_.diffuse's times
 * ltcal.amb's. other flags change nothing */
void njCheckLightMaterials(Int flags);
/* queue each created light that is on, with stat 1, after the lights queued
 * already; the queue must have room for them. then, if any light is created,
 * pick each queued light's calculator by its type, for the
 * njCalculateLightsRGB calls */
void njSetQueue(void);
/* set _nj_light_material_ to the values njLightSystemInit gives it: ambient
 * (1, 0, 0, 0) and diffuse and specular (1, 0.5, 0.5, 0.5), as a, r, g, b, and
 * exponent 6. then set the ltcal.rate of each queued light of type 0x88 or
 * 0xc8 to 1 / |ltcal.cosi - ltcal.cose|, as FSRRA approximates it */
void njSetLightMaterials(void);
/* set each queued light's ltcal.lpnt and ltcal.lvctr to its pnt and vctr
 * transformed by its mtrx and then by _nj_view_matrix_, through njCalcPoint
 * and njCalcVector */
void njCalcLightMatrices(void);
/* @hack type: volatile on the parameter, which keeps model in a stack
 * slot, MOV.L R4,@(4,R15) at 0x8c653772, while njDrawModelSub saves the
 * caller's R8; without it, 13 of njDrawModelSub's 14 words differ and it runs
 * 4 bytes short, and 18 differ in 6 functions beside it. SHC's release R10
 * refuses, with error 2118, a prototype whose qualifier differs from the
 * definition's, so the declaration takes volatile too */
void njDrawModelSub(NJS_MODEL *volatile model);
void njResetQueue(void); /* empty the queue */
/* set the r, g and b of each queued light's ltcal.mamb, mdif and mspc to
 * _nj_light_material_'s ambient, diffuse and specular times the light's amb,
 * dif and spc, and mspc.a to spc.a. set ltcal.exp to the material's exponent,
 * truncated. for a light whose type has NJD_SPOT_LIGHT, NJD_SPEC_DIR or
 * NJD_SPEC_POINT, set ltcal.rate to 1 / cosi^p, for the power p that exp
 * names. types 0x88 and 0xc8 take that rate too, in place of the one
 * njSetLightMaterials gives them */
void njCalcLightMaterials(void);
/* set ltcal.ratten and ltcal.lmvctr of each queued light of type 0xc4, 0xc5,
 * 0xe0, 0xe5 or 0xc8 for point. d2 is the squared distance from point to
 * ltcal.lpnt. where d2 < ltcal.frr, lmvctr is the unit vector from point
 * toward lpnt, and ratten is nrr / d2 where d2 > nrr, or else 1. where
 * d2 >= frr, ratten is 0, and a light of any type but 0xc8 keeps its lmvctr.
 *
 * type 0xc8 sets lmvctr at any distance, and ltcal.ipd to the square of ip,
 * lmvctr's dot with ltcal.lvctr. its ratten is 0 where ip >= 0 or
 * ipd <= cose, too, and where ipd < cosi it is multiplied by
 * rate * (ipd - cose) */
void njGetLightDistance(NJS_POINT3 *point);
/* the light of the njEasyDraw calls. njSetEasyLight sets the direction it
 * shines in, in the space the current matrix takes a model into, and copies
 * the screen's projection: the easy draws project and clip by that copy until
 * the next call. a point's intensity is ambient plus inten times its normal's
 * dot with the reversed direction, where that product is positive; neither
 * vector is made unit length. njSetEasyLightColor sets the light's color,
 * which starts white. a textured meshset's intensities scale that color, and
 * an untextured one's scale it times the material's diffuse */
void njSetEasyLight(Float x, Float y, Float z);
void njSetEasyLightIntensity(Float inten, Float ambient);
void njSetEasyLightColor(Float r, Float g, Float b);

/* njInitSystem's display modes and frame buffer formats. a display mode's low
 * twelve bits, NJD_DSP_MASK, are Kamui's display mode, whose low two bits are
 * its video output. NJD_DSP_ANTI asks for anti-aliasing */
#define NJD_DSP_WIDTH_640 0x010
#define NJD_DSP_HEIGHT_480 0x020
#define NJD_DSP_ANTI 0x00010000
#define NJD_DSP_MASK 0x00000fff
#define NJD_RESOLUTION_VGA 0x01
#define NJD_RESOLUTION_640x480_NTSCI 0x34
#define NJD_RESOLUTION_640x480_NTSCNI 0x38
#define NJD_FRAMEBUFFER_MODE_RGB565 0
#define NJD_FRAMEBUFFER_MODE_RGB555 1
#define NJD_FRAMEBUFFER_MODE_ARGB1555 3
#define NJD_FRAMEBUFFER_MODE_RGB888 4
#define NJD_FRAMEBUFFER_MODE_ARGB8888 5
/* set up the display, Kamui and Ninja's state. frame must be one of the frame
 * buffer formats above: for any other, Kamui sets no display mode */
void njInitSystem(Int mode, Int frame, Int count);
/* set the display mode, frame buffer format and vsync count again, keeping
 * Kamui's setup, and reset the screen's center, the 2D clip and the draw
 * counts. frame must be one of the formats above */
void njChangeSystem(Int mode, Int frame, Int count);
void njExitSystem(void);
/* the bytes of a Kamui vertex context: njGetSystemAttr copies the current one
 * out, and njSetSystemAttr copies one in */
#define NJD_SYS_ATTR_SIZE 192
typedef Sint8 NJS_SYS_ATTR[NJD_SYS_ATTR_SIZE];

/* njColorBlendingMode's targets and modes */
#define NJD_SOURCE_COLOR 0
#define NJD_DESTINATION_COLOR 1
#define NJD_COLOR_BLENDING_BOTHINVALPHA 0
#define NJD_COLOR_BLENDING_BOTHSRCALPHA 1
#define NJD_COLOR_BLENDING_DESTALPHA 2
#define NJD_COLOR_BLENDING_DESTCOLOR 3
#define NJD_COLOR_BLENDING_INVDESTALPHA 4
#define NJD_COLOR_BLENDING_INVDESTCOLOR 5
#define NJD_COLOR_BLENDING_INVSRCALPHA 6
#define NJD_COLOR_BLENDING_INVSRCCOLOR 7
#define NJD_COLOR_BLENDING_SRCALPHA 8
#define NJD_COLOR_BLENDING_SRCCOLOR 9
#define NJD_COLOR_BLENDING_ONE 10
#define NJD_COLOR_BLENDING_ZERO 11

/* material flags, in attrflags, which njSetConstantAttr can also set or clear.
 * NJD_SA_ and NJD_DA_ are the source and destination alpha blend factors */
#define NJD_SA_ONE 0x20000000
#define NJD_SA_SRC 0x80000000
#define NJD_SA_INV_SRC 0xa0000000
#define NJD_SA_MASK 0xe0000000
#define NJD_DA_ONE 0x04000000
#define NJD_DA_INV_SRC 0x14000000
#define NJD_DA_MASK 0x1c000000
#define NJD_FLAG_IGNORE_LIGHT 0x02000000
#define NJD_FLAG_USE_FLAT 0x01000000
#define NJD_FLAG_DOUBLE_SIDE 0x00800000
#define NJD_FLAG_USE_ENV 0x00400000
#define NJD_FLAG_USE_TEXTURE 0x00200000
#define NJD_FLAG_USE_ALPHA 0x00100000
#define NJD_FLAG_IGNORE_SPECULAR 0x00080000
#define NJD_FLAG_CLAMP_U 0x00010000
#define NJD_FLAG_CLAMP_V 0x00008000
#define NJD_FILTER_BILINEAR 0x00002000
#define NJD_D_100 0x00000400

/* attributes of njDrawLine2D, njDrawLine3D, njDrawTriangle3D,
 * njDrawPolygon2D and njDrawPolygon3D */
#define NJD_DRAW_MASK 0x0000000f
#define NJD_DRAW_NORMAL 0x00000000
#define NJD_DRAW_FAN 0x00000001
#define NJD_DRAW_CONNECTED 0x00000002
#define NJD_FILL 0x00000020
#define NJD_TRANSPARENT 0x00000040
#define NJD_USE_TEXTURE 0x80000000

void njInit3D(NJS_VERTEX_BUF *vbuf, Int vn);
#define NJD_CONTROL_3D_DISP_AXIS 0x00000001
#define NJD_CONTROL_3D_NO_CLIP_CHECK 0x00000002
#define NJD_CONTROL_3D_CONSTANT_ATTR 0x00000004
#define NJD_CONTROL_3D_ENABLE_ALPHA 0x00000008
#define NJD_CONTROL_3D_CONSTANT_MATERIAL 0x00000010
#define NJD_CONTROL_3D_OFFSET_MATERIAL 0x00000020
#define NJD_CONTROL_3D_DEPTH_QUEUE 0x00000040
#define NJD_CONTROL_3D_VERTEX_OFFSET 0x00000080
#define NJD_CONTROL_3D_MODEL_CLIP 0x00000100
#define NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL 0x00000200
#define NJD_CONTROL_3D_SHADOW 0x00000400
#define NJD_CONTROL_3D_SHADOW_OPAQUE 0x00001000
void njControl3D(Uint32 flag);
void njSetConstantAttr(Uint32 and_attr, Uint32 or_attr);
void njSetConstantMaterial(NJS_ARGB *mat);
void njSetDepthQueue(Float dnear, Float dfar);
void njInitMatrix(NJS_MATRIX *m, Sint32 n, Int flag);
/* keep buf for the shape motion draws to morph models into: 3 Floats for each
 * point of a morphed model, or 6 where both its points and normals morph */
void njInitShape(Float *buf);
/* Ninja's built-in font, which njInitPrint loads when texture is null */
extern Uint8 _nj_debug_char_[];
void njInitPrint(Uint32 *texture, Uint32 size, Uint32 attr);
void njExitPrint(void);
void njPrintSize(Uint16 size);
/* a screen position in characters, for the njPrint calls: x in the high half
 * and y in the low */
#define NJM_LOCATION(x, y) ((x) << 16 | (y))
void njPrintColor(Uint32 c);
void njPrintC(Int loc, const char *s);
void njPrintD(Int loc, Int val, Int digit);
void njPrintB(Int loc, Uint32 val, Int digit);
void njPrintH(Int loc, Uint32 val, Int digit);
void njPrintF(Int loc, Float val, Int digit);
void njPrint(Int loc, const char *fmt, ...);

void njInitView(NJS_VIEW *v);
void njSetView(NJS_VIEW *v);
void njSetCamera(NJS_CAMERA *c);
void njPointCameraInterest(NJS_CAMERA *c, Float x, Float y, Float z);
void njRollCameraInterest(NJS_CAMERA *c, Angle ang);
void njSetCameraAngle(NJS_CAMERA *c, Angle ang);
/* the njRotateView and njTranslateView calls write only v's direction, vx to
 * vz, its roll or its position, px to pz, and a null v selects the current
 * view. njSetView(v) rebuilds v's matrix from those, so a call on the current
 * view changes no drawing until njSetView */
void njRotateViewXRelative(NJS_VIEW *v, Angle ang);
void njRotateViewYRelative(NJS_VIEW *v, Angle ang);
void njRotateViewZRelative(NJS_VIEW *v, Angle ang);
void njTranslateViewRelative(NJS_VIEW *v, Float x, Float y, Float z);
void njRotateViewXAbsolute(NJS_VIEW *v, Angle ang);
void njRotateViewYAbsolute(NJS_VIEW *v, Angle ang);
void njRotateViewZAbsolute(NJS_VIEW *v, Angle ang);
void njTranslateViewAbsolute(NJS_VIEW *v, Float x, Float y, Float z);
void njSetScreen(NJS_SCREEN *s);
void njSetPerspective(Angle ang);
void njClipZ(Float n, Float f);
/* njUserClipping modes: off, or drawing only inside or only outside the tiles
 * from p[0] to p[1], which every mode reads */
#define NJD_CLIP_DISABLE 0
#define NJD_CLIP_INSIDE 2
#define NJD_CLIP_OUTSIDE 3
void njUserClipping(Int mode, NJS_POINT2 *p);

/* empty the matrix stack and make its base the current matrix: the current
 * view's matrix after a scale of -1 in x and y. copy the base to
 * _nj_view_matrix_ and to XMTRX. it is declared without a prototype because
 * some calls pass an argument, which it doesn't read */
void njClearMatrix();
Bool njPushMatrix(NJS_MATRIX *m);
Bool njPopMatrix(Uint32 n);
void njUnitMatrix(NJS_MATRIX *m);
void njSetMatrix(NJS_MATRIX *md, NJS_MATRIX *ms);
void njGetMatrix(NJS_MATRIX *m);
void njMultiMatrix(NJS_MATRIX *md, NJS_MATRIX *ms);
void njTranslate(NJS_MATRIX *m, Float x, Float y, Float z);
void njTranslateV(NJS_MATRIX *m, NJS_VECTOR *v);
void njRotate(NJS_MATRIX *m, NJS_VECTOR *v, Angle ang);
void njRotateX(NJS_MATRIX *m, Angle ang);
void njRotateY(NJS_MATRIX *m, Angle ang);
void njRotateZ(NJS_MATRIX *m, Angle ang);
void njRotateXYZ(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz);
void njRotateZXY(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz);
Bool njInvertMatrix(NJS_MATRIX *m);
Float njDetMatrix(NJS_MATRIX *m);
void njScale(NJS_MATRIX *m, Float sx, Float sy, Float sz);
void njScaleV(NJS_MATRIX *m, NJS_VECTOR *v);
void njMirror(NJS_MATRIX *m, NJS_PLANE *pl);
void njAddVector(NJS_VECTOR *vd, NJS_VECTOR *vs);
void njSubVector(NJS_VECTOR *vd, NJS_VECTOR *vs);
void njCalcPoint(NJS_MATRIX *m, NJS_POINT3 *ps, NJS_POINT3 *pd);
void njCalcVector(NJS_MATRIX *m, NJS_VECTOR *vs, NJS_VECTOR *vd);
void njGetTranslation(NJS_MATRIX *m, NJS_POINT3 *p);
void njProjectScreen(NJS_MATRIX *m, NJS_POINT3 *p3, NJS_POINT2 *p2);
Float njScalor(NJS_VECTOR *v);
Float njScalor2(NJS_VECTOR *v);

/* SHC intrinsics: fabsf, sqrtf, FSRRA, the reciprocal square root, and FSCA, an
 * angle's sine and cosine */
#ifdef __clang__
#define _builtin_fabsf __builtin_fabsf
#define _builtin_sqrtf __builtin_sqrtf
Float _builtin_fsrra(Float n);
void _builtin_fsca(Angle ang, Float *sin, Float *cos);
#endif
#define njAbs(n) ((Float)_builtin_fabsf((Float)(n)))

/* degrees as an Angle */
#define NJM_DEG_ANG(n) ((Angle)(((n) * 65536.0) / 360.0))

Float njSin(Angle n);
Float njSqrt(Float n);
Float njInvertSqrt(Float n);
Float njCos(Angle n);
Float njTan(Angle n);
/* odata[0..2] = idata[0..2] * (1 - frame) + idata[3..5] * frame. attr is not
 * read */
void njLinear(Float *idata, Float *odata, NJS_SPLINE *attr, Float frame);
Float njUnitVector(NJS_VECTOR *v);
Float njInnerProduct(NJS_VECTOR *v1, NJS_VECTOR *v2);
Float njOuterProduct(NJS_VECTOR *v1, NJS_VECTOR *v2, NJS_VECTOR *ov);
void njGetPlaneNormal2(NJS_POINT3 *p0, NJS_POINT3 *p1, NJS_POINT3 *p2,
                       NJS_VECTOR *v);
Float njDistanceP2P(NJS_POINT3 *p1, NJS_POINT3 *p2);
Float njDistanceP2L(NJS_POINT3 *p, NJS_LINE *l, NJS_POINT3 *cp);
Float njDistanceP2PL(NJS_POINT3 *p, NJS_PLANE *pl, NJS_POINT3 *cp);
Float njDistanceL2PL(NJS_LINE *l, NJS_PLANE *pl, NJS_POINT3 *cp);
Float njDistancePL2PL(NJS_PLANE *pl1, NJS_PLANE *pl2, NJS_LINE *l);
Bool njIsParalellL2PL(NJS_LINE *l, NJS_PLANE *pl);
Bool njIsParalellPL2PL(NJS_PLANE *pl1, NJS_PLANE *pl2);
/* the fractional part of n, as modff gives it, with n's integral part left in
 * FracInt */
Float njFraction(Float n);
extern Float FracInt;
/* n rounded toward zero: floorf(|n|), negated for n below 0 */
Float njRoundOff(Float n);
/* 1 when the spheres overlap, else 0 */
Int njCollisionCheckSS(NJS_SPHERE *sphere1, NJS_SPHERE *sphere2);

/* set the motion callback: the motion draws call func with each node, once its
 * matrix is moved and before its model is drawn. NULL sets none */
void njSetMotionCallback(void (*func)(NJS_OBJECT *obj));

/* draw model through the current matrix, in XMTRX, between calls of
 * njGetkmPointer and njSetkmPointer, lit by the first created light, each
 * meshset as its material's flags ask. a model with more points than
 * _nj_vertex_num_ sends nothing, and a meshset whose material has
 * NJD_FLAG_USE_ENV without NJD_FLAG_USE_TEXTURE sends no polygon.
 * njSimpleDrawObject draws obj, its children and its siblings, each through
 * its own transform. it skips a node's model under NJD_EVAL_HIDE, and its
 * children under NJD_EVAL_BREAK */
void njSimpleDrawModel(NJS_MODEL *model);
void njSimpleDrawObject(NJS_OBJECT *obj);
/* project model's points through XMTRX into _nj_vertex_buf_, turn the light
 * njSimpleDrawModel or njSimpleDrawObject last took into model space for the
 * drawers, and return 0.
 *
 * it returns -1 instead when every point lies beyond the same side of the clip
 * area, when every point lies beyond the far plane, or when every point lies
 * before the near plane. under NJD_CONTROL_3D_MODEL_CLIP, it also returns -1
 * when the model's bounding sphere lies wholly off the clip area, beyond the
 * far plane or before the near plane. NJD_CONTROL_3D_NO_CLIP_CHECK skips the
 * points' tests and keeps the sphere's.
 *
 * model->nbPoint must be at most _nj_vertex_num_, and the point past the last
 * must be readable: the projection reads it */
Sint32 njSimpleCalcModel(NJS_MODEL *model);
/* njSimpleDrawModel and njSimpleDrawObject lit by the light last taken, inside
 * the caller's own njGetkmPointer and njSetkmPointer */
void njSimpleDrawModelSub(NJS_MODEL *model);
void njSimpleDrawObjectSub(NJS_OBJECT *obj);
/* draw as njDrawMotion, njAction and their Shape and Old forms do, but with
 * njSimpleDrawModel, and the whole tree inside one pair of njGetkmPointer and
 * njSetkmPointer calls. a node under NJD_EVAL_SKIP turns by the three words at
 * the draw's stack pointer, which hold saved registers, not by its ang.
 * njSimpleDrawShapeMotion sets _nj_draw_model_sub_ to njSimpleDrawModel for
 * the shape steps; njSimpleDrawShapeMotionOld's shape steps draw with
 * _nj_draw_model_sub_ as it stands */
void njSimpleAction(NJS_ACTION *action, Float frame);
void njSimpleDrawMotion(NJS_OBJECT *obj, NJS_MOTION *motion, Float frame);
void njSimpleDrawShapeMotion(NJS_OBJECT *obj, NJS_MOTION *motion,
                             NJS_MOTION *shape, Float frame);
void njSimpleActionOld(NJS_ACTION *action, Float frame);
void njSimpleDrawMotionOld(NJS_OBJECT *obj, NJS_MOTION *motion, Float frame);
void njSimpleDrawShapeMotionOld(NJS_OBJECT *obj, NJS_MOTION *motion,
                                NJS_MOTION *shape, Float frame);
/* draw the tree posed between a link's two motions, each at its frame in the
 * link: the pose is blended from the first motion's toward the second's by
 * rate. a null motion gives each node its own pos, ang and scl as that
 * motion's pose. each node's matrix is its parent's, moved by the blend,
 * turning in the order the root's NJD_EVAL_ZXY_ANG gives. a node under
 * NJD_EVAL_SKIP moves by its own pos, ang and scl instead, in its own order.
 * the motion callback, if one is set, then takes the node, and
 * njSimpleDrawModel draws its model unless NJD_EVAL_HIDE.
 *
 * both motions take one interpolation: the second motion's, or the first's
 * when the second is null, or when both are null the one _nj_mot_spline_cnt_
 * holds already. nothing is
 * drawn for a rate above 1, or for a frame not before its motion's nbFrame.
 * the Shape forms also blend shapelink's two shape motions into the models'
 * points and normals, and each Old form draws as its other form does */
void njSimpleActionLink(NJS_ACTION_LINK *actionlink, Float rate);
void njSimpleDrawMotionLink(NJS_OBJECT *object, NJS_MOTION_LINK *motionlink,
                            Float rate);
void njSimpleDrawShapeMotionLink(NJS_OBJECT *object,
                                 NJS_MOTION_LINK *motionlink,
                                 NJS_MOTION_LINK *shapelink, Float rate);
void njSimpleActionLinkOld(NJS_ACTION_LINK *actionlink, Float rate);
void njSimpleDrawMotionLinkOld(NJS_OBJECT *object, NJS_MOTION_LINK *motionlink,
                               Float rate);
void njSimpleDrawShapeMotionLinkOld(NJS_OBJECT *object,
                                    NJS_MOTION_LINK *motionlink,
                                    NJS_MOTION_LINK *shapelink, Float rate);
/* njGetkmPointer saves the current vertex context's parameter buffers and the
 * vertex buffer's list write pointers, and njSetkmPointer restores them */
void njGetkmPointer(void);
void njSetkmPointer(void);
/* Ninja's end-of-render callback, which Kamui calls with an argument it
 * ignores */
/* @hack type: declared without a prototype, so timer_end_of_render can call
 * it with no argument, as 1ST_READ.BIN does (0x8c06fc48); prototyped, and
 * passed timer_end_of_render's own argument, 2 of its 10 words differ */
/* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
void njSetEORflag();
/* how Ninja's own draw calls send vertices, between njGetkmPointer and
 * njSetkmPointer. a mode sets the polygon type: Gouraud shaded, in the opaque
 * or the translucent list, with or without a texture */
#define COLOR_MODE_OPAQUE 0x00080002
#define COLOR_MODE_TRANS 0x02100002
#define COLOR_MODE_TEXTURE 0x00000008
/* each vertex gives one intensity, which scales the header's color */
#define COLOR_MODE_INTENSITY 0x00000020
/* each vertex adds an offset color, the specular */
#define COLOR_MODE_OFFSET 0x00000004
/* Gouraud shading. without it, a triangle takes the color of the vertex that
 * completes it */
#define COLOR_MODE_GOURAUD 0x00000002
void njSetkmColorMode(Uint32 mode);
/* each writes a polygon header at the end of its list and returns the store
 * queue address for the first vertex */
void *njStartVertexNonTexOpaque(void);
void *njStartVertexNonTexTrans(void);
void *njStartVertexTexOpaque(void);
void *njStartVertexTexTrans(void);
/* the same, with the four words of color after the header's first four: the
 * color the vertices' intensities scale */
void *njStartVertexNonTexOpaqueIntens(Uint32 *color);
void *njStartVertexNonTexTransIntens(Uint32 *color);
void *njStartVertexTexOpaqueIntens(Uint32 *color);
void *njStartVertexTexTransIntens(Uint32 *color);
/* set the write pointer of the list the last njStart call opened to vertex,
 * the end of the vertices written */
void njEndVertex(Uint32 *vertex);
/* make texture n of the current texture list current, setting the TSP word's
 * size bits and the texture word from its record, and return 1. a texture
 * that is current already changes nothing. a texture that starts more than
 * 8 MB into video memory is made current with its size bits but not its
 * texture word, and returns -1 */
Sint32 njSetkmTextureNum(Uint32 n);
/* copy attr's bits 0xfc07ff00, those of the TSP word that a material's flags
 * hold, into the TSP word */
void njSetkmModelAttr(Uint32 attr);

/* two of the SDK's color modes, which njSetColorMode and njSetkmColorMode
 * take: Gouraud-shaded polygons with floating-point colors and no texture, in
 * the opaque and the translucent list */
#define NJD_COLOR_MODE_NORMAL 0x00080012
#define NJD_COLOR_MODE_NORMAL_TRANS 0x02100012

/* draw a model, or an object tree, in the current matrix, lit by the created
 * lights that are on. a model of more points than _nj_vertex_num_ draws
 * nothing. a model whose bounding sphere lies out of view under
 * NJD_CONTROL_3D_MODEL_CLIP, or whose points all lie outside one clip plane
 * unless NJD_CONTROL_3D_NO_CLIP_CHECK is set, draws nothing but the axes
 * NJD_CONTROL_3D_DISP_AXIS asks for.
 *
 * njDrawObject draws obj and its siblings. each node draws its model unless
 * NJD_EVAL_HIDE, and its children unless NJD_EVAL_BREAK. no node's model or
 * child is tested for null, so a node needs a model unless NJD_EVAL_HIDE, and
 * a child unless NJD_EVAL_BREAK */
void njDrawModel(NJS_MODEL *model);
void njDrawObject(NJS_OBJECT *obj);
/* draw obj's tree, or action's object's, as motion poses it at frame. nothing
 * is drawn unless frame is below the motion's nbFrame. a null motion leaves
 * each node its own pos, ang and scl. each node's matrix is its parent's,
 * moved by the node's record of the motion, turning in the order the root's
 * NJD_EVAL_ZXY_ANG gives. a node under NJD_EVAL_SKIP moves by its own pos, ang
 * and scl instead, in its own order. _nj_motion_callback_, if set, then takes
 * the node, and njDrawModelSub draws its model unless NJD_EVAL_HIDE, with the
 * light setup njDrawModel runs, here once for the whole tree.
 *
 * the shape draws also need frame below the shape's nbFrame, and morph each
 * model by shape. a null shape, or a node under NJD_EVAL_SHAPE_SKIP, leaves
 * the model as it is. the Old draws interpolate a linear motion's angles each
 * the short way round */
void njDrawMotion(NJS_OBJECT *obj, NJS_MOTION *motion, Float frame);
void njAction(NJS_ACTION *action, Float frame);
void njDrawShapeMotion(NJS_OBJECT *obj, NJS_MOTION *motion, NJS_MOTION *shape,
                       Float frame);
void njDrawMotionOld(NJS_OBJECT *obj, NJS_MOTION *motion, Float frame);
void njActionOld(NJS_ACTION *action, Float frame);
void njDrawShapeMotionOld(NJS_OBJECT *obj, NJS_MOTION *motion,
                          NJS_MOTION *shape, Float frame);
/* draw model, or obj with its children and siblings, through the current
 * matrix, in XMTRX, lit by the easy light, each vertex shaded by one
 * intensity. strips are drawn whole, and triangles and quads are culled by
 * their winding on the screen. when any point has z 0 or less, each polygon
 * that reaches nearer than z = 1 is cut at z = 1. the points are projected into
 * _nj_vertex_buf_.
 *
 * a model is not drawn when it has more than _nj_vertex_num_ points, when
 * every point has z 0 or less, or, under NJD_CONTROL_3D_MODEL_CLIP, when its
 * bounding sphere lies outside the clip planes or _nj_2d_clip_. a model must
 * have a point and a meshset, each meshset a polygon, and each strip three
 * points. an NJD_MESHSET_N meshset sends its header and no vertices */
void njEasyDrawModel(NJS_MODEL *model);
void njEasyDrawObject(NJS_OBJECT *obj);
/* draw as the motion draws above do, but each model with njEasyDrawModel. the
 * motion and the shape interpolate linearly whatever their interpolation, and
 * each angle as a plain number, not the short way round. a morphed normal is
 * scaled to unit length, as FSRRA approximates it, or is (0, 0, 1) where the
 * blend's squared length is not above 0.00001. under NJD_EVAL_SKIP,
 * njEasyDrawMotion and njEasyAction turn a node by the three words at the
 * draw's stack pointer, which hold saved registers, not by its ang */
void njEasyDrawMotion(NJS_OBJECT *obj, NJS_MOTION *motion, Float frame);
void njEasyAction(NJS_ACTION *action, Float frame);
void njEasyDrawShapeMotion(NJS_OBJECT *obj, NJS_MOTION *motion,
                           NJS_MOTION *shape, Float frame);
/* 1 when no point's z is beyond _nj_far_clip_, else 0 */
Int njFarClipping(NJS_POINT3COL *p, Int n);
/* store the part of a polygon or a line on the visible side of the near plane,
 * and return its point count */
Int njNearClipping(NJS_POINT3COL *in, Int n, NJS_POINT3COL *out, Uint32 attr);
Int njNearClippingLine(NJS_POINT3COL *in, NJS_POINT3COL *out, Uint32 attr);
/* send the points the clipping leaves as triangles, filled or outlined, plain
 * or textured, writing vertices from *vertex */
void njtri_filled(Int n, NJS_POINT3COL *p, Float **vertex);
void njtri_filled_with_tex(Int n, NJS_POINT3COL *p, Float **vertex);
void njtri_wiredpoly(Int n, NJS_POINT3COL *p, Float **vertex);
void njtri_wiredpoly_with_tex(Int n, NJS_POINT3COL *p, Float **vertex);
/* write the position, angles and scale motion gives each node of object at
 * frame into data->pos, data->ang and data->scl, three values a node: each
 * node before its children, and they before its siblings. nothing is written
 * unless frame is below the motion's nbFrame. a null motion, or NJD_EVAL_SKIP
 * on a node, gives the node's own pos, ang and scl */
void njGetDrawMotion(NJS_OBJECT *object, NJS_MOTION *motion,
                     NJS_MOTION_DATA *data, Float frame);
/* njGetDrawMotion of action's object and motion */
void njGetAction(NJS_ACTION *action, NJS_MOTION_DATA *data, Float frame);
/* set the view, as njSetCamera does, to a copy of camera moved by motion at
 * frame. njCameraAction and njCameraActionOld take camera and motion from
 * caction. motion must be set, and frame below its nbFrame.
 *
 * the motion's type names the factors its record holds, one each, in bit
 * order. NJD_MTYPE_POS_0 is the copy's position, by the motion's
 * interpolation. NJD_MTYPE_VEC_3 is a vector from the position to a point the
 * copy turns to look at, and NJD_MTYPE_TARGET_3 is such a point itself.
 * NJD_MTYPE_ROLL_6 is an angle njRollCameraInterest rolls the copy by, from
 * roll 0, or in the Old forms from the copy's roll. NJD_MTYPE_ANGLE_7 is the
 * copy's angle of view. every factor but the position takes its keys
 * linearly. the factors of NJD_MTYPE_ANG_1, NJD_MTYPE_SCL_2, 0x200, 0x400 and
 * 0x1800 are passed over, and 0x1800 names one factor, by either of its bits
 * or both. with both NJD_MTYPE_VEC_3 and NJD_MTYPE_TARGET_3 set,
 * NJD_MTYPE_VEC_3's factor isn't passed over: NJD_MTYPE_TARGET_3 reads its
 * keys as the point, and each factor after it reads the keys of the one
 * before.
 *
 * njCameraAction and njCameraActionOld take a record of one to five factors,
 * and njCameraMotion and njCameraMotionOld take one of one to four. another
 * count sets no view */
void njCameraAction(NJS_CACTION *caction, Float frame);
void njCameraMotion(NJS_CAMERA *camera, NJS_MOTION *motion, Float frame);
void njCameraActionOld(NJS_CACTION *caction, Float frame);
void njCameraMotionOld(NJS_CAMERA *camera, NJS_MOTION *motion, Float frame);
void njDrawLine3D(NJS_POINT3COL *p, Int n, Uint32 attr);
void njDrawTriangle3D(NJS_POINT3COL *p, Int n, Uint32 attr);
void njDrawPolygon3D(NJS_POINT3COL *p, Int n, Uint32 attr);
void njDrawLine2D(NJS_POINT2COL *p, Int n, Float pri, Uint32 attr);
void njDrawPolygon2D(NJS_POINT2COL *p, Int n, Float pri, Uint32 attr);
void njDrawSprite2D(NJS_SPRITE *sp, Int n, Float pri, Uint32 attr);
void njDrawSprite3D(NJS_SPRITE *sp, Int n, Uint32 attr);
/* draw the count vertices of polygon, count at least 1, as one strip of
 * Gouraud-shaded triangles, into the translucent list when trans is nonzero and
 * else into the opaque, and leave the current vertex context in that color
 * mode. njDrawTexture takes flag in trans's place, and maps the loaded texture
 * whose global index is tex, which it leaves current. njDrawTextureMemList maps
 * record tex of the texture memory list instead */
void njDrawPolygon(NJS_POLYGON_VTX *polygon, Int count, Int trans);
void njDrawTexture(NJS_TEXTURE_VTX *polygon, Int count, Int tex, Int flag);
void njDrawTextureMemList(NJS_TEXTURE_VTX *polygon, Int count, Int tex,
                          Int flag);

void njFogEnable(void);
void njFogDisable(void);
void njSetFogColor(Uint32 c);
void njSetFogDensity(Uint32 density);
/* -f as the fog density register takes it, m * 2^e: bits 15 to 8 hold m, from
 * 1 to 2, truncated to 1.7 fixed point, and bits 7 to 0 hold e, a signed byte.
 * f below -65536 gives FOG_DENSITY_65536, and f of 0 or more gives m 0. f must
 * not be a NaN or +infinity, for which frexp leaves e unset */
Uint32 njGenerateFogDensity(Float f);
/* 2^16 as the fog density register takes it: 1 in bits 15 to 8, as 1.7 fixed
 * point, and 16 in bits 7 to 0 */
#define FOG_DENSITY_65536 0x8010
void njGenerateFogTable(NJS_FOG_TABLE fog);
/* set the fog density, and fill fog, a table for fog between the depths n and
 * f. a depth is a z, so -n and -f are the distances. entry i stands for the
 * distance -f / (2^(i / 16) * (16 + i % 16) / 16), from -f at entry 0 to
 * -f / 248 at entry 127. where -n is at most -f / 256, fog is
 * njGenerateFogTable's. otherwise fog falls linearly with distance, from 1 at
 * -f to 0 at m, the distance of the first entry no farther than -n, or -f / 256
 * where every entry is farther. the entries nearer than m hold 0. the density
 * is FOG_DENSITY_65536 when n is below -65535, and else -f, packed as
 * njGenerateFogDensity packs it but with no limit; f then must not be a NaN or
 * an infinity */
void njGenerateFogTable3(NJS_FOG_TABLE fog, Float n, Float f);
void njSetFogTable(NJS_FOG_TABLE fog);
/* a fade is table fog: njExecuteFade sets every entry of the table to f, and
 * njSetFadeColor sets its color. njFadeEnable sets the current vertex context's
 * fog to the table and njFadeDisable turns it off, as njFogEnable and
 * njFogDisable do */
void njExecuteFade(Float f);
void njSetFadeColor(Uint32 c);
void njFadeEnable(void);
void njFadeDisable(void);

/* give Kamui a vertex buffer for its opaque, opaque modifier, translucent,
 * translucent modifier and punch-through lists, of op, om, tp, tm and pt
 * words. the punch-through list's words go to the translucent list.
 *
 * a negative op selects Kamui's 2V latency, which takes no buffer for the
 * opaque list, and op then counts as 100 words. failing that, a negative tp
 * or pt selects 2V latency with no buffer for the translucent list, and tp
 * then counts as 100 words and pt as 0.
 *
 * the buffer comes from syMalloc, or from njSetVertexBuffer once that is
 * called; Kamui gets none if njSetVertexBuffer's is too small. it waits for a
 * vertical blank */
void njInitVertexBuffer(Sint32 op, Sint32 om, Sint32 tp, Sint32 tm, Sint32 pt);
/* call func at each vertical blank, in place of the last function set, whose
 * handler is deleted from the vertical blank chain. a NULL func deletes the
 * last handler but keeps its handle, and the next call deletes it again.
 * njInitSystem drops the handle without deleting the handler */
void njSetVSyncFunction(void (*func)(void));
/* if a render has ended since njInitSystem or njChangeSystem, wait, for about
 * 272 ms at most, for the render the last call started to end, and call the
 * function njSetEORFunction set if it ended in time. then render the frame,
 * flip the frame buffers and serve the peripherals */
void njWaitVSync(void);
void njInitTextureBuffer(Sint8 *addr, Uint32 size);
void njInitTexture(NJS_TEXMEMLIST *addr, Uint32 n);
void njInitTextureGlobalIndex(Uint32 globalIndex);
void njExitTexture(void);
void njReleaseTextureAll(void);
void njGarbageTexture(NJS_TEXMEMLIST *addr, Uint32 n);
NJS_TEXERRSTAT *njGetTextureErrStat(void);
/* njCalcTexture's flag. NJD_TEXMEM_FREESIZE returns the total free texture
 * memory, NJD_TEXMEM_MAXSIZE the free total njInitVertexBuffer recorded, and
 * any other flag the largest free block */
#define NJD_TEXMEM_FREESIZE 0
#define NJD_TEXMEM_MAXSIZE 2
Uint32 njCalcTexture(Uint32 flag);
void njSetTexturePath(const char *path);
Sint32 njSetTexture(NJS_TEXLIST *texlist);
Sint32 njSetTextureNum(Uint32 n);
void njSetTextureMemorySize(Uint32 size);
Sint32 njLoadTexture(NJS_TEXLIST *texlist);
Sint32 njReleaseTexture(NJS_TEXLIST *texlist);
Sint32 njReleaseTextureNum(Uint32 n);
/* the texture cache: main memory that holds textures' pixels, so that a
 * released texture can load again without reading the disc */
void njInitCacheTextureBuffer(Sint8 *addr, Uint32 size);
void njReleaseCacheTextureAll(void);
/* the bytes of a name in a PVM archive's entry table */
#define PVM_NAME_SIZE 28
/* point texlist at texname's size names, each given PVM_NAME_SIZE bytes of
 * filename for its entry's name; nbTexture is left as it was */
void njSetPvmTextureList(NJS_TEXLIST *texlist, NJS_TEXNAME *texname,
                         char *filename, Sint32 size);
/* load the PVM archive that filename names, under the texture path, into
 * texlist, which must have a name for each of the archive's textures. .PVM is
 * added to a filename that doesn't end in it, ignoring case. returns 1, or -1
 * if the file can't be opened, isn't a PVM archive, or a read or a load
 * fails. a failure sets _nj_texerr_, but a failed read of the entries goes
 * unchecked.
 *
 * nbTexture takes the archive's texture count, and the names take its entries
 * in order. each entry has a number, which its name's attr takes in its low
 * 16 bits. where the archive gives them, the entry's PVM_NAME_SIZE-byte name
 * goes to the bytes the name's filename points to, and its global index to
 * texaddr. where the archive banks its indexes, a palette bank sits above the
 * index, and the name takes NJD_TEXATTR_PALGLOBALINDEX.
 *
 * the archive's textures then load in turn: the first into the name numbered
 * 0, the next into the name numbered 1, and so on. a name takes its texture
 * by the global index its texaddr holds, and texaddr becomes the address of
 * the texture's record. a palette chunk before the first texture sets the
 * palette's mode and colors, and sets the current palette bank to the mode.
 *
 * under NJD_TEXATTR_BOTH, a failed cache copy still returns 1 and marks the
 * record cached. a palette name whose texture is listed only in other banks
 * stores that texture's record's address through an unset pointer. the
 * global index its texaddr still holds then serves as a record's address: a
 * failed load writes its error 66 bytes past it. if that texture is loaded
 * but not cached, and the name asks for NJD_TEXATTR_CACHE or
 * NJD_TEXATTR_BOTH, the name's cache copy reads the archive's chunk after the
 * texture's, and the texture's info goes 20 bytes past that global index. the
 * loop over the numbers then goes on from _nj_texmemsize + 1, whatever number
 * it had reached */
Sint32 njLoadTexturePvmFile(const char *filename, NJS_TEXLIST *texlist);
/* load the PVM archive at addr into texlist, as njLoadTexturePvmFile loads a
 * file's. returns 1, or -1 if addr isn't a PVM archive or a load fails; a
 * failure sets _nj_texerr_. it differs from njLoadTexturePvmFile as follows.
 * a banked global index goes to njSetTexListToPvmM's banked_index, which
 * nothing reads, so its name keeps its texaddr and takes
 * NJD_TEXATTR_PALGLOBALINDEX. a palette chunk's colors are read 4 bytes each
 * only from a chunk whose bank is 6, and 2 bytes each from any other. a
 * palette name whose texture is listed only in other banks reads that
 * texture's record's bank as its read area, and stores through no unset
 * pointer. no bank the loaders record, 0 to 63 or -1, selects
 * NJD_TEXATTR_CACHE or NJD_TEXATTR_BOTH. 0 to 63 sets neither bit. -1 sets
 * both bits, which together select nothing, so the name gets no record and
 * keeps its texaddr. a texture it caches keeps the archive's own pixels as
 * its cache copy, so the archive must stay in place while the texture is
 * cached */
Sint32 njLoadTexturePvmMemory(Uint8 *addr, NJS_TEXLIST *texlist);
/* texture formats of pixels in memory, and texture name attributes */
#define NJD_TEXFMT_ARGB_1555 0x00
#define NJD_TEXFMT_RGB_565 0x01
#define NJD_TEXFMT_ARGB_4444 0x02
#define NJD_TEXFMT_ARGB_8888 0x06
#define NJD_TEXFMT_TWIDDLED 0x0100
#define NJD_TEXFMT_PALETTIZE4 0x0500
#define NJD_TEXFMT_PALETTIZE4_MM 0x0600
#define NJD_TEXFMT_PALETTIZE8 0x0700
#define NJD_TEXFMT_PALETTIZE8_MM 0x0800
#define NJD_TEXFMT_RECTANGLE 0x0900
#define NJD_TEXFMT_ABGR 0x0e00
#define NJD_TEXFMT_ABGR_MM 0x0f00
#define NJD_TEXFMT_COLOR_MASK 0xff
#define NJD_TEXFMT_TYPE_MASK 0xff00
/* where a name's pixels go: video memory, under CACHE the texture cache alone,
 * under BOTH both */
#define NJD_TEXATTR_CACHE 0x80000000
#define NJD_TEXATTR_BOTH 0x20000000
#define NJD_TEXATTR_READAREA_MASK (NJD_TEXATTR_CACHE | NJD_TEXATTR_BOTH)
#define NJD_TEXATTR_TYPE_MEMORY 0x40000000
/* the name's texaddr holds the texture's global index, under PALGLOBALINDEX
 * below its palette bank */
#define NJD_TEXATTR_PALGLOBALINDEX 0x01000000
#define NJD_TEXATTR_GLOBALINDEX 0x00800000
#define NJD_TEXATTR_AUTOMIPMAP 0x00400000
/* a name after the first takes the texture of the name before it, in the
 * palette bank its own texaddr holds */
#define NJD_TEXATTR_TEXCONTINUE 0x00010000
#define NJD_TEXBANK_SHIFT 26
#define NJD_TEXBANK_MASK 0xfc000000
#define NJD_TEXPALGLOBAL_MASK 0x03ffffff
void njSetTextureInfo(NJS_TEXINFO *info, Uint16 *tex, Sint32 type, Sint32 width,
                      Sint32 height);
void njSetTextureName(NJS_TEXNAME *texname, void *addr, Uint32 globalIndex,
                      Uint32 attr);
/* Ninja's internal texture functions, down to strupr: no source outside
 * sdk/ninja calls them */

/* a texture cache block of at least size bytes, or NULL when no free block is
 * that large. njCacheTextureFree frees one */
void *njCacheTextureMalloc(Uint32 size);
void njCacheTextureFree(void *addr);
/* the index in list of the texture with globalIndex, or -1 */
Sint32 njSearchTexList(NJS_TEXMEMLIST *list, Uint32 n, Uint32 globalIndex);
/* free a record, and move the list's end back past the free records it ends
 * with */
void njFreeTexMemList(NJS_TEXMEMLIST *tex);
/* fill a free record and store its address at *handle; texaddr is TEXMEM_
 * flags. with no record free, it fills the one past the list's end */
void njEntryTexMemList(Uint32 *handle, NJS_TEXINFO *info, Uint32 globalIndex,
                       Uint32 texaddr, Uint32 bank);
/* the index in list of the texture with globalIndex and bank, or -1 */
Sint32 njSearchTexList3(NJS_TEXMEMLIST *list, Uint32 n, Uint32 globalIndex,
                        Uint32 bank);
/* set flag in the texaddr of each record with globalIndex, and its pixels'
 * address to addr; with TEXMEM_LOADED, also give it one user and set its
 * texture words and palette bank */
void njSetFlagAddrAllTexture(Uint32 globalIndex, Uint32 flag, void *addr);
/* the texture info of the next free record, the one njEntryTexMemList would
 * fill; globalIndex is not read */
NJS_TEXINFO *njTexMemList(Uint32 globalIndex);
Sint32 njCheckPaletteType(Uint32 type); /* whether type is a palette format */
void njInitTextureErrStat(void); /* clear _nj_texerr_ */
/* open filename, and start njTexFread's reads at its first byte. it returns
 * 0, or -2 with _nj_texerr_.texerr set to NJD_TEXERR_FILEOPEN if the file
 * can't be opened or sized. a file that can't be sized stays open */
Sint32 njTexFopen(const char *filename);
void njTexFclose(void);
/* read size bytes into buf from the next byte to read, and move past them.
 * the sector buffer, _nj_file_buffer, gives what it holds of them first, then
 * whole buffers' worth go directly into buf, and the rest comes through the
 * buffer. it returns 1. it returns -1, and sets _nj_texerr_, for a size of 0,
 * a read past the end of the file or a failed seek or read */
Sint32 njTexFread(void *buf, Uint32 size);
/* move the next byte to read by offset under GDD_SEEK_CUR, to offset under
 * GDD_SEEK_SET, or to offset back from the end under GDD_SEEK_END. another
 * type leaves it */
void njTexFseek(Sint32 offset, Sint32 type);
Sint32 njCreateTextureSurface(NJS_TEXSURFACE *surface, Sint32 width,
                              Sint32 height, Sint32 type);
Sint32 njSetTexInfo(Sint32 *type, Sint32 width, Sint32 height, Sint32 bit,
                    Sint32 cn);
/* load the open file's texture into a new surface, a buffer at a time
 * through _nj_texture_buffer_st. a small VQ texture, and the bytes after the
 * texture in its chunk, take one read each, whatever the buffer's size. a
 * failed read frees the surface */
Sint32 njLoadTexMemFromFile(NJS_TEXNAME *texname, NJS_TEXINFO *info);
/* read the open file's texture into a new texture-cache block, which a
 * failed read leaves allocated */
Sint32 njLoadCacheFromFile(NJS_TEXNAME *texname, NJS_TEXINFO *info);
Sint32 njLoadTexMemFromMemory(NJS_TEXNAME *texname, NJS_TEXINFO *info);
Sint32 njLoadTexMemFromCache(NJS_TEXNAME *texname, NJS_TEXINFO *info);
Sint32 njGetInfoMemory(NJS_TEXINFO *info);
Sint32 njGetSurfacePvr(NJS_TEXINFO *info, Uint32 attr, Sint32 *size);
Sint32 njGetInfoPvr(NJS_TEXINFO *info, Uint32 attr, Sint32 *size);
Sint32 njTexDownloadMipMap(void *tex, NJS_TEXINFO *info);
/* copy the pixels at info->texaddr into the texture cache, point info->texaddr
 * at the copy, and return the copy's address. with no block free for it,
 * return -NJD_TEXERR_MAINMEMORY, which _nj_texerr_ also takes. tex is not
 * read. a mipmapped texture's copy is 4 bytes shorter than its TextureSize */
Sint32 njTexDownloadCache(void *tex, NJS_TEXINFO *info);
/* write the texture path and name to path, adding .PVR to a name that doesn't
 * end in one of the extensions in fileext, declared below. set _nj_tex_ext_,
 * and return 1. for a name shorter than 4 characters, it reads bytes before
 * the name's start */
Sint32 njCheckFileName(char *path, char *name);
/* the global index of the open file's texture: texname's own texaddr under
 * NJD_TEXATTR_GLOBALINDEX; else, for a .PVR file, its global index chunk, if
 * one comes before the texture chunk; else the highest index at or below
 * _nj_user_globalIndex that no texture record holds, which
 * _nj_user_globalIndex takes. -1 if a read fails; otherwise the file is left
 * at its start */
Sint32 njGetFileGlobalIndex(NJS_TEXNAME *texname);
/* the open file's texture type, from its texture chunk, or -1 if a read fails;
 * otherwise the file is left at its start */
Sint32 njGetFileTexType(void);
/* njTexLoad's file extension table, which njCheckFileName scans; njTexFile's
 * sector buffer; and the buffer textures load through, with its size */
extern char fileext[][5];
extern Sint8 _nj_file_buffer[2048];
extern Sint8 *_nj_texture_buffer_st;
extern Uint32 _nj_texture_buffer_max_size;
/* load texture n of texlist into a texture record, from the texture's file
 * or, under NJD_TEXATTR_TYPE_MEMORY, from memory, and set the name's texaddr to
 * the record's address. returns 1, or -1 if the load fails. under
 * NJD_TEXATTR_BOTH, a failed cache copy still returns 1 and marks the record
 * cached. a failure sets _nj_texerr_.
 *
 * for a texture in memory, the name's filename points to an NJS_TEXINFO
 * whose texaddr holds the PVR file. reading the file's chunk headers changes
 * that NJS_TEXINFO: its texaddr moves on to the pixels, and its surface takes
 * the texture chunk's width and height, and the chunk's type in Type's high
 * 16 bits */
Sint32 njLoadTextureLN(NJS_TEXLIST *texlist, Uint32 n);
/* drop one user of the texture whose record texaddr points to, and free its
 * video memory with the last user; -1 for a texture not in video memory */
Sint32 njReleaseTextureLN(Uint32 texaddr);
/* set a record's TSP and texture words from its surface, replacing both. the
 * TSP word takes only the size codes ConvertUVnum gives the width and height,
 * in bits 3 to 5 and 0 to 2. the texture word takes the mipmap, VQ, scan order
 * and stride flags from fSurfaceFlags, the pixel format from PixelFormat, and
 * pSurface / 8 as the address */
void njSetTextureParam(NJS_TEXMEMLIST *tex);
/* make the n records at addr the texture list, all free */
void njInitTexMemList(NJS_TEXMEMLIST *addr, Uint32 n);
/* set a palettized texture's palette bank, counted in banks of 16 colors.
 * its texture word's bits 21 to 26 take bank's low six bits for a 4-bit
 * texture, and only bits 4 and 5 for an 8-bit one, so there bank 5 draws as
 * bank 0. tex->bank takes all of bank. a texture that isn't palettized is
 * left alone */
void njSetPaletteBankLN(NJS_TEXMEMLIST *tex, Sint32 bank);
/* make bank the current vertex context's palette bank */
void njSetPaletteBank(Uint32 bank);
void strupr(char *s);
#define NJD_TEXPALETTE_16BPP 16
#define NJD_TEXPALETTE_32BPP 32
/* set the palette bank a texture draws with, by its index in the current list
 * or by its global index */
void njSetPaletteBankNum(Uint32 n, Sint32 bank);
void njSetPaletteBankNumG(Uint32 globalIndex, Sint32 bank);
/* the palette's color format, an NJD_TEXFMT_ value. njSetPaletteData and
 * njGetPaletteData then copy a Uint32 per color for NJD_TEXFMT_ARGB_8888, and a
 * Uint16 for any other. a mode other than NJD_TEXFMT_RGB_565,
 * NJD_TEXFMT_ARGB_4444 or NJD_TEXFMT_ARGB_8888 sets the palette to ARGB 1555,
 * though njGetPaletteMode returns the mode given */
void njSetPaletteMode(Uint32 mode);
Uint32 njGetPaletteMode(void);
void njSetPaletteData(Sint32 offset, Sint32 count, void *data);
void njGetPaletteData(Sint32 offset, Sint32 count, void *data);
Sint32 njLoadPaletteFile(const char *filename, NJS_TEXPALETTE *pl);
/* the background: a plane through three of the screen's corners, colored col1
 * at the top left, col2 at the top right and col3 at the bottom left */
void njSetBackColor(Uint32 col1, Uint32 col2, Uint32 col3);
void njSetBorderColor(Uint32 color);
/* render the next frame into a texture, by its index in the current list or by
 * its global index; njSetRenderWidth sets the width of such a texture */
void njRenderTextureNum(Uint32 n);
void njRenderTextureNumG(Uint32 globalIndex);
void njSetRenderWidth(Uint32 nWidth);
/* read the displayed frame buffer into buffer, then write it at bmp as a 24-bit
 * BMP; with a NULL bmp, only the read is done */
void njFrameBufferBmp(char *bmp, char *buffer);
#define NJD_TEXTUREFILTER_POINT_SAMPLE 0
#define NJD_TEXTUREFILTER_BILINEAR 1
#define NJD_TEXTUREFILTER_TRILINEAR 2
void njTextureFilterMode(Int mode);
/* njTextureClampMode and njTextureFlipMode modes */
#define NJD_TEXTURECLAMP_NOCLAMP 0
#define NJD_TEXTURECLAMP_CLAMP_V 1
#define NJD_TEXTURECLAMP_CLAMP_U 2
#define NJD_TEXTURECLAMP_CLAMP_UV 3
#define NJD_TEXTUREFLIP_NOFLIP 0
#define NJD_TEXTUREFLIP_FLIP_V 1
#define NJD_TEXTUREFLIP_FLIP_U 2
#define NJD_TEXTUREFLIP_FLIP_UV 3
void njTextureClampMode(Int mode);
void njTextureFlipMode(Int mode);
void njColorBlendingMode(Int target, Int mode);
void njSuperSampleMode(Int mode);
/* njPolygonCullingMode modes: CULLINGSMALL culls polygons smaller than the
 * njPolygonCullingSize size */
#define NJD_POLYGON_NOCULLING 0
#define NJD_POLYGON_CULLINGSMALL 1
#define NJD_POLYGON_CULLINGACW 2
#define NJD_POLYGON_CULLINGCW 3
void njPolygonCullingMode(Int mode);
void njPolygonCullingSize(Float size);
/* njTextureShadingMode modes */
#define NJD_TEX_SHADING_MODE_MODULATE 0
#define NJD_TEX_SHADING_MODE_DECALALPHA 1
#define NJD_TEX_SHADING_MODE_MODULATEALPHA 2
#define NJD_TEX_SHADING_MODE_DECAL 3

/* njMemCopy copies num bytes, njMemCopy2 num halves and njMemCopy4 num longs
 * from src to dst, one at a time, first to last. num must be at least 1: each
 * copies before it counts, so 0 or less runs on until the count wraps.
 * njMemCopy2's and njMemCopy4's dst and src must be aligned to the element */
void njMemCopy(void *dst, const void *src, Sint32 num);
void njMemCopy2(void *dst, const void *src, Sint32 num);
void njMemCopy4(void *dst, const void *src, Sint32 num);

extern NJS_MATRIX _unit_matrix_;
extern Float _nj_far_clip_;
extern NJS_MATRIX *_nj_current_matrix_ptr_;
/* a copy of the matrix stack's base, which njClearMatrix and njSetCamera
 * write */
extern NJS_MATRIX _nj_view_matrix_;
extern NJS_SCREEN _nj_screen_;
extern Int _nj_frame_mode_;
extern Int _nj_frame_size_w;
extern Int _nj_frame_size_h;
extern Float _nj_near_clip_;
extern Float _nj_scr_aspect_x_;
extern Float _nj_scr_aspect_y_;
extern Uint32 _nj_control_3d_flag_;
extern NJS_VERTEX_BUF *_nj_vertex_buf_;
extern Int _nj_vertex_num_;
extern Int _nj_calc_vertex_;
extern Int _nj_calc_polygon_;
extern Int _nj_draw_polygon_;
extern Uint32 _nj_constant_attr_and_;
extern Uint32 _nj_constant_attr_or_;
extern NJS_ARGB _nj_constant_material_;
extern Uint32 _nj_vsync_count_;
extern Float _nj_depth_near_;
extern Float _nj_depth_far_;
extern Float _nj_depth_range_;
extern Int _nj_depth_enable_;
/* njInitMatrix's flag: while it is set, transformed vectors are made unit
 * length */
extern Int _nj_matrix_flag_;
extern NJS_TEXLIST *_nj_current_texlist;
extern NJS_TEXMEMLIST *_nj_texmemlist;
extern Uint32 _nj_texmemsize;
/* the index after the last texture record in use */
extern Sint32 _nj_tex_count;
extern Uint32 _nj_current_texture;
extern Uint32 _nj_current_texture_bank;
extern char _nj_current_texture_path[256];
extern Uint32 _nj_tex_max_free_size;
extern Uint32 _nj_user_globalIndex;
extern NJS_TEXERRSTAT _nj_texerr_;
/* the surface a texture record takes when its file fails to open */
extern NJS_TEXSURFACE _nj_tex_err_surface;
/* the Kamui vertex context Ninja draws with */
extern struct tagKMVERTEXCONTEXT *_nj_current_ctx_;
extern NJS_LIGHT_MATERIAL _nj_light_material_;
/* the lights njCreateLight has created, and those a draw lights with;
 * njLightSystemInit empties both */
extern struct nj_light_control {
    NJS_LIGHT *light[NJD_LIGHT_MAX];
    Int count;
} _nj_light_control_;
extern struct nj_light_queue {
    Int light[NJD_LIGHT_MAX];
    Int count;
} _nj_light_queue_;
void njLightSystemInit(void);
/* the timer counts at the start and end of njWaitVSync's wait */
extern Uint32 _nj_wait_vsync_in_;
extern Uint32 _nj_wait_vsync_out_;
/* 1 from njWaitVSync's render until the render ends; the next njWaitVSync
 * waits, with a limit, for it to clear */
extern volatile Int _nj_eor_flag_;
/* the display mode njInitSystem took, and the vsync count it or njChangeSystem
 * last took */
extern Int _nj_display_mode_;
extern Int _nj_waitvsync_count;
/* the 2D clip rectangle's corners, in pixels: the screen's, or with
 * NJD_CLIP_INSIDE, njUserClipping's tiles' */
extern NJS_POINT2 _nj_2d_clip_[2];
/* the list sizes njInitVertexBuffer gave Kamui, in words, and their total.
 * the punch-through list's words count in the translucent list's, so
 * _nj_vertex_buffer_pt_ is 0 */
extern Uint32 _nj_vertex_buffer_op_;
extern Uint32 _nj_vertex_buffer_om_;
extern Uint32 _nj_vertex_buffer_tp_;
extern Uint32 _nj_vertex_buffer_tm_;
extern Uint32 _nj_vertex_buffer_pt_;
extern Uint32 _nj_vertex_buffer_size_;
/* njSetTextureMemorySize's size, which the system setup gives Kamui */
extern Uint32 _nj_texture_memory_size;
/* njSetVertexBuffer's buffer, by its uncached address, and its size in bytes */
extern Uint32 *_nj_usr_vertex_buffer_ptr_;
extern Uint32 _nj_usr_vertex_buffer_size_;
/* set by Kamui's callbacks: _nj_render_flag_ to 1 as each render ends,
 * _nj_eov_flag_ to 0 at the end of the vertices, and _nj_fatal_err_callback_
 * to a fatal error's code. njInitSystem and njChangeSystem set
 * _nj_render_flag_ to 0, _nj_eov_flag_ to 1 and _nj_fatal_err_callback_ to 0 */
extern volatile Int _nj_render_flag_;
extern volatile Int _nj_eov_flag_;
extern volatile Int _nj_fatal_err_callback_;
/* njSetEORFunction's function, which njExeEORFunction calls if it is set */
extern void (*_nj_eor_function_)(void);
/* the motion callback, which njInitGlobal clears */
extern void (*_nj_motion_callback_)(NJS_OBJECT *obj);
/* the motion draws' state: where a motion, and a shape motion, stands */
extern struct nj_motion_info _nj_motion_data_info_;
extern struct nj_motion_info _nj_shape_data_info_;
/* the node steps the motion draws run on each node. the motion's factor count
 * selects _nj_motion_data_func_ from _nj_motion_data_func_table_. that step
 * takes the node's record, for a count above 0, and calls the position, angle
 * and scale steps. the shape motion's NJD_MTYPE_VERT_4, as bit 0, and
 * NJD_MTYPE_NORM_5, as bit 1, select _nj_shape_data_func_ from
 * _nj_shape_data_func_table_. with either bit, that step takes the node's
 * record and calls the shape step. with neither, it draws the node's model,
 * if any, through _nj_draw_model_sub_ unless NJD_EVAL_HIDE */
extern void (*_nj_motion_data_func_)(NJS_OBJECT *obj);
extern void (*_nj_shape_data_func_)(NJS_OBJECT *obj);
extern void (*_nj_motion_data_func_table_[5])(NJS_OBJECT *obj);
extern void (*_nj_shape_data_func_table_[4])(NJS_OBJECT *obj);
extern void (*_nj_motion_translate_)(NJS_OBJECT *obj);
extern void (*_nj_motion_rotate_)(NJS_OBJECT *obj);
extern void (*_nj_motion_scale_)(NJS_OBJECT *obj);
/* the shape step. unless the node's model is null or the node is under
 * NJD_EVAL_HIDE, it morphs the model's points, normals or both into the buffer
 * njInitShape takes, draws the model through _nj_draw_model_sub_, and restores
 * them. _nj_shape_int_func_table_ holds the linear step, then the cubic */
extern void (*_nj_motion_shape_)(NJS_OBJECT *obj);
extern void (*_nj_shape_int_func_table_[2])(NJS_OBJECT *obj);
extern void (*_nj_draw_model_sub_)(NJS_MODEL *model);
/* the interpolations. each writes a factor's three values at the motion's
 * frame into out, from key index toward the next key, which after the last is
 * the first; nb is the key count. each table holds the linear interpolation,
 * then the cubic. the old table's linear one takes each angle the short way
 * round */
extern void (*_nj_motion_int_f_func_)(NJS_MKEY_F *keys, Uint32 index, Uint32 nb,
                                      Float *out);
extern void (*_nj_motion_int_f_func_table_[2])(NJS_MKEY_F *keys, Uint32 index,
                                               Uint32 nb, Float *out);
extern void (*_nj_motion_int_a_func_)(NJS_MKEY_A *keys, Uint32 index, Uint32 nb,
                                      Angle *out);
extern void (*_nj_motion_int_a_func_table_[2])(NJS_MKEY_A *keys, Uint32 index,
                                               Uint32 nb, Angle *out);
extern void (*_nj_motion_int_a_func_table_old_[2])(NJS_MKEY_A *keys,
                                                   Uint32 index, Uint32 nb,
                                                   Angle *out);
/* what a shape motion moves, by NJD_MTYPE_VERT_4 and NJD_MTYPE_NORM_5, or 0
 * for neither. a shape motion's count holds it */
enum { SHAPE_POINTS = 1, SHAPE_NORMALS = 2, SHAPE_BOTH = 3 };
/* the motion's factor count, type and interpolation, and the shape motion's.
 * an interpolation is 0 for linear and 1 for any other, and indexes the tables
 * above */
extern Uint16 _nj_mot_data_cnt_;
extern Uint16 _nj_mot_data_type_;
extern Uint16 _nj_mot_spline_cnt_;
extern Uint16 _nj_mot_shape_data_cnt_;
extern Uint16 _nj_mot_shape_data_type_;
extern Uint16 _nj_mot_shape_spline_cnt_;
/* the buffer njInitShape takes: a shape motion morphs a model's points or
 * normals into it, or both, the normals after the points */
extern Float *_nj_shape_buf_;
/* the position, angle and scale steps. an Int step moves the matrix by the
 * keys of the node's next factor, through the interpolations above, or by the
 * node's own pos, ang or scl where that factor has no keys, and moves on to
 * the factor after. an Obj step moves the matrix by the node's own */
void njMotionIntTranslate(NJS_OBJECT *obj);
void njMotionIntScale(NJS_OBJECT *obj);
void njMotionIntRotateXYZ(NJS_OBJECT *obj);
void njMotionIntRotateZXY(NJS_OBJECT *obj);
void njMotionObjTranslate(NJS_OBJECT *obj);
void njMotionObjRotateXYZ(NJS_OBJECT *obj);
void njMotionObjRotateZXY(NJS_OBJECT *obj);
void njMotionObjScale(NJS_OBJECT *obj);
/* where njGetDrawMotion writes the next node's position, angles and scale */
extern Float *_nj_mot_pos_buf_ptr_;
extern Angle *_nj_mot_ang_buf_ptr_;
extern Float *_nj_mot_scl_buf_ptr_;
/* njGetDrawMotion's position, angle and scale steps. an Int step writes the
 * keys of the node's next factor, through the interpolations above, or the
 * node's own pos, ang or scl where that factor has no keys, and moves on to
 * the factor after. an Obj step writes the node's own. each step moves its
 * pointer past the three values it writes */
void njGetMotionIntTranslate(NJS_OBJECT *obj);
void njGetMotionIntScale(NJS_OBJECT *obj);
void njGetMotionIntRotate(NJS_OBJECT *obj);
void njGetMotionObjTranslate(NJS_OBJECT *obj);
void njGetMotionObjRotate(NJS_OBJECT *obj);
void njGetMotionObjScale(NJS_OBJECT *obj);
/* motion links. a link's two motions are its slots, 0 and 1. a walk of the
 * tree runs each node's steps for slot 0, then for slot 1, with
 * _nj_mot_current_data_ the slot. njMakeMotionLink and njMakeShapeLink then
 * blend what the slots' steps leave by _nj_mot_current_frame_, the link's
 * rate */
extern Uint16 _nj_mot_current_data_;
extern Float _nj_mot_current_frame_;
/* where each slot's motion, and its shape motion, stands */
extern struct nj_motlink_data_info _nj_motlink_data_info_[2];
extern struct nj_motlink_data_info _nj_shapelink_data_info_[2];
/* each slot's motion's factor count and type, and its shape motion's count
 * and type. a shape motion's count holds what it moves, a SHAPE_ value */
extern Uint16 _nj_motlink_data_cnt_[2];
extern Uint16 _nj_motlink_data_type_[2];
extern Uint16 _nj_motlink_shape_data_cnt_[2];
extern Uint16 _nj_motlink_shape_data_type_[2];
/* each slot's steps. the slot's factor count selects its data step, which runs
 * the slot's translate, rotate and scale steps on the node. an Int step takes
 * the node's values from the keys of the slot's next factor at the slot's
 * frame, or the node's own pos, ang or scl where that factor has no keys, and
 * moves on to the factor after. an Obj step takes the node's own. the slot's
 * NJD_MTYPE_VERT_4 and NJD_MTYPE_NORM_5 bits select its shape data step, which
 * runs the slot's shape step, or with neither bit takes the model's own points
 * and normals */
extern void (*_nj_motlink_data_func_[2])(NJS_OBJECT *object);
extern void (*_nj_motlink_data_func_table_[5])(NJS_OBJECT *object);
extern void (*_nj_motion_translate_data_[2])(NJS_OBJECT *object);
extern void (*_nj_motion_rotate_data_[2])(NJS_OBJECT *object);
extern void (*_nj_motion_scale_data_[2])(NJS_OBJECT *object);
extern void (*_nj_shapelink_data_func_[2])(NJS_OBJECT *object);
extern void (*_nj_shapelink_data_func_table_[4])(NJS_OBJECT *object);
extern void (*_nj_motion_shape_data_[2])(NJS_OBJECT *object);
extern void (*_nj_shapelink_int_func_table[2])(NJS_OBJECT *object);
void njMotionIntTranslateData(NJS_OBJECT *object);
void njMotionIntRotateData(NJS_OBJECT *object);
void njMotionIntScaleData(NJS_OBJECT *object);
void njMotionObjTranslateData(NJS_OBJECT *object);
void njMotionObjRotateData(NJS_OBJECT *object);
void njMotionObjScaleData(NJS_OBJECT *object);
/* the link's interpolations; each table holds the linear one, then the cubic.
 * each interpolation in the two motlink tables writes a factor's three values
 * at the frame of slot _nj_mot_current_data_ into out, from key index toward
 * the next key, which after the last is the first; nb is the key count.
 * _nj_int_f_func_ and _nj_int_a_func_, chosen from their tables, blend the two
 * slots' values by rate for njMakeMotionLink */
extern void (*_nj_motlink_int_f_func_table_[2])(NJS_MKEY_F *keys, Uint32 index,
                                                Uint32 nb, Float *out);
extern void (*_nj_motlink_int_a_func_table_[2])(NJS_MKEY_A *keys, Uint32 index,
                                                Uint32 nb, Angle *out);
extern void (*_nj_int_f_func_)(Float *values, Float *out, Float rate);
extern void (*_nj_int_a_func_)(Angle *values, Angle *out, Float rate);
extern void (*_nj_int_f_func_table_[2])(Float *values, Float *out, Float rate);
extern void (*_nj_int_a_func_table_[2])(Angle *values, Angle *out, Float rate);
/* njRotateXYZ or njRotateZXY, which njMakeMotionLink rotates with */
extern void (*_nj_rotate_func_)(NJS_MATRIX *m, Angle angx, Angle angy,
                                Angle angz);
/* for the node the walk is on, the slots whose keys set its position, angles,
 * scale, points and normals: 1 for slot 0, 2 for slot 1 */
extern Uint16 _nj_pos_int_flag_;
extern Uint16 _nj_ang_int_flag_;
extern Uint16 _nj_scl_int_flag_;
extern Uint16 _nj_pnt_int_flag_;
extern Uint16 _nj_nml_int_flag_;
/* what each slot's steps leave for the node the walk is on. the first three
 * arrays hold its position, scale and angles: slot 0's three values in [0] to
 * [2] and slot 1's in [3] to [5], then in [6] to [11] the tangents a cubic
 * link blends them by, slot 0's and then slot 1's. the last two point at its
 * points and normals: slot 0's in [0] and slot 1's in [1], then in [2] and [3]
 * the control points a cubic link blends them by */
extern Float _nj_motion_link_pos_[12];
extern Float _nj_motion_link_scl_[12];
extern Angle _nj_motion_link_ang_[12];
extern NJS_POINT3 *_nj_shape_link_point_[4];
extern NJS_VECTOR *_nj_shape_link_normal_[4];
/* transform the current matrix by the node's position, angles and scale, from
 * what the two slots' steps left */
void njMakeMotionLink(void);
/* draw object's model through _nj_draw_model_sub_, then restore its points
 * and normals. a null or hidden model is not drawn. where either slot's keys
 * gave points and either slot's gave normals, the two slots' are first
 * blended into _nj_shape_buf_. the model is then drawn with the blended
 * normals as its points, and the nbPoint vectors after them, past the blends,
 * as its normals */
void njMakeShapeLink(NJS_OBJECT *object);
/* the easy chunk lights' source block. _nj_cnk_easy_multilight_buffer_dst_'s
 * 32 words follow it in memory, and njInitGlobal writes both through this
 * name */
extern Float _nj_cnk_easy_multilight_buffer_[16];
/* the surfaces of the two frame buffers the system setup names */
extern NJS_TEXSURFACE _nj_framebuffer_surface0_;
extern NJS_TEXSURFACE _nj_framebuffer_surface1_;
void njInitSprite(void);

#endif
