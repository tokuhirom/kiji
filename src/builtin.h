#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void Kiji_bootstrap_Array(MVMCompUnit* cu, MVMThreadContext*tc);
void Kiji_bootstrap_Str(MVMCompUnit* cu, MVMThreadContext*tc);
void Kiji_bootstrap_Hash(MVMCompUnit* cu, MVMThreadContext*tc);
void Kiji_bootstrap_File(MVMCompUnit* cu, MVMThreadContext*tc);
void Kiji_bootstrap_Int(MVMCompUnit* cu, MVMThreadContext*tc);
MVMObject * Kiji_bootstrap_Pair(MVMCompUnit* cu, MVMThreadContext*tc);

#ifdef __cplusplus
}
#endif
