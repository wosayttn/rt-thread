/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/

#include "utest.h"

#if (defined(BSP_USING_CRYPTO) && defined(RT_USING_HWCRYPTO))

#include "rtdevice.h"

#if defined(RT_HWCRYPTO_USING_CRC)
    #include "hw_crc.h"
#endif

/*----------------------------------------------------------------------------*/
/* Helper functions */
/*----------------------------------------------------------------------------*/

static int unhexify(unsigned char *obuf, const char *ibuf)
{
    unsigned char c, c2;
    int len = rt_strlen(ibuf) / 2;

    while (*ibuf != 0)
    {
        c = *ibuf++;
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'a' && c <= 'f')
            c -= 'a' - 10;
        else if (c >= 'A' && c <= 'F')
            c -= 'A' - 10;
        else
            return -1;

        c2 = *ibuf++;
        if (c2 >= '0' && c2 <= '9')
            c2 -= '0';
        else if (c2 >= 'a' && c2 <= 'f')
            c2 -= 'a' - 10;
        else if (c2 >= 'A' && c2 <= 'F')
            c2 -= 'A' - 10;
        else
            return -1;

        *obuf++ = (c << 4) | c2;
    }

    return len;
}

static void hexify(unsigned char *obuf, const unsigned char *ibuf, int len)
{
    unsigned char l, h;

    while (len != 0)
    {
        h = *ibuf / 16;
        l = *ibuf % 16;

        if (h < 10)
            *obuf++ = '0' + h;
        else
            *obuf++ = 'a' + h - 10;

        if (l < 10)
            *obuf++ = '0' + l;
        else
            *obuf++ = 'a' + l - 10;

        ++ibuf;
        len--;
    }
    *obuf = '\0';
}

/*----------------------------------------------------------------------------*/
/* AES test using RT-Thread hwcrypto API */
/*----------------------------------------------------------------------------*/
#if defined(RT_HWCRYPTO_USING_AES)

static void test_crypto_aes_ecb(void)
{
    /* AES-128 ECB test vector (NIST FIPS 197) */
    const char *hex_key = "2b7e151628aed2a6abf7158809cf4f3c";
    const char *hex_plaintext = "6bc1bee22e409f96e93d7e117393172a";
    const char *hex_expected  = "3ad77bb40d7a3660a89ecaf32466ef97";

    unsigned char key[16], plaintext[16], expected[16];
    unsigned char output[16], decrypted[16];
    char hex_output[33];
    struct rt_hwcrypto_ctx *ctx;

    unhexify(key, hex_key);
    unhexify(plaintext, hex_plaintext);
    unhexify(expected, hex_expected);

    /* Encrypt */
    ctx = rt_hwcrypto_symmetric_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_AES_ECB);
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    uassert_int_equal(rt_hwcrypto_symmetric_setkey(ctx, key, 128), RT_EOK);
    uassert_int_equal(rt_hwcrypto_symmetric_crypt(ctx, HWCRYPTO_MODE_ENCRYPT, 16, plaintext, output), RT_EOK);

    hexify((unsigned char *)hex_output, output, 16);
    uassert_str_equal(hex_output, hex_expected);

    rt_hwcrypto_symmetric_destroy(ctx);

    /* Decrypt */
    ctx = rt_hwcrypto_symmetric_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_AES_ECB);
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    uassert_int_equal(rt_hwcrypto_symmetric_setkey(ctx, key, 128), RT_EOK);
    uassert_int_equal(rt_hwcrypto_symmetric_crypt(ctx, HWCRYPTO_MODE_DECRYPT, 16, output, decrypted), RT_EOK);
    uassert_buf_equal(decrypted, plaintext, 16);

    rt_hwcrypto_symmetric_destroy(ctx);
}

