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

#define KEY_VALUE_MAPPING_FOUND     0
#define KEY_VALUE_MAPPING_NOT_FOUND (-1)

#define DISPATCH_TEST_SUCCESS       0
#define DISPATCH_TEST_FN_NOT_FOUND  1
#define DISPATCH_INVALID_TEST_DATA  2
#define DISPATCH_UNSUPPORTED_SUITE  3

#if defined(PKG_USING_MBEDTLS)
    #define NU_UNIT_TEST_ENABLE_AES
    #define NU_UNIT_TEST_ENABLE_SHA
#endif

#if defined (NU_UNIT_TEST_ENABLE_AES)

    #include "mbedtls/aes.h"

    static const char szVectorBase_AES_ECB[] =
    #include "test_suite_aes.ecb.data"
    ;

    static const char szVectorBase_AES_CBC[] =
    #include "test_suite_aes.cbc.data"
    ;

    static const char szVectorBase_AES_CFB[] =
    #include "test_suite_aes.cfb.data"
    ;

    static int32_t i32VectorBase_AES_ECB_size = sizeof(szVectorBase_AES_ECB);
    static int32_t i32VectorBase_AES_CBC_size = sizeof(szVectorBase_AES_CBC);
    static int32_t i32VectorBase_AES_CFB_size = sizeof(szVectorBase_AES_CFB);

#endif

#if defined (NU_UNIT_TEST_ENABLE_AES)
    static unsigned char s_key_str[100];
    static unsigned char s_iv_str[100];
    static unsigned char s_src_str[100];
    static unsigned char s_dst_str[100];
#endif

#if defined (NU_UNIT_TEST_ENABLE_SHA)

    #include "mbedtls/sha1.h"
    #include "mbedtls/sha256.h"
    #include "mbedtls/sha512.h"

    static const char szVectorBase_SHA[] =
    #include "test_suite_shax.data"
    ;

    static int32_t i32VectorBase_SHA_size = sizeof(szVectorBase_SHA);
    static unsigned char s_hash_str[129];

    #define SHA_MAX_DATASIZE  2560
#endif

#if defined (NU_UNIT_TEST_ENABLE_AES) || defined (NU_UNIT_TEST_ENABLE_SHA)
    static char  g_line_buff[2560];
    static uint32_t  s_file_idx, s_file_size;
    static uint8_t   *s_file_base_ptr;
    static int       pass_cnt;
    static unsigned char s_output_buf[129];
#endif

/*----------------------------------------------------------------------------*/
/* Macros */

