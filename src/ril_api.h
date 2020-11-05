/*	see copyright notice in ril.h */

#ifndef _RIL_API_H_
#define _RIL_API_H_

#ifdef __cplusplus
extern "C" {
#endif

RILRESULT ril_setargumentsbycmd(RILVM vm, ril_vmcmd_t *cmd);
void ril_setnextcmd(RILVM vm, ril_vmcmd_t *cmd);
void ril_cleartags(RILVM vm);
void ril_deletetags(RILVM vm);
void ril_deletemacros(RILVM vm);
ril_tag_t* ril_createtag(RILVM vm, ril_signature_t signature);
void ril_inittag(ril_tag_t *tag);
void ril_deletetag(ril_tag_t *tag);
void ril_addstack(RILVM vm, ril_tag_t *tag);

#ifdef __cplusplus
}
#endif

#endif