static void test_crypto_aes_cbc(void)
{
    /* AES-128 CBC test vector (NIST SP 800-38A) */
    const char *hex_key = "2b7e151628aed2a6abf7158809cf4f3c";
    const char *hex_iv  = "000102030405060708090a0b0c0d0e0f";
    const char *hex_plaintext = "6bc1bee22e409f96e93d7e117393172a";
    const char *hex_expected  = "7649abac8119b246cee98e9b12e9197d";

    unsigned char key[16], iv[16], plaintext[16], expected[16];
    unsigned char output[16], decrypted[16], iv2[16];
    char hex_output[33];
    struct rt_hwcrypto_ctx *ctx;

    unhexify(key, hex_key);
    unhexify(iv, hex_iv);
    unhexify(plaintext, hex_plaintext);
    unhexify(expected, hex_expected);

    /* Encrypt */
    ctx = rt_hwcrypto_symmetric_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_AES_CBC);
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    uassert_int_equal(rt_hwcrypto_symmetric_setkey(ctx, key, 128), RT_EOK);
    uassert_int_equal(rt_hwcrypto_symmetric_setiv(ctx, iv, 16), RT_EOK);
    uassert_int_equal(rt_hwcrypto_symmetric_crypt(ctx, HWCRYPTO_MODE_ENCRYPT, 16, plaintext, output), RT_EOK);

    hexify((unsigned char *)hex_output, output, 16);
    uassert_str_equal(hex_output, hex_expected);

    rt_hwcrypto_symmetric_destroy(ctx);

    /* Decrypt */
    unhexify(iv2, hex_iv);

    ctx = rt_hwcrypto_symmetric_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_AES_CBC);
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    uassert_int_equal(rt_hwcrypto_symmetric_setkey(ctx, key, 128), RT_EOK);
    uassert_int_equal(rt_hwcrypto_symmetric_setiv(ctx, iv2, 16), RT_EOK);
    uassert_int_equal(rt_hwcrypto_symmetric_crypt(ctx, HWCRYPTO_MODE_DECRYPT, 16, output, decrypted), RT_EOK);
    uassert_buf_equal(decrypted, plaintext, 16);

    rt_hwcrypto_symmetric_destroy(ctx);
}

static void test_crypto_aes_cfb(void)
{
    /* AES-128 CFB128 test vector (NIST SP 800-38A) */
    const char *hex_key = "2b7e151628aed2a6abf7158809cf4f3c";
    const char *hex_iv  = "000102030405060708090a0b0c0d0e0f";
    const char *hex_plaintext = "6bc1bee22e409f96e93d7e117393172a";
    const char *hex_expected  = "3b3fd92eb72dad20333449f8e83cfb4a";

    unsigned char key[16], iv[16], plaintext[16], expected[16];
    unsigned char output[16], decrypted[16], iv2[16];
    char hex_output[33];
    struct rt_hwcrypto_ctx *ctx;

    unhexify(key, hex_key);
    unhexify(iv, hex_iv);
    unhexify(plaintext, hex_plaintext);
    unhexify(expected, hex_expected);

    /* Encrypt */
    ctx = rt_hwcrypto_symmetric_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_AES_CFB);
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    uassert_int_equal(rt_hwcrypto_symmetric_setkey(ctx, key, 128), RT_EOK);
    uassert_int_equal(rt_hwcrypto_symmetric_setiv(ctx, iv, 16), RT_EOK);
    uassert_int_equal(rt_hwcrypto_symmetric_crypt(ctx, HWCRYPTO_MODE_ENCRYPT, 16, plaintext, output), RT_EOK);

    hexify((unsigned char *)hex_output, output, 16);
    uassert_str_equal(hex_output, hex_expected);

    rt_hwcrypto_symmetric_destroy(ctx);

    /* Decrypt */
    unhexify(iv2, hex_iv);

    ctx = rt_hwcrypto_symmetric_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_AES_CFB);
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    uassert_int_equal(rt_hwcrypto_symmetric_setkey(ctx, key, 128), RT_EOK);
    uassert_int_equal(rt_hwcrypto_symmetric_setiv(ctx, iv2, 16), RT_EOK);
    uassert_int_equal(rt_hwcrypto_symmetric_crypt(ctx, HWCRYPTO_MODE_DECRYPT, 16, output, decrypted), RT_EOK);
    uassert_buf_equal(decrypted, plaintext, 16);

    rt_hwcrypto_symmetric_destroy(ctx);
}

static void test_crypto_aes(void)
{
    test_crypto_aes_ecb();
    test_crypto_aes_cbc();
    test_crypto_aes_cfb();
    rt_kprintf("AES ECB/CBC/CFB test done.\n");
}

#endif /* RT_HWCRYPTO_USING_AES */

/*----------------------------------------------------------------------------*/
/* SHA test using RT-Thread hwcrypto API */
/*----------------------------------------------------------------------------*/
#if defined(RT_HWCRYPTO_USING_SHA1) || defined(RT_HWCRYPTO_USING_SHA2)