#define TEST_ASSERT( TEST )                         \
    do                                              \
    {                                               \
        if( ! (TEST) )                              \
        {                                           \
            test_fail( #TEST, __LINE__, __FILE__ ); \
            goto exit;                              \
        }                                           \
    } while( 0 )

#define crypto_assert(a) if( !( a ) )                             \
do{                                                        \
    rt_kprintf("Assertion Failed at %s:%d - %s\n",         \
                             __FILE__, __LINE__, #a );     \
    while( 1 );                                            \
} while( 0 )

#if defined (NU_UNIT_TEST_ENABLE_AES) || defined (NU_UNIT_TEST_ENABLE_SHA)

static int  read_file(uint8_t *pu8Buff, int i32Len)
{
    if (s_file_idx + 1 >= s_file_size)
        return -1;
    memcpy(pu8Buff, &s_file_base_ptr[s_file_idx], i32Len);
    s_file_idx += i32Len;
    return 0;
}

static int  get_line(void)
{
    int         i;
    uint8_t     ch[2];

    if (s_file_idx + 1 >= s_file_size)
    {
        return -1;
    }

    memset(g_line_buff, 0, sizeof(g_line_buff));

    for (i = 0; i < sizeof(g_line_buff);)
    {
        if (read_file(ch, 1) < 0)
            return 0;

        if ((ch[0] == 0x0D) || (ch[0] == 0x0A))
            break;

        g_line_buff[i++] = ch[0];
    }

    while (1)
    {
        if (read_file(ch, 1) < 0)
            return 0;

        if ((ch[0] != 0x0D) && (ch[0] != 0x0A))
            break;
    }
    s_file_idx--;
    return 0;
}

static int unhexify(unsigned char *obuf, const char *ibuf)
{
    unsigned char c, c2;
    int len = strlen(ibuf) / 2;
    crypto_assert(strlen(ibuf) % 2 == 0);     /* must be even number of bytes */

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
            crypto_assert(0);

        c2 = *ibuf++;
        if (c2 >= '0' && c2 <= '9')
            c2 -= '0';
        else if (c2 >= 'a' && c2 <= 'f')
            c2 -= 'a' - 10;
        else if (c2 >= 'A' && c2 <= 'F')
            c2 -= 'A' - 10;
        else
            crypto_assert(0);

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
}

static void test_fail(const char *test, int line_no, const char *filename)
{
    rt_kprintf("FAILED\n");
    rt_kprintf("  %s\n  at line %d, %s\n", test, line_no, filename);
}

#endif

int parse_arguments(char *buf, size_t len, char *params[50])
{
    int cnt = 0, i;
    char *cur = buf;
    char *p = buf;

    params[cnt++] = cur;

    while (*p != '\0' && p < buf + len)
    {
        if (*p == '\\')
        {
            p++;
            p++;
            continue;
        }
        if (*p == ':')
        {
            if (p + 1 < buf + len)
            {
                cur = p + 1;
                params[cnt++] = cur;
            }
            *p = '\0';
        }

        p++;
    }

    /* Replace newlines, question marks and colons in strings */
    for (i = 0; i < cnt; i++)
    {
        char *q = params[i];
        p = params[i];

        while (*p != '\0')
        {
            if (*p == '\\' && *(p + 1) == 'n')
            {
                p += 2;
                *(q++) = '\n';
            }
            else if (*p == '\\' && *(p + 1) == ':')
            {
                p += 2;
                *(q++) = ':';
            }
            else if (*p == '\\' && *(p + 1) == '?')
            {
                p += 2;
                *(q++) = '?';
            }
            else
                *(q++) = *(p++);
        }
        *q = '\0';
    }

    return (cnt);
}

int verify_int(char *str, int *value)
{
    size_t i;
    int minus = 0;
    int digits = 1;
    int hex = 0;

    for (i = 0; i < strlen(str); i++)
    {
        if (i == 0 && str[i] == '-')
        {
            minus = 1;
            continue;
        }

        if (((minus && i == 2) || (!minus && i == 1)) &&
                str[i - 1] == '0' && str[i] == 'x')
        {
            hex = 1;
            continue;
        }

        if (!((str[i] >= '0' && str[i] <= '9') ||
                (hex && ((str[i] >= 'a' && str[i] <= 'f') ||
                         (str[i] >= 'A' && str[i] <= 'F')))))
        {
            digits = 0;
            break;
        }
    }

    if (digits)
    {
        if (hex)
            *value = strtol(str, NULL, 16);
        else
            *value = strtol(str, NULL, 10);

        return (0);
    }
    rt_kprintf("Expected integer for parameter and got: %s\n", str);
    return (KEY_VALUE_MAPPING_NOT_FOUND);
}

int verify_string(char **str)
{
    if ((*str)[0] != '"' ||
            (*str)[strlen(*str) - 1] != '"')
    {
        rt_kprintf("Expected string (with \"\") for parameter and got: %s\n", *str);
        return (-1);
    }

    (*str)++;
    (*str)[strlen(*str) - 1] = '\0';

    return (0);
}

#if defined (NU_UNIT_TEST_ENABLE_AES)

void test_suite_aes_encrypt_ecb(char *hex_key_string, char *hex_src_string,
                                char *hex_dst_string, int setkey_result)
{
    mbedtls_aes_context ctx;
    int key_len;

    memset(s_key_str, 0x00, 100);
    memset(s_src_str, 0x00, 100);
    memset(s_dst_str, 0x00, 100);
    memset(s_output_buf, 0x00, 100);
    mbedtls_aes_init(&ctx);

    key_len = unhexify(s_key_str, hex_key_string);
    unhexify(s_src_str, hex_src_string);

    TEST_ASSERT(mbedtls_aes_setkey_enc(&ctx, s_key_str, key_len * 8) == setkey_result);
    if (setkey_result == 0)
    {
        TEST_ASSERT(mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, s_src_str, s_output_buf) == 0);
        hexify(s_dst_str, s_output_buf, 16);

        TEST_ASSERT(strcmp((char *) s_dst_str, hex_dst_string) == 0);
        pass_cnt++;
    }

exit:
    mbedtls_aes_free(&ctx);
}

static void test_suite_aes_decrypt_ecb(char *hex_key_string, char *hex_src_string,
                                       char *hex_dst_string, int setkey_result)
{
    mbedtls_aes_context ctx;
    int key_len;

    memset(s_key_str, 0x00, 100);
    memset(s_src_str, 0x00, 100);
    memset(s_dst_str, 0x00, 100);
    memset(s_output_buf, 0x00, 100);
    mbedtls_aes_init(&ctx);

    key_len = unhexify(s_key_str, hex_key_string);
    unhexify(s_src_str, hex_src_string);

    TEST_ASSERT(mbedtls_aes_setkey_dec(&ctx, s_key_str, key_len * 8) == setkey_result);
    if (setkey_result == 0)
    {
        TEST_ASSERT(mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, s_src_str, s_output_buf) == 0);
        hexify(s_dst_str, s_output_buf, 16);

        TEST_ASSERT(strcmp((char *) s_dst_str, hex_dst_string) == 0);
        pass_cnt++;
    }

exit:
    mbedtls_aes_free(&ctx);
}

