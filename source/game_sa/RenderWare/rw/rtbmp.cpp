#include "StdInc.h"

#include "rtbmp.h"

#ifdef NOTSA_STANDALONE
#include "RwBridge.h" // librw-backed RenderWare (standalone build)
#endif

RwImage* RtBMPImageWrite(RwImage* image, const RwChar* imageName) {
    return ((RwImage*(__cdecl *)(RwImage*, const RwChar*))0x7CE990)(image, imageName);
}

RwImage* RtBMPImageRead(const RwChar* imageName) {
#ifdef NOTSA_STANDALONE
    return (RwImage*)rwbridge::ImageReadBMP(imageName);
#else
    return ((RwImage*(__cdecl *)(const RwChar*))0x7CDF60)(imageName);
#endif
}
