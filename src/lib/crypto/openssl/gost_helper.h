#include <openssl/evp.h>

void krb5int_init_gost();
void krb5int_free_gost();
const EVP_MD * EVP_gostR3411_2012_256();
const EVP_MD * EVP_gostR3411_2012_512();