static void test_suite_aes_encrypt_cbc(char *hex_key_string, char *hex_iv_string,
                                       char *hex_src_string, char *hex_dst_string,
                                       int cbc_result)
{
    mbedtls_aes_context ctx;
    int key_len, data_len;

    memset(s_key_str, 0x00, 100);
    memset(s_iv_str, 0x00, 100);
    memset(s_src_str, 0x00, 100);
    memset(s_dst_str, 0x00, 100);
    memset(s_output_buf, 0x00, 100);
    mbedtls_aes_init(&ctx);

    key_len = unhexify(s_key_str, hex_key_string);
    unhexify(s_iv_str, hex_iv_string);
    data_len = unhexify(s_src_str, hex_src_string);

    mbedtls_aes_setkey_enc(&ctx, s_key_str, key_len * 8);
    TEST_ASSERT(mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, data_len, s_iv_str, s_src_str, s_output_buf) == cbc_result);
    if (cbc_result == 0)
    {
        hexify(s_dst_str, s_output_buf, data_len);

        TEST_ASSERT(strcmp((char *) s_dst_str, hex_dst_string) == 0);
        pass_cnt++;
    }

exit:
    mbedtls_aes_free(&ctx);
}

static void test_suite_aes_decrypt_cbc(char *hex_key_string, char *hex_iv_string,
                                       char *hex_src_string, char *hex_dst_string,
                                       int cbc_result)
{
    mbedtls_aes_context ctx;
    int key_len, data_len;

    memset(s_key_str, 0x00, 100);
    memset(s_iv_str, 0x00, 100);
    memset(s_src_str, 0x00, 100);
    memset(s_dst_str, 0x00, 100);
    memset(s_output_buf, 0x00, 100);
    mbedtls_aes_init(&ctx);

    key_len = unhexify(s_key_str, hex_key_string);
    unhexify(s_iv_str, hex_iv_string);
    data_len = unhexify(s_src_str, hex_src_string);

    mbedtls_aes_setkey_dec(&ctx, s_key_str, key_len * 8);
    TEST_ASSERT(mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, data_len, s_iv_str, s_src_str, s_output_buf) == cbc_result);
    if (cbc_result == 0)
    {
        hexify(s_dst_str, s_output_buf, data_len);

        TEST_ASSERT(strcmp((char *) s_dst_str, hex_dst_string) == 0);
        pass_cnt++;
    }

exit:
    mbedtls_aes_free(&ctx);
}

static void test_suite_aes_encrypt_cfb128(char *hex_key_string, char *hex_iv_string,
        char *hex_src_string, char *hex_dst_string)
{
    mbedtls_aes_context ctx;
    size_t iv_offset = 0;
    int key_len;

    memset(s_key_str, 0x00, 100);
    memset(s_iv_str, 0x00, 100);
    memset(s_src_str, 0x00, 100);
    memset(s_dst_str, 0x00, 100);
    memset(s_output_buf, 0x00, 100);
    mbedtls_aes_init(&ctx);

    key_len = unhexify(s_key_str, hex_key_string);
    unhexify(s_iv_str, hex_iv_string);
    unhexify(s_src_str, hex_src_string);

    mbedtls_aes_setkey_enc(&ctx, s_key_str, key_len * 8);
    TEST_ASSERT(mbedtls_aes_crypt_cfb128(&ctx, MBEDTLS_AES_ENCRYPT, 16, &iv_offset, s_iv_str, s_src_str, s_output_buf) == 0);
    hexify(s_dst_str, s_output_buf, 16);

    TEST_ASSERT(strcmp((char *) s_dst_str, hex_dst_string) == 0);
    pass_cnt++;

exit:
    mbedtls_aes_free(&ctx);
}

static void test_suite_aes_decrypt_cfb128(char *hex_key_string, char *hex_iv_string,
        char *hex_src_string, char *hex_dst_string)
{
    mbedtls_aes_context ctx;
    size_t iv_offset = 0;
    int key_len;

    memset(s_key_str, 0x00, 100);
    memset(s_iv_str, 0x00, 100);
    memset(s_src_str, 0x00, 100);
    memset(s_dst_str, 0x00, 100);
    memset(s_output_buf, 0x00, 100);
    mbedtls_aes_init(&ctx);

    key_len = unhexify(s_key_str, hex_key_string);
    unhexify(s_iv_str, hex_iv_string);
    unhexify(s_src_str, hex_src_string);

    mbedtls_aes_setkey_enc(&ctx, s_key_str, key_len * 8);
    TEST_ASSERT(mbedtls_aes_crypt_cfb128(&ctx, MBEDTLS_AES_DECRYPT, 16, &iv_offset, s_iv_str, s_src_str, s_output_buf) == 0);
    hexify(s_dst_str, s_output_buf, 16);

    TEST_ASSERT(strcmp((char *) s_dst_str, hex_dst_string) == 0);
    pass_cnt++;

exit:
    mbedtls_aes_free(&ctx);
}

