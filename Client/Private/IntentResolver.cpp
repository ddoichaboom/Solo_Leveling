#include "IntentResolver.h"

void CIntentResolver::Resolve(const PLAYER_RAW_INPUT_FRAME& Raw, _float fCameraYaw, PLAYER_INTENT_FRAME* pOutIntent)
{
    if (nullptr == pOutIntent)
        return;

    pOutIntent->vMoveAxis.x = Compute_Axis(Raw.bMoveLeftHeld, Raw.bMoveRightHeld);
    pOutIntent->vMoveAxis.y = Compute_Axis(Raw.bMoveBackwardHeld, Raw.bMoveForwardHeld);

    pOutIntent->lLookDeltaX = Raw.lMouseDeltaX;
    pOutIntent->bHasMoveIntent = ((0.f != pOutIntent->vMoveAxis.x) || (0.f != pOutIntent->vMoveAxis.y));

    pOutIntent->bDashRequested = Raw.bDashPressed;

    pOutIntent->vMoveDirWorld = { 0.f, 0.f, 0.f };

    if (true == pOutIntent->bHasMoveIntent)
    {
        const _float fSin = sinf(fCameraYaw);
        const _float fCos = cosf(fCameraYaw);

        const _float ax = pOutIntent->vMoveAxis.x;
        const _float az = pOutIntent->vMoveAxis.y;

        // Forward(XZ) = (sin, cos), Right(XZ) = (cos, -sin)
        _float vx = az * fSin + ax * fCos;
        _float vz = az * fCos - ax * fSin;

        const _float fLenSq = vx * vx + vz * vz;
        if (fLenSq > 0.f)
        {
            const _float fInvLen = 1.f / sqrtf(fLenSq);
            vx *= fInvLen;
            vz *= fInvLen;
        }

        pOutIntent->vMoveDirWorld = { vx, 0.f, vz };
    }
}

_float CIntentResolver::Compute_Axis(_bool bNegativeHeld, _bool bPositiveHeld) const
{
    if (bNegativeHeld == bPositiveHeld)
        return 0.f;

    return bPositiveHeld ? 1.f : -1.f;
}

CIntentResolver* CIntentResolver::Create()
{
    CIntentResolver* pInstance = new CIntentResolver();

    if (nullptr == pInstance)
        return nullptr;

    return pInstance;
}

void CIntentResolver::Free()
{
    __super::Free();
}