static void test_crypto_sha1(void)
{
#if defined(RT_HWCRYPTO_USING_SHA1)
    /* SHA-1 test vector: "abc" */
    const unsigned char input[] = "abc";
    const char *hex_expected = "a9993e364706816aba3e25717850c26c9cd0d89d";

    unsigned char output[20];
    char hex_output[41];
    struct rt_hwcrypto_ctx *ctx;

    ctx = rt_hwcrypto_hash_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_SHA1);
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    uassert_int_equal(rt_hwcrypto_hash_update(ctx, input, 3), RT_EOK);
    uassert_int_equal(rt_hwcrypto_hash_finish(ctx, output, 20), RT_EOK);

    hexify((unsigned char *)hex_output, output, 20);
    uassert_str_equal(hex_output, hex_expected);

    rt_hwcrypto_hash_destroy(ctx);
    rt_kprintf("SHA1 test done.\n");
#endif
}

static void test_crypto_sha256(void)
{
#if defined(RT_HWCRYPTO_USING_SHA2)
    /* SHA-256 test vector: "abc" */
    const unsigned char input[] = "abc";
    const char *hex_expected = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

    unsigned char output[32];
    char hex_output[65];
    struct rt_hwcrypto_ctx *ctx;

    ctx = rt_hwcrypto_hash_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_SHA256);
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    uassert_int_equal(rt_hwcrypto_hash_update(ctx, input, 3), RT_EOK);
    uassert_int_equal(rt_hwcrypto_hash_finish(ctx, output, 32), RT_EOK);

    hexify((unsigned char *)hex_output, output, 32);
    uassert_str_equal(hex_output, hex_expected);

    rt_hwcrypto_hash_destroy(ctx);
    rt_kprintf("SHA256 test done.\n");
#endif
}

static void test_crypto_sha(void)
{
    test_crypto_sha1();
    test_crypto_sha256();
}

#endif /* RT_HWCRYPTO_USING_SHA1 || RT_HWCRYPTO_USING_SHA2 */

/*----------------------------------------------------------------------------*/
/* CRC test using RT-Thread hwcrypto API */
/*----------------------------------------------------------------------------*/
#if defined(RT_HWCRYPTO_USING_CRC)