static void test_suite_aes_selftest()
{
    TEST_ASSERT(mbedtls_aes_self_test(1) == 0);

exit:
    return;
}

static int  open_AES_test_vector(int vector_no)
{
    s_file_idx = 0;

    if (vector_no == 1)
    {
        rt_kprintf("\n\nOpen test vector test_suite_aes.cbc.data.\n");
        s_file_base_ptr = (uint8_t *)szVectorBase_AES_CBC;
        s_file_size = i32VectorBase_AES_CBC_size;
        return 0;
    }
    else if (vector_no == 2)
    {
        rt_kprintf("\n\nOpen test vector test_suite_aes.cfb.data.\n");
        s_file_base_ptr = (uint8_t *)szVectorBase_AES_CFB;
        s_file_size = (uint32_t)i32VectorBase_AES_CFB_size;
        return 0;
    }
    else if (vector_no == 3)
    {
        rt_kprintf("\n\nOpen test vector test_suite_aes.ecb.data.\n");
        s_file_base_ptr = (uint8_t *)szVectorBase_AES_ECB;
        s_file_size = (uint32_t)i32VectorBase_AES_ECB_size;
        return 0;
    }
    return -1;
}

static int dispatch_AES_test(int cnt, char *params[50])
{
    int ret;

    ret = DISPATCH_TEST_SUCCESS;

    if (strcmp(params[0], "aes_encrypt_ecb") == 0)
    {
        char *param1 = params[1];
        char *param2 = params[2];
        char *param3 = params[3];
        int param4;

        if (cnt != 5)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 5);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        if (verify_string(&param1) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param2) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param3) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_int(params[4], &param4) != 0) return (DISPATCH_INVALID_TEST_DATA);

        test_suite_aes_encrypt_ecb(param1, param2, param3, param4);
        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "aes_decrypt_ecb") == 0)
    {
        char *param1 = params[1];
        char *param2 = params[2];
        char *param3 = params[3];
        int param4;

        if (cnt != 5)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 5);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        if (verify_string(&param1) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param2) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param3) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_int(params[4], &param4) != 0) return (DISPATCH_INVALID_TEST_DATA);

        test_suite_aes_decrypt_ecb(param1, param2, param3, param4);
        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "aes_encrypt_cbc") == 0)
    {
        char *param1 = params[1];
        char *param2 = params[2];
        char *param3 = params[3];
        char *param4 = params[4];
        int param5;

        if (cnt != 6)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 6);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        if (verify_string(&param1) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param2) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param3) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param4) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_int(params[5], &param5) != 0) return (DISPATCH_INVALID_TEST_DATA);

        test_suite_aes_encrypt_cbc(param1, param2, param3, param4, param5);
        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "aes_decrypt_cbc") == 0)
    {
        char *param1 = params[1];
        char *param2 = params[2];
        char *param3 = params[3];
        char *param4 = params[4];
        int param5;

        if (cnt != 6)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 6);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        if (verify_string(&param1) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param2) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param3) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param4) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_int(params[5], &param5) != 0) return (DISPATCH_INVALID_TEST_DATA);

        test_suite_aes_decrypt_cbc(param1, param2, param3, param4, param5);
        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "aes_encrypt_cfb128") == 0)
    {
        char *param1 = params[1];
        char *param2 = params[2];
        char *param3 = params[3];
        char *param4 = params[4];

        if (cnt != 5)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 5);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        if (verify_string(&param1) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param2) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param3) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param4) != 0) return (DISPATCH_INVALID_TEST_DATA);

        test_suite_aes_encrypt_cfb128(param1, param2, param3, param4);
        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "aes_decrypt_cfb128") == 0)
    {
        char *param1 = params[1];
        char *param2 = params[2];
        char *param3 = params[3];
        char *param4 = params[4];

        if (cnt != 5)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 5);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        if (verify_string(&param1) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param2) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param3) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param4) != 0) return (DISPATCH_INVALID_TEST_DATA);

        test_suite_aes_decrypt_cfb128(param1, param2, param3, param4);
        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "aes_selftest") == 0)
    {
        if (cnt != 1)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 1);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        test_suite_aes_selftest();
        return (DISPATCH_TEST_SUCCESS);
    }
    else
    {
        rt_kprintf("FAILED\nSkipping unknown test function '%s'\n", params[0]);
        ret = DISPATCH_TEST_FN_NOT_FOUND;
    }

    return (ret);
}

