#include <stdio.h>
#include "aes128n8silcv1_silc.h"
#include <stdlib.h>

// CHANGE namespace moved due to includes
namespace Aes128n8silcv1_raw {

ae_cxt* ae_allocate(){
    ae_cxt* pt = (ae_cxt*) calloc(sizeof(ae_cxt), 1);
    return pt;
}

void xor_bytes(byte*x, const byte*y, int nb)
{
    xor_bytes2(x, x, y, nb);
}

void xor_bytes2(byte* result, const byte* x, const byte* y, int nb)
{
    int i = 0;
    for(i = 0; i < nb; i++)
        result[i] = x[i] ^ y[i];
}


/* print state with a prefix string */
// CHANGE unsigned quantifier dropped (strings are incompatible with 'unsigned')
//void pstate2(const unsigned char *str, const byte* state)
void pstate2(const char *str, const byte* state)
{
#ifdef _PRINT
    printf("%s\n", str);
    if(state)
    pstate(state, STATE_LEN);
#endif
}
void pstate(const unsigned char* st, int len)
{
#ifdef _PRINT
    int i;
    for(i = 0; i < len; i++){
        printf("%02x", st[i]);
        if(((i%16) == (16-1)) && (i+1 !=len)) printf("\n");
    }
    printf("\n");
#endif
}


void print_cxt(const ae_cxt* cxt)
{
    printf("user key of %d bytes:\n", cxt->klen);
    pstate(cxt->userkey, cxt->klen);
    printf("associated data of %llu bytes:\n", cxt->adlen);
    pstate(cxt->ad, cxt->adlen);
    printf("nonce of %llu bytes:\n", cxt->nlen);
    pstate(cxt->nonce, cxt->nlen);
    printf("plaintext of %llu bytes:\n", cxt->ptlen);
    pstate(cxt->pt, cxt->ptlen);
    printf("ciphertext of %llu bytes:\n", cxt->ctlen);
    pstate(cxt->ct, cxt->ctlen);
    printf("MAC of %d bytes:\n", cxt->tlen);
    pstate(cxt->tag, cxt->tlen);
}

} // namespace Aes128n8silcv1_raw