static void test_suite_crc8(void)
{
    const uint8_t au8CRCSrcPattern[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    uint32_t u32TargetChecksum = 0x58, u32CalChecksum = 0;

    struct rt_hwcrypto_ctx *ctx;
    struct hwcrypto_crc_cfg cfg =
    {
        .last_val = 0x5A,
        .poly = 0x00000007,
        .width = 8,
        .xorout = 0x00000000,
        .flags = 0,
    };

    ctx = rt_hwcrypto_crc_create(rt_hwcrypto_dev_default(), HWCRYPTO_CRC_CRC8);
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    rt_hwcrypto_crc_cfg(ctx, &cfg);
    u32CalChecksum = rt_hwcrypto_crc_update(ctx, au8CRCSrcPattern, sizeof(au8CRCSrcPattern));
    uassert_int_equal(u32CalChecksum, u32TargetChecksum);
    rt_kprintf("CRC8 checksum: 0x%X\n", u32CalChecksum);

    rt_hwcrypto_crc_destroy(ctx);
}

static void test_suite_crc16(void)
{
    const uint8_t au8CRCSrcPattern[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38};
    uint32_t u32TargetChecksum = 0x972D, u32CalChecksum = 0;

    struct rt_hwcrypto_ctx *ctx;
    struct hwcrypto_crc_cfg cfg =
    {
        .last_val = 0xFFFF,
        .poly = 0x8005,
        .width = 16,
        .xorout = 0x00000000,
        .flags = 0,
    };

    ctx = rt_hwcrypto_crc_create(rt_hwcrypto_dev_default(), HWCRYPTO_CRC_CRC16);
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    rt_hwcrypto_crc_cfg(ctx, &cfg);
    u32CalChecksum = rt_hwcrypto_crc_update(ctx, au8CRCSrcPattern, sizeof(au8CRCSrcPattern));
    uassert_int_equal(u32CalChecksum, u32TargetChecksum);
    rt_kprintf("CRC16 checksum: 0x%X\n", u32CalChecksum);

    rt_hwcrypto_crc_destroy(ctx);
}

static void test_suite_crc32(void)
{
    const uint8_t au8CRCSrcPattern[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    uint32_t u32TargetChecksum = 0x3CF1A989, u32CalChecksum = 0;

    struct rt_hwcrypto_ctx *ctx;
    struct hwcrypto_crc_cfg cfg =
    {
        .last_val = 0xFFFF0000,
        .poly = 0x04C11DB7,
        .width = 32,
        .xorout = 0x00000000,
        .flags = 0,
    };

    ctx = rt_hwcrypto_crc_create(rt_hwcrypto_dev_default(), HWCRYPTO_CRC_CRC32);
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    rt_hwcrypto_crc_cfg(ctx, &cfg);
    u32CalChecksum = rt_hwcrypto_crc_update(ctx, au8CRCSrcPattern, sizeof(au8CRCSrcPattern));
    uassert_int_equal(u32CalChecksum, u32TargetChecksum);
    rt_kprintf("CRC32 checksum: 0x%X\n", u32CalChecksum);

    rt_hwcrypto_crc_destroy(ctx);
}

static void test_suite_ccitt(void)
{
    const uint8_t au8CRCSrcPattern[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38};
    uint32_t u32TargetChecksum = 0xA12B, u32CalChecksum = 0;

    struct rt_hwcrypto_ctx *ctx;
    struct hwcrypto_crc_cfg cfg =
    {
        .last_val = 0xFFFF,
        .poly = 0x1021,
        .width = 16,
        .xorout = 0x00000000,
        .flags = 0,
    };

    ctx = rt_hwcrypto_crc_create(rt_hwcrypto_dev_default(), HWCRYPTO_CRC_CRC32);
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    rt_hwcrypto_crc_cfg(ctx, &cfg);
    /* input address not alignment test */
    u32CalChecksum = rt_hwcrypto_crc_update(ctx, au8CRCSrcPattern + 1, sizeof(au8CRCSrcPattern) - 1);
    uassert_int_equal(u32CalChecksum, u32TargetChecksum);
    rt_kprintf("CRC CCITT checksum: 0x%X\n", u32CalChecksum);

    rt_hwcrypto_crc_destroy(ctx);
}

static void test_crypto_crc(void)
{
    test_suite_crc8();
    test_suite_crc16();
    test_suite_crc32();
    test_suite_ccitt();
    rt_kprintf("All CRC tests done.\n");
}

#endif /* RT_HWCRYPTO_USING_CRC */

/*----------------------------------------------------------------------------*/
/* RNG test using RT-Thread hwcrypto API */
/*----------------------------------------------------------------------------*/
#if defined(RT_HWCRYPTO_USING_RNG)

#define GENERATE_COUNT 10

static void test_crypto_trng(void)
{
    struct rt_hwcrypto_ctx *ctx;
    int i;
    rt_uint32_t result, prev_result;

    ctx = rt_hwcrypto_rng_create(rt_hwcrypto_dev_default());
    uassert_not_null(ctx);
    if (ctx == RT_NULL) return;

    prev_result = 0;
    for (i = 0; i < GENERATE_COUNT; i++)
    {
        result = rt_hwcrypto_rng_update_ctx(ctx);
        rt_kprintf("RNG value[%d]: 0x%08x\n", i, result);
        /* Verify randomness: should not always be 0 or same value */
        if (i > 0)
        {
            uassert_int_not_equal(result, prev_result);
        }
        prev_result = result;
    }
    uassert_int_not_equal(result, 0);

    rt_hwcrypto_rng_destroy(ctx);
    rt_kprintf("RNG test done.\n");
}

#endif /* RT_HWCRYPTO_USING_RNG */

/*----------------------------------------------------------------------------*/
/* Utest framework */
/*----------------------------------------------------------------------------*/

static rt_err_t utest_tc_init(void)
{
    rt_kprintf("\n[CRYPTO utest] Verify AES, SHA, CRC and RNG hardware crypto.\n");
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
#if defined(RT_HWCRYPTO_USING_AES)
    UTEST_UNIT_RUN(test_crypto_aes);
#endif

#if defined(RT_HWCRYPTO_USING_SHA1) || defined(RT_HWCRYPTO_USING_SHA2)
    UTEST_UNIT_RUN(test_crypto_sha);
#endif

#if defined(RT_HWCRYPTO_USING_CRC)
    UTEST_UNIT_RUN(test_crypto_crc);
#endif

#if defined(RT_HWCRYPTO_USING_RNG)
    UTEST_UNIT_RUN(test_crypto_trng);
#endif
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"crypto",
                utest_tc_init, utest_tc_cleanup, 10);

#endif /* (defined(BSP_USING_CRYPTO) && defined(RT_USING_HWCRYPTO)) */