static void test_crypto_aes(void)
{
    int cnt, vector_no;
    char  *params[50];
    int   is_eof;

    for (vector_no = 1; ; vector_no++)
    {
        if (open_AES_test_vector(vector_no) != 0)
            break;

        pass_cnt = 0;

        while (1)
        {
            while (1)
            {
                is_eof = get_line();
                if (is_eof || (strncmp(g_line_buff, "aes_", 4) == 0))
                    break;
            }
            if (is_eof)
                break;

            cnt = parse_arguments(g_line_buff, sizeof(g_line_buff), params);

            dispatch_AES_test(cnt, params);
        }
        rt_kprintf("PASS count: %d\n", pass_cnt);

    }
    rt_kprintf("All AES test file done.\n");
}

#endif

#if defined (NU_UNIT_TEST_ENABLE_SHA)

void test_suite_mbedtls_sha1(char *hex_src_string, char *hex_hash_string)
{
    int src_len;
    unsigned char *sha_src_str = rt_malloc(SHA_MAX_DATASIZE);
    if (sha_src_str == RT_NULL)
    {
        goto exit;
    }

    memset(s_hash_str, 0x00, 41);
    memset(s_output_buf, 0x00, 41);

    src_len = unhexify(sha_src_str, hex_src_string);

    if (src_len != 0)
    {
        TEST_ASSERT(src_len < SHA_MAX_DATASIZE);

        mbedtls_sha1(sha_src_str, src_len, s_output_buf);

        hexify(s_hash_str, s_output_buf, 20);

        TEST_ASSERT(strcmp((char *) s_hash_str, hex_hash_string) == 0);
    }

exit:
    if (sha_src_str)
    {
        rt_free(sha_src_str);
    }

    return;
}

void test_suite_sha224(char *hex_src_string, char *hex_hash_string)
{
    int src_len;
    unsigned char *sha_src_str = rt_malloc(SHA_MAX_DATASIZE);
    if (sha_src_str == RT_NULL)
    {
        goto exit;
    }

    memset(s_hash_str, 0x00, 65);
    memset(s_output_buf, 0x00, 65);

    src_len = unhexify(sha_src_str, hex_src_string);

    if (src_len != 0)
    {
        TEST_ASSERT(src_len < SHA_MAX_DATASIZE);

        mbedtls_sha256(sha_src_str, src_len, s_output_buf, 1);
        hexify(s_hash_str, s_output_buf, 28);

        TEST_ASSERT(strcmp((char *) s_hash_str, hex_hash_string) == 0);
    }

exit:
    if (sha_src_str)
    {
        rt_free(sha_src_str);
    }

    return;
}

void test_suite_mbedtls_sha256(char *hex_src_string, char *hex_hash_string)
{
    int src_len;
    unsigned char *sha_src_str = rt_malloc(SHA_MAX_DATASIZE);
    if (sha_src_str == RT_NULL)
    {
        goto exit;
    }

    memset(s_hash_str, 0x00, 65);
    memset(s_output_buf, 0x00, 65);

    src_len = unhexify(sha_src_str, hex_src_string);

    if (src_len != 0)
    {
        TEST_ASSERT(src_len < SHA_MAX_DATASIZE);

        mbedtls_sha256(sha_src_str, src_len, s_output_buf, 0);
        hexify(s_hash_str, s_output_buf, 32);

        TEST_ASSERT(strcmp((char *) s_hash_str, hex_hash_string) == 0);
    }

exit:
    if (sha_src_str)
    {
        rt_free(sha_src_str);
    }

    return;
}

void test_suite_sha384(char *hex_src_string, char *hex_hash_string)
{
    int src_len;
    unsigned char *sha_src_str = rt_malloc(SHA_MAX_DATASIZE);
    if (sha_src_str == RT_NULL)
    {
        goto exit;
    }

    memset(s_hash_str, 0x00, 129);
    memset(s_output_buf, 0x00, 129);

    src_len = unhexify(sha_src_str, hex_src_string);

    if (src_len != 0)
    {
        TEST_ASSERT(src_len < SHA_MAX_DATASIZE);

        mbedtls_sha512(sha_src_str, src_len, s_output_buf, 1);
        hexify(s_hash_str, s_output_buf, 48);

        TEST_ASSERT(strcmp((char *) s_hash_str, hex_hash_string) == 0);
    }

exit:
    if (sha_src_str)
    {
        rt_free(sha_src_str);
    }

    return;
}

void test_suite_mbedtls_sha512(char *hex_src_string, char *hex_hash_string)
{
    int src_len;
    unsigned char *sha_src_str = rt_malloc(SHA_MAX_DATASIZE);
    if (sha_src_str == RT_NULL)
    {
        goto exit;
    }

    memset(s_hash_str, 0x00, 129);
    memset(s_output_buf, 0x00, 129);

    src_len = unhexify(sha_src_str, hex_src_string);

    if (src_len != 0)
    {
        TEST_ASSERT(src_len < SHA_MAX_DATASIZE);

        mbedtls_sha512(sha_src_str, src_len, s_output_buf, 0);
        hexify(s_hash_str, s_output_buf, 64);

        TEST_ASSERT(strcmp((char *) s_hash_str, hex_hash_string) == 0);
    }

exit:
    if (sha_src_str)
    {
        rt_free(sha_src_str);
    }

    return;
}

