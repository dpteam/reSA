#include "StdInc.h"

#include "rtpng.h"

#ifdef NOTSA_STANDALONE
#include "RwBridge.h" // librw-backed RenderWare (standalone build)
#endif

RwImage *RtPNGImageWrite(RwImage *image, const RwChar *imageName) {
    return ((RwImage *(__cdecl *)(RwImage *, const RwChar *)) 0x7CF600)(image, imageName);
}

RwImage *RtPNGImageRead(const RwChar *imageName) {
#ifdef NOTSA_STANDALONE
    return (RwImage*)rwbridge::ImageReadPNG(imageName);
#else
    return ((RwImage *(__cdecl *)(const RwChar *)) 0x7CF9B0)(imageName);
#endif
}
