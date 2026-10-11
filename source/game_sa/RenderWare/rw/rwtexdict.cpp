#include "StdInc.h"

#include "rwtexdict.h"

#ifdef NOTSA_STANDALONE
#include "RwBridge.h" // librw-backed RenderWare (standalone)
#endif

RwTexture* GetFirstTexture(RwTexDictionary* txd) {
    return plugin::CallAndReturn<RwTexture*, 0x734940, RwTexDictionary*>(txd);
}

RwTexture* GetFirstTextureCallback(RwTexture* texture, void* data) {
    return plugin::CallAndReturn<RwTexture*, 0x734930, RwTexture*, void*>(texture, data);
}

RwTexture* RwTextureGtaStreamRead(RwStream* stream) {
    return plugin::CallAndReturn<RwTexture*, 0x730E60, RwStream*>(stream);
}

RwTexDictionary* RwTexDictionaryGtaStreamRead(RwStream* stream) {
#ifdef NOTSA_STANDALONE
    return (RwTexDictionary*)rwbridge::TexDictionaryStreamRead(stream);
#else
    return plugin::CallAndReturn<RwTexDictionary*, 0x730FC0, RwStream*>(stream);
#endif
}

RwTexDictionary* RwTexDictionaryGtaStreamRead1(RwStream* stream) {
    return plugin::CallAndReturn<RwTexDictionary*, 0x731070, RwStream*>(stream);
}

RwTexDictionary* RwTexDictionaryGtaStreamRead2(RwStream* stream, RwTexDictionary* txd) {
    return plugin::CallAndReturn<RwTexDictionary*, 0x731150, RwStream*, RwTexDictionary*>(stream, txd);
}

bool RpClumpGtaStreamRead1(RwStream* stream) {
    return plugin::CallAndReturn<bool, 0x72E570, RwStream*>(stream);
}

RpClump* RpClumpGtaStreamRead2(RwStream* stream) {
    return plugin::CallAndReturn<RpClump*, 0x72E620, RwStream*>(stream);
}