void test_suite_sha1_selftest()
{
    TEST_ASSERT(mbedtls_sha1_self_test(1) == 0);

exit:
    return;
}

void test_suite_sha256_selftest()
{
    TEST_ASSERT(mbedtls_sha256_self_test(1) == 0);

exit:
    return;
}

void test_suite_sha512_selftest()
{
    TEST_ASSERT(mbedtls_sha512_self_test(1) == 0);

exit:
    return;
}

int dispatch_SHA_test(int cnt, char *params[50])
{
    int ret;

    ret = DISPATCH_TEST_SUCCESS;

    if (strcmp(params[0], "mbedtls_sha1") == 0)
    {
        char *param1 = params[1];
        char *param2 = params[2];

        if (cnt != 3)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 3);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        if (verify_string(&param1) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param2) != 0) return (DISPATCH_INVALID_TEST_DATA);

        if (param1 && param2)
            rt_kprintf("\n\n[%s] %s -> %s\n\n", __func__, param1, param2);

        test_suite_mbedtls_sha1(param1, param2);

        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "sha224") == 0)
    {
        char *param1 = params[1];
        char *param2 = params[2];

        if (cnt != 3)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 3);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        if (verify_string(&param1) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param2) != 0) return (DISPATCH_INVALID_TEST_DATA);

        if (param1 && param2)
            rt_kprintf("\n\n[%s] %s -> %s\n\n", __func__, param1, param2);

        test_suite_sha224(param1, param2);
        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "mbedtls_sha256") == 0)
    {
        char *param1 = params[1];
        char *param2 = params[2];

        if (cnt != 3)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 3);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        if (verify_string(&param1) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param2) != 0) return (DISPATCH_INVALID_TEST_DATA);

        if (param1 && param2)
            rt_kprintf("\n\n[%s] %s -> %s\n\n", __func__, param1, param2);

        test_suite_mbedtls_sha256(param1, param2);
        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "sha384") == 0)
    {
        char *param1 = params[1];
        char *param2 = params[2];

        if (cnt != 3)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 3);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        if (verify_string(&param1) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param2) != 0) return (DISPATCH_INVALID_TEST_DATA);

        if (param1 && param2)
            rt_kprintf("\n\n[%s] %s -> %s\n\n", __func__, param1, param2);

        test_suite_sha384(param1, param2);
        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "mbedtls_sha512") == 0)
    {
        char *param1 = params[1];
        char *param2 = params[2];

        if (cnt != 3)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 3);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        if (verify_string(&param1) != 0) return (DISPATCH_INVALID_TEST_DATA);
        if (verify_string(&param2) != 0) return (DISPATCH_INVALID_TEST_DATA);

        if (param1 && param2)
            rt_kprintf("\n\n[%s] %s -> %s\n\n", __func__, param1, param2);

        test_suite_mbedtls_sha512(param1, param2);
        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "sha1_selftest") == 0)
    {
        if (cnt != 3)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 1);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        test_suite_sha1_selftest();
        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "sha256_selftest") == 0)
    {
        if (cnt != 3)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 1);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        test_suite_sha256_selftest();
        return (DISPATCH_TEST_SUCCESS);
    }
    else if (strcmp(params[0], "sha512_selftest") == 0)
    {
        if (cnt != 3)
        {
            rt_kprintf("\nIncorrect argument count (%d != %d)\n", cnt, 1);
            return (DISPATCH_INVALID_TEST_DATA);
        }

        test_suite_sha512_selftest();
        return (DISPATCH_TEST_SUCCESS);
    }
    else

    {
        rt_kprintf("FAILED\nSkipping unknown test function '%s'\n", params[0]);
        ret = DISPATCH_TEST_FN_NOT_FOUND;
    }
    return (ret);
}

int  open_SHA_test_vector(int vector_no)
{
    s_file_idx = 0;

    if (vector_no == 1)
    {
        rt_kprintf("\n\nOpen test vector test_suite_sha.data.\n");
        s_file_base_ptr = (uint8_t *)szVectorBase_SHA;
        s_file_size = (uint32_t)i32VectorBase_SHA_size;
        return 0;
    }
    return -1;
}

