/**
 * @brief       STSAFEA test application
 *
 * @author      Bennet Blischke <bennet.blischke@haw-hamburg.de>
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stsafea_conf.h"
#include "stsafea_service.h"
#include "stsafea_types.h"
#include "stsafea_core.h"
#include "stsafea_crypto.h"

#include "shell.h"
#include "msg.h"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

const uint8_t externHostMacKey[STSAFEA_HOST_KEY_LENGTH] =
{ 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 };
const uint8_t externHostCipherKey[STSAFEA_HOST_KEY_LENGTH] =
{ 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 };

static StSafeA_Handle_t stsafea_handle;
static uint8_t a_rx_tx_stsafea_data[STSAFEA_BUFFER_MAX_SIZE];

int se_random(int argc, char *argv[]);
int se_echo(int argc, char *argv[]);
int se_hostkey(int argc, char *argv[]);
int se_envelopekey(int argc, char *argv[]);
int se_install_hostkey(int argc, char *argv[]);
int se_generate_envelope_key(int argc, char *argv[]);
int se_wrap_unwrap(int argc, char *argv[]);

static const shell_command_t shell_commands[] = {
    { "se_random", "StSafea, get random data", se_random },
    { "se_echo", "StSafea, echo command", se_echo },
    { "se_hostkey", "StSafea, query the hostkey configuration", se_hostkey },
    { "se_envelopekey", "StSafea, query the envelopekey configuration", se_envelopekey },
    { "se_install_hostkey", "StSafea, permanently installs an host key", se_install_hostkey },
    { "se_generate_envelope_key", "StSafea, permanently installs an envelope key",
      se_generate_envelope_key },
    { "se_wrap_unwrap", "StSafea, test (un)wrapping of data using the envelope key",
      se_wrap_unwrap },
    { NULL, NULL, NULL },
};

int main(void)
{
    puts("Hello World!");

    StSafeA_ResponseCode_t resp;

    if ((resp = StSafeA_Init(&stsafea_handle, a_rx_tx_stsafea_data)) != STSAFEA_OK) {
        printf("ERROR: StSafeA_Init: %x\n", resp);
    }

    uint32_t version = StSafeA_GetVersion();

    printf("STSAFE-A1xx middleware revision: %u.%u.%u.%u\n", (uint8_t)((version >> 24) & 0xFF),
           (uint8_t)((version >> 16) & 0xFF), (uint8_t)((version >> 8) & 0xFF),
           (uint8_t)(version  & 0xFF));

    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    char line_buf[SHELL_DEFAULT_BUFSIZE];

    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}

int se_random(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    StSafeA_ResponseCode_t resp;
    uint8_t random[4] = { 0 };
    StSafeA_LVBuffer_t TrueRandom;

    TrueRandom.Data = random;
    resp = StSafeA_GenerateRandom(&stsafea_handle, STSAFEA_EPHEMERAL_RND, sizeof(random),
                                  &TrueRandom, STSAFEA_MAC_NONE);
    printf("Generate random: %x\n", resp);
    for (unsigned i = 0; i < sizeof(random); i++) {
        printf("random[%i]: 0x%x\n", i, random[i]);
    }
    return 0;
}

int se_echo(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <msg>\n", argv[0]);
    }

    uint16_t len = strlen(argv[1]);
    uint8_t *bytes = (uint8_t *)argv[1];

    uint8_t response_buffer[len];
    StSafeA_LVBuffer_t pOutLVResponse;

    pOutLVResponse.Data = &response_buffer[0];
    pOutLVResponse.Length = len;

    if (StSafeA_Echo(&stsafea_handle, bytes, len, &pOutLVResponse, 0) != STSAFEA_OK) {
        printf("ERROR: StSafeA_Echo;\n");
        return -1;
    }
    response_buffer[len] = 0;
    printf("Response: %s\n", (char *)response_buffer);
    return 0;
}

int se_hostkey(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    StSafeA_HostKeySlotBuffer_t pOutHostKeySlot;

    if (StSafeA_HostKeySlotQuery(&stsafea_handle, &pOutHostKeySlot,
                                 STSAFEA_MAC_NONE) != STSAFEA_OK) {
        puts("ERROR: StSafeA_HostKeySlotQuery");
        return -1;
    }
    else {
        printf("CMAC counter is at: %lu\n", pOutHostKeySlot.HostCMacSequenceCounter);
        if (pOutHostKeySlot.HostKeyPresenceFlag != 0) {
            puts("HostKey already set.");
            return 1;
        }
        else {
            puts("No HostKey set.");
            return 0;
        }
    }
}

int se_envelopekey(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    StSafeA_LocalEnvelopeKeyTableBuffer_t pOutLocalEnvelopeKeyTable;
    StSafeA_LocalEnvelopeKeyInformationRecordBuffer_t pOutLlocalEnvelopeKeySlot0InformationRecord;
    StSafeA_LocalEnvelopeKeyInformationRecordBuffer_t pOutLlocalEnvelopeKeySlot1InformationRecord;

    if (StSafeA_LocalEnvelopeKeySlotQuery(
            &stsafea_handle,
            &pOutLocalEnvelopeKeyTable,
            &pOutLlocalEnvelopeKeySlot0InformationRecord,
            &pOutLlocalEnvelopeKeySlot1InformationRecord,
            STSAFEA_MAC_HOST_CMAC
            ) != STSAFEA_OK) {
        printf("ERROR: StSafeA_LocalEnvelopeKeySlotQuery\n");
        return -1;
    }
    else {
        if (pOutLlocalEnvelopeKeySlot0InformationRecord.PresenceFlag) {
            printf("LocalEnvelopeKey 0: Present. Length: %u\n",
                   pOutLlocalEnvelopeKeySlot0InformationRecord.KeyLength);
        }
        else {
            puts("LocalEnvelopeKey 0: Not present");
        }
        if (pOutLlocalEnvelopeKeySlot1InformationRecord.PresenceFlag) {
            printf("LocalEnvelopeKey 1: Present. Length: %u\n",
                   pOutLlocalEnvelopeKeySlot1InformationRecord.KeyLength);
        }
        else {
            puts("LocalEnvelopeKey 1: Not present");
        }
    }

    return (pOutLlocalEnvelopeKeySlot0InformationRecord.PresenceFlag ||
            pOutLlocalEnvelopeKeySlot1InformationRecord.PresenceFlag);
}

int se_install_hostkey(int argc, char *argv[])
{
    uint8_t c_mac_key_and_cipher_key[2U * STSAFEA_HOST_KEY_LENGTH];
    uint8_t InAttributeTag = STSAFEA_TAG_HOST_KEY_SLOT;
    uint16_t InMAXSIZE = 2U * STSAFEA_HOST_KEY_LENGTH; // 32

    memcpy(c_mac_key_and_cipher_key, externHostMacKey, STSAFEA_HOST_KEY_LENGTH);
    memcpy(c_mac_key_and_cipher_key + STSAFEA_HOST_KEY_LENGTH, externHostCipherKey,
           STSAFEA_HOST_KEY_LENGTH);

    if (argc != 2) {
        puts("This is a one-time operation that CANNOT be undone.");
        puts("The CMAC and cipher key will be stored *PERMANENT* in the secure enclave.");
        printf("No keys have been stored yet, to store them execute %s -SAVE_FOREVER\n", argv[0]);
        puts("The following keys are compiled in this application: ");
    }
    else {
        if (strcmp(argv[1], "-SAVE_FOREVER") == 0) {
            if (StSafeA_PutAttribute(
                    &stsafea_handle, InAttributeTag, c_mac_key_and_cipher_key, InMAXSIZE,
                    STSAFEA_MAC_NONE
                    ) != STSAFEA_OK) {
                puts("ERROR: Could not install the hostkey");
                return -1;
            }
            else {
                puts("Successfully safed MAC & cipher key:");
            }
        }
    }

    puts("128 Bit MAC in hex:");
    for (int i = 0; i < 8; ++i) {
        printf("%x ", c_mac_key_and_cipher_key[i]);
    }
    printf("\n");
    for (int i = 8; i < 16; ++i) {
        printf("%x ", c_mac_key_and_cipher_key[i]);
    }
    printf("\n");
    printf("128 Bit cipher key in hex:\n");
    for (int i = 16; i < 24; ++i) {
        printf("%x ", c_mac_key_and_cipher_key[i]);
    }
    printf("\n");
    for (int i = 24; i < 32; ++i) {
        printf("%x ", c_mac_key_and_cipher_key[i]);
    }
    printf("\n");
    return 0;
}

int se_generate_envelope_key(int argc, char *argv[])
{
    uint8_t InKeyType = STSAFEA_KEY_TYPE_AES_256;
    uint8_t *pInSeed = NULL;
    uint16_t InSeedSize = 0;
    uint8_t InKeySlotNum = 0;

    if (argc != 2) {
        puts("This is a one-time operation that CANNOT be undone.");
        puts("The envelope key will be stored *PERMANENT* in the secure enclave.");
        printf("No key has been generated yet, to do so, execute %s -SAVE_FOREVER\n", argv[0]);
    }
    else {
        if (strcmp(argv[1], "-SAVE_FOREVER") == 0) {
            if (StSafeA_GenerateLocalEnvelopeKey(&stsafea_handle, InKeySlotNum, InKeyType, pInSeed,
                                                 InSeedSize, STSAFEA_MAC_HOST_CMAC) != STSAFEA_OK) {
                puts("ERROR: Could not generate an envelope key");
                return -1;
            }
            else {
                printf("Successfully generated an envelope key in slot: %u\n", InKeySlotNum);
            }
        }
    }
    return 0;
}

int se_wrap_unwrap(int argc, char *argv[])
{
    #define MAXSIZE 480                     // Max size supported by A110 is 480
    #define MAXSIZE_RESPONSE MAXSIZE + 8    // Max size + 8 for some reason ??
    uint8_t InKeySlotNum = 0;
    StSafeA_ResponseCode_t resp;

    uint8_t plainData[MAXSIZE];
    uint8_t encryptedData[MAXSIZE_RESPONSE];
    uint8_t decryptedData[MAXSIZE_RESPONSE];

    if (argc < 2) {
        printf("Usage: %s <msg>\n", argv[0]);
        return -1;
    }

    uint16_t str_len = strlen(argv[1]);
    char *str = argv[1];

    if (str_len % 8 != 0) {
        str_len = str_len + 8 - (str_len % 8);
    }
    printf("String length(padded): %u(%u)\n", strlen(str), str_len);
    if (str_len <= MAXSIZE) {
        memcpy(plainData, str, strlen(str));
    }
    else {
        puts("Message too long.");
        return -1;
    }

    // Buffer for holding the result of plain -> encrypted
    StSafeA_LVBuffer_t encryptedEnvelope;
    encryptedEnvelope.Data = encryptedData;
    encryptedEnvelope.Length = MAXSIZE_RESPONSE;

    // Buffer for holding the result of encrypted -> plain
    StSafeA_LVBuffer_t plainEnvelope;
    plainEnvelope.Data = decryptedData;
    plainEnvelope.Length = MAXSIZE_RESPONSE;

    if ((resp = StSafeA_WrapLocalEnvelope(
             &stsafea_handle,
             InKeySlotNum,
             plainData,
             str_len,
             &encryptedEnvelope,
             STSAFEA_MAC_HOST_CMAC,
             STSAFEA_ENCRYPTION_COMMAND_RESPONSE
             ))  != STSAFEA_OK) {
        printf("ERROR: StSafeA_WrapLocalEnvelope: %x\n", (unsigned)resp);
        return -1;
    }
    else {
        printf("Successfully wraped %u bytes of data\n", str_len);
        printf("plain: %s\n", str);
        printf("crypt: ");
        for (int i = 0; i < encryptedEnvelope.Length; i++) {
            printf("%x ", encryptedEnvelope.Data[i]);
        }
        printf("\n");

        if ((resp = StSafeA_UnwrapLocalEnvelope(
                 &stsafea_handle,
                 InKeySlotNum,
                 encryptedEnvelope.Data,
                 encryptedEnvelope.Length,
                 &plainEnvelope,
                 STSAFEA_MAC_HOST_CMAC,
                 STSAFEA_ENCRYPTION_RESPONSE
                 )) != STSAFEA_OK) {
            printf("ERROR: StSafeA_UnwrapLocalEnvelope: %x\n", (unsigned)resp);
            return -1;
        }
        else {
            printf("Successfully unwraped %u bytes of data\n", plainEnvelope.Length);
            printf("crypt: ");
            for (int i = 0; i < encryptedEnvelope.Length; i++) {
                printf("%c",
                       (encryptedEnvelope.Data[i] > 31 &&
                        encryptedEnvelope.Data[i] < 127) ? encryptedEnvelope.Data[i] : '.'
                       );
            }
            printf("\n");
            printf("plain: ");
            for (int i = 0; i < plainEnvelope.Length; i++) {
                printf("%c",
                       (plainEnvelope.Data[i] > 31 &&
                        plainEnvelope.Data[i] < 127) ? plainEnvelope.Data[i] : '.'
                       );
            }
            printf("\n");
        }
    }
    return 0;
}
