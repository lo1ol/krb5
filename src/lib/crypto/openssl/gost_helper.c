#include "gost_helper.h"
#include <openssl/engine.h>

static ENGINE *eng = NULL;

void
krb5int_init_gost()
{
    if (eng) return;
    OPENSSL_add_all_algorithms_conf();
    ERR_load_crypto_strings();

    if (!(eng = ENGINE_by_id("gost"))) {
        printf("Engine gost doesnt exist");
        return;
    }

    ENGINE_init(eng);
    ENGINE_set_default(eng, ENGINE_METHOD_ALL);
}

void
krb5int_free_gost()
{
    // We don't free gost engine, because it has no propper interface
    // ENGINE_finish(eng);
    // ENGINE_free(eng);
}

const EVP_MD *
EVP_gostR3411_2012_256()
{
    krb5int_init_gost();
    return EVP_get_digestbynid(NID_id_GostR3411_2012_256);
}

const EVP_MD *
EVP_gostR3411_2012_512()
{
    krb5int_init_gost();
    return EVP_get_digestbynid(NID_id_GostR3411_2012_512);
}