static void test_crypto_sha(void)
{
    int cnt, vector_no;
    char  *params[50];
    int   is_eof;

    for (vector_no = 1; ; vector_no++)
    {
        if (open_SHA_test_vector(vector_no) != 0)
            break;

        pass_cnt = 0;

        while (1)
        {
            while (1)
            {
                is_eof = get_line();

                if (is_eof)
                    break;

                cnt = parse_arguments(g_line_buff, sizeof(g_line_buff), params);

                if (strcmp(params[0], "depends_on") == 0)     /* ignore */
                    continue;

                if (cnt > 2)
                    break;
            }
            if (is_eof)
                break;

            dispatch_SHA_test(cnt, params);
            pass_cnt++;
        }
        rt_kprintf("PASS count: %d\n", pass_cnt);
    }
    rt_kprintf("All SHA test file done.\n");
}

#endif //defined (NU_UNIT_TEST_ENABLE_SHA)

//CRC online test http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
#if defined(RT_HWCRYPTO_USING_CRC)
static void test_suite_crc8(void)
{
    const uint8_t au8CRCSrcPattern[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    uint32_t u32TargetChecksum = 0x58, u32CalChecksum = 0;

    struct rt_hwcrypto_ctx *ctx;
    struct hwcrypto_crc_cfg cfg =
    {
        .last_val = 0x5A,           //seed
        .poly = 0x00000007,         //CRC8
        .width = 8,
        .xorout = 0x00000000,
        .flags = 0,
    };

    //create CRC8 crypto context
    ctx = rt_hwcrypto_crc_create(rt_hwcrypto_dev_default(), HWCRYPTO_CRC_CRC8);
    if (ctx == RT_NULL)
    {
        rt_kprintf("CRC8 create hardware crypto context failed \n");
        return;
    }

    //setup crypto context
    rt_hwcrypto_crc_cfg(ctx, &cfg);

    //calculate CRC checksum
    u32CalChecksum = rt_hwcrypto_crc_update(ctx, au8CRCSrcPattern, sizeof(au8CRCSrcPattern));
    rt_kprintf("CRC8 checksum is 0x%X ... %s.\n", u32CalChecksum, (u32CalChecksum == u32TargetChecksum) ? "PASS" : "FAIL");
    rt_hwcrypto_crc_destroy(ctx);
}

static void test_suite_crc16(void)
{
    const uint8_t au8CRCSrcPattern[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38};
    uint32_t u32TargetChecksum = 0x972D, u32CalChecksum = 0;

    struct rt_hwcrypto_ctx *ctx;
    struct hwcrypto_crc_cfg cfg =
    {
        .last_val = 0xFFFF,         //seed
        .poly = 0x8005,             //CRC16
        .width = 16,
        .xorout = 0x00000000,
        .flags = 0,
    };

    //create CRC8 crypto context
    ctx = rt_hwcrypto_crc_create(rt_hwcrypto_dev_default(), HWCRYPTO_CRC_CRC16);
    if (ctx == RT_NULL)
    {
        rt_kprintf("CRC16 create hardware crypto context failed \n");
        return;
    }

    //setup crypto context
    rt_hwcrypto_crc_cfg(ctx, &cfg);

    //calculate CRC checksum
    u32CalChecksum = rt_hwcrypto_crc_update(ctx, au8CRCSrcPattern, sizeof(au8CRCSrcPattern));
    rt_kprintf("CRC16 checksum is 0x%X ... %s.\n", u32CalChecksum, (u32CalChecksum == u32TargetChecksum) ? "PASS" : "FAIL");
    rt_hwcrypto_crc_destroy(ctx);
}

static void test_suite_crc32(void)
{
    const uint8_t au8CRCSrcPattern[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    uint32_t u32TargetChecksum = 0x3CF1A989, u32CalChecksum = 0;

    struct rt_hwcrypto_ctx *ctx;
    struct hwcrypto_crc_cfg cfg =
    {
        .last_val = 0xFFFF0000,         //seed
        .poly = 0x04C11DB7,             //CRC32
        .width = 32,
        .xorout = 0x00000000,
        .flags = 0,
    };

    //create CRC8 crypto context
    ctx = rt_hwcrypto_crc_create(rt_hwcrypto_dev_default(), HWCRYPTO_CRC_CRC32);

    if (ctx == RT_NULL)
    {
        rt_kprintf("CRC32 create hardware crypto context failed \n");
        return;
    }

    //setup crypto context
    rt_hwcrypto_crc_cfg(ctx, &cfg);

    //calculate CRC checksum
    u32CalChecksum = rt_hwcrypto_crc_update(ctx, au8CRCSrcPattern, sizeof(au8CRCSrcPattern));
    rt_kprintf("CRC32 checksum is 0x%X ... %s.\n", u32CalChecksum, (u32CalChecksum == u32TargetChecksum) ? "PASS" : "FAIL");
    rt_hwcrypto_crc_destroy(ctx);
}

static void test_suite_ccitt(void)
{
    const uint8_t au8CRCSrcPattern[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38};
    uint32_t u32TargetChecksum = 0xA12B, u32CalChecksum = 0;

    struct rt_hwcrypto_ctx *ctx;
    struct hwcrypto_crc_cfg cfg =
    {
        .last_val = 0xFFFF,         //seed
        .poly = 0x1021,
        .width = 16,
        .xorout = 0x00000000,
        .flags = 0,
    };

    //create CRC8 crypto context
    ctx = rt_hwcrypto_crc_create(rt_hwcrypto_dev_default(), HWCRYPTO_CRC_CRC32);

    if (ctx == RT_NULL)
    {
        rt_kprintf("CRC32 create hardware crypto context failed \n");
        return;
    }

    //setup crypto context
    rt_hwcrypto_crc_cfg(ctx, &cfg);

    //calculate CRC checksum
    //input address not alignment test
    u32CalChecksum = rt_hwcrypto_crc_update(ctx, au8CRCSrcPattern + 1, sizeof(au8CRCSrcPattern) - 1);
    rt_kprintf("CRC CCITT checksum is 0x%X ... %s.\n", u32CalChecksum, (u32CalChecksum == u32TargetChecksum) ? "PASS" : "FAIL");
    rt_hwcrypto_crc_destroy(ctx);
}
#endif //#if defined(RT_HWCRYPTO_USING_CRC)

#if defined(RT_HWCRYPTO_USING_RNG)
static void test_suite_rng(void)
{
#define GENERATE_COUNT 10
    struct rt_hwcrypto_ctx *ctx;
    int i;
    rt_uint32_t result;

    ctx = rt_hwcrypto_rng_create(rt_hwcrypto_dev_default());

    if (ctx == RT_NULL)
    {
        rt_kprintf("RNG create hardware crypto failed \n");
        return;
    }

    for (i = 0; i < GENERATE_COUNT; i ++)
    {
        result = rt_hwcrypto_rng_update_ctx(ctx);
        rt_kprintf("RNG1 value: %x \n", result);
    }

    rt_hwcrypto_rng_destroy(ctx);

    ctx = rt_hwcrypto_rng_create(rt_hwcrypto_dev_default());

    if (ctx == RT_NULL)
    {
        rt_kprintf("RNG create hardware crypto failed \n");
        return;
    }

    for (i = 0; i < GENERATE_COUNT; i ++)
    {
        result = rt_hwcrypto_rng_update_ctx(ctx);
        rt_kprintf("RNG2 value: %x \n", result);
    }

    rt_hwcrypto_rng_destroy(ctx);
}
#endif

static void test_crypto_crc(void)
{
#if defined(RT_HWCRYPTO_USING_CRC)
    test_suite_crc8();
    test_suite_crc16();
    test_suite_crc32();
    test_suite_ccitt();
#endif
}

static void test_crypto_trng(void)
{
#if defined(RT_HWCRYPTO_USING_RNG)
    test_suite_rng();
#endif
}

static void crypto_dump_test_setting(void)
{
    rt_kprintf("\n[CRYPTO utest] description\n");
    rt_kprintf("  purpose               : Verify AES, SHA, CRC and RNG hardware crypto coverage.\n");
#if defined(NU_UNIT_TEST_ENABLE_AES)
    rt_kprintf("  aes vectors (ecb)     : %d\n", i32VectorBase_AES_ECB_size);
    rt_kprintf("  aes vectors (cbc)     : %d\n", i32VectorBase_AES_CBC_size);
    rt_kprintf("  aes vectors (cfb)     : %d\n", i32VectorBase_AES_CFB_size);
#else
    rt_kprintf("  aes support           : disabled\n");
#endif
#if defined(NU_UNIT_TEST_ENABLE_SHA)
    rt_kprintf("  sha vectors           : %d\n", i32VectorBase_SHA_size);
    rt_kprintf("  sha max data size     : %d\n", SHA_MAX_DATASIZE);
#else
    rt_kprintf("  sha support           : disabled\n");
#endif
    rt_kprintf("  rng generate count    : %d\n\n", GENERATE_COUNT);
}

static rt_err_t utest_tc_init(void)
{
    crypto_dump_test_setting();
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
#if defined (NU_UNIT_TEST_ENABLE_AES)
    UTEST_UNIT_RUN(test_crypto_aes);
#endif

#if defined (NU_UNIT_TEST_ENABLE_SHA)
    UTEST_UNIT_RUN(test_crypto_sha);
#endif

    UTEST_UNIT_RUN(test_crypto_crc);
    UTEST_UNIT_RUN(test_crypto_trng);
}

UTEST_TC_EXPORT(testcase, UTEST_CMD_PREFIX"crypto",
                utest_tc_init, utest_tc_cleanup, 10);

#endif //#if (defined(BSP_USING_CRYPTO) && defined(RT_USING_HWCRYPTO))
