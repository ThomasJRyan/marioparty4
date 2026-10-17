#include "game/hu3d.h"
#include "game/object.h"

#include "game/window.h"

#include "ext_math.h"

void ShadowLayerDraw(s16 cameraIndex);

void ShadowLayerHookInstall(void)
{
    Hu3DLayerHookSet(7, ShadowLayerDraw);
}

void ShadowLayerDraw(s16 cameraIndex)
{
    Mtx44 projectionMatrix;
    Mtx viewMatrix;
    Vec quadHalfSize;
    Vec cameraTarget;
    Vec cameraUp;
    GXColor shadowColor;
    HU3DCAMERA *camera;

    camera = &Hu3DCamera[0];
    C_MTXPerspective(projectionMatrix, camera->fov, camera->aspect, camera->near, camera->far);
    GXSetProjection(projectionMatrix, GX_PERSPECTIVE);
    GXSetViewport(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    GXSetScissor(0, 0, 0x280, 0x1E0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    shadowColor.a = 0xFF;
    GXSetTevColor(GX_TEVREG0, shadowColor);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetCullMode(GX_CULL_NONE);
    GXSetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_GEQUAL, 0);
    GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
    quadHalfSize.x = quadHalfSize.y = quadHalfSize.z = 0.0f;
    cameraTarget.x = cameraTarget.y = 0.0f;
    cameraTarget.z = -100.0f;
    cameraUp.x = cameraUp.z = 0.0f;
    cameraUp.y = 1.0f;
    C_MTXLookAt(viewMatrix, &quadHalfSize, &cameraUp, &cameraTarget);
    GXLoadPosMtxImm(viewMatrix, 0);
    quadHalfSize.x = 1.2f * (10000.0 * -sin(camera->fov / 2));
    quadHalfSize.y = (2.5f / 3) * (10000.0 * -sin(camera->fov / 2));
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-quadHalfSize.x, -quadHalfSize.y, -10000.0f);
    GXPosition3f32(quadHalfSize.x, -quadHalfSize.y, -10000.0f);
    GXPosition3f32(quadHalfSize.x, quadHalfSize.y, -10000.0f);
    GXPosition3f32(-quadHalfSize.x, quadHalfSize.y, -10000.0f);
}
