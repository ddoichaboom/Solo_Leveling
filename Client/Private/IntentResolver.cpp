#include "IntentResolver.h"

void CIntentResolver::Resolve(const PLAYER_RAW_INPUT_FRAME& Raw, PLAYER_INTENT_FRAME* pOutIntent)
{
    if (nullptr == pOutIntent)
        return;

    pOutIntent->vMoveAxis.x = Compute_Axis(Raw.bMoveLeftHeld, Raw.bMoveRightHeld);
    pOutIntent->vMoveAxis.y = Compute_Axis(Raw.bMoveBackwardHeld, Raw.bMoveForwardHeld);

    pOutIntent->lLookDeltaX = Raw.lMouseDeltaX;
    pOutIntent->bHasMoveIntent = ((0.f != pOutIntent->vMoveAxis.x) || (0.f != pOutIntent->vMoveAxis.y));
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