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
#include "xtimer.h"

const uint8_t externHostMacKey   [STSAFEA_HOST_KEY_LENGTH] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
const uint8_t externHostCipherKey[STSAFEA_HOST_KEY_LENGTH] = {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};

void echo(StSafeA_Handle_t *handle, const char *str, char *strBuffer, uint8_t lenBuffer);
int query_hostkey_info(StSafeA_Handle_t *handle);
int query_envelopekey_info(StSafeA_Handle_t *handle);
void setup_secure_channel(StSafeA_Handle_t *handle);
void productData(StSafeA_Handle_t *handle);
void generate_envelope_key(StSafeA_Handle_t *handle, uint8_t InKeySlotNum);
void wrap_unwrap(StSafeA_Handle_t *handle, uint8_t InKeySlotNum, const char * str);
void delete_key(StSafeA_Handle_t *pStSafeA, uint8_t InKeySlotNum);

int main(void)
{
    puts("Hello World!");

    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);
    xtimer_msleep(1000);
    
    StSafeA_Handle_t stsafea_handle;
    uint8_t a_rx_tx_stsafea_data [STSAFEA_BUFFER_MAX_SIZE];

    StSafeA_ResponseCode_t resp;
    if((resp = StSafeA_Init(&stsafea_handle, a_rx_tx_stsafea_data)) != STSAFEA_OK) {
        printf("ERROR: StSafeA_Init: %x\n", resp);
    }
    
    puts("Test getting random data:");
    uint8_t random[4]={0};
    StSafeA_LVBuffer_t TrueRandom;
    TrueRandom.Data = random;
    resp = StSafeA_GenerateRandom(&stsafea_handle, STSAFEA_EPHEMERAL_RND, sizeof(random), &TrueRandom, STSAFEA_MAC_NONE);
    printf("Generate random: %x\n", resp);
    for(unsigned i = 0; i < sizeof(random); i++)
        printf("random[%i]: 0x%x\n", i, random[i]);

    uint32_t version = StSafeA_GetVersion();
    printf("STSAFE-A1xx middleware revision: %u.%u.%u.%u\n", (uint8_t)((version >> 24) & 0xFF), (uint8_t)((version >> 16) & 0xFF), (uint8_t)((version >> 8) & 0xFF), (uint8_t)(version  & 0xFF));

    char buffer[100];
    echo(&stsafea_handle, "Hallo Peter", buffer, 100);
    puts("Testing Echo:"); puts("@ Hallo Peter"); printf("< %s\n", buffer);

    puts("Query Hostkey configuration:");
    if(query_hostkey_info(&stsafea_handle) == 0){
        puts("No Hostkey present. Installing a new hostkey...");
        setup_secure_channel(&stsafea_handle);
    }

    puts("Query Envelopekey configuration:");
    if(query_envelopekey_info(&stsafea_handle) == 0){
        puts("No Envelopekey present. Generation a new Envelopekey...");
        generate_envelope_key(&stsafea_handle, STSAFEA_KEY_SLOT_0);
    }
    puts("Testing wraping and unwraping via envelope...");
    wrap_unwrap(&stsafea_handle, STSAFEA_KEY_SLOT_0, "Peter, do you even lift?");

    //puts("Delete key slot 0 ...");
    //delete_key(&stsafea_handle, STSAFEA_KEY_SLOT_0);
    return 0;

}

void echo(StSafeA_Handle_t *handle, const char *str, char *strBuffer, uint8_t lenBuffer){
    uint16_t len = strlen(str);
    uint8_t *bytes = (uint8_t *)str;
    StSafeA_LVBuffer_t pOutLVResponse;
    pOutLVResponse.Data = (uint8_t *) strBuffer;
    pOutLVResponse.Length = lenBuffer;

    if(StSafeA_Echo(handle, bytes, len, &pOutLVResponse, 0) != STSAFEA_OK) {
        printf("ERROR: StSafeA_Echo;\n");
    }
    strBuffer[len] = 0;
}

/*
void hash_test(){
    uint8_t test_data[6] = {0x42, 0x41, 0x44, 0x43};
    StSafeA_InitHASH(&stsafea_handle);
    StSafeA_SHA_Update(STSAFEA_SHA_256, stsafea_handle.HashObj.HashCtx, test_data, 4);
    StSafeA_ComputeHASH(&stsafea_handle);
    printf("Hash: ");
    for (unsigned i = 0; i < STSAFEA_SHA_384_LENGTH; ++i)
        printf("%x ", stsafea_handle.HashObj.HashRes[i]);
    puts("");
}*/

int query_hostkey_info(StSafeA_Handle_t *handle){
    StSafeA_HostKeySlotBuffer_t pOutHostKeySlot;
    if(StSafeA_HostKeySlotQuery(handle, &pOutHostKeySlot, STSAFEA_MAC_NONE) != STSAFEA_OK){
        puts("ERROR: StSafeA_HostKeySlotQuery");
        return -1;
    } else {
        printf("CMAC counter is at: %lu\n", pOutHostKeySlot.HostCMacSequenceCounter);
        if(pOutHostKeySlot.HostKeyPresenceFlag != 0){
            puts("HostKey already set");
            return 1;
        } else {
            puts("No HostKey set");
            return 0;
        }
    }
}

int query_envelopekey_info(StSafeA_Handle_t *handle){
    StSafeA_LocalEnvelopeKeyTableBuffer_t pOutLocalEnvelopeKeyTable;
    StSafeA_LocalEnvelopeKeyInformationRecordBuffer_t pOutLlocalEnvelopeKeySlot0InformationRecord;
    StSafeA_LocalEnvelopeKeyInformationRecordBuffer_t pOutLlocalEnvelopeKeySlot1InformationRecord;

    if(StSafeA_LocalEnvelopeKeySlotQuery(
        handle,
        &pOutLocalEnvelopeKeyTable,
        &pOutLlocalEnvelopeKeySlot0InformationRecord,
        &pOutLlocalEnvelopeKeySlot1InformationRecord,
        STSAFEA_MAC_HOST_CMAC
        ) != STSAFEA_OK){
        printf("ERROR: StSafeA_LocalEnvelopeKeySlotQuery\n");
        return -1;
    } else {
        if(pOutLlocalEnvelopeKeySlot0InformationRecord.PresenceFlag){
            printf("LocalEnvelopeKey 0: Present. Length: %u\n", pOutLlocalEnvelopeKeySlot0InformationRecord.KeyLength);
        } else {
            puts("LocalEnvelopeKey 0: Not present");
        }
        if(pOutLlocalEnvelopeKeySlot1InformationRecord.PresenceFlag){
            printf("LocalEnvelopeKey 1: Present. Length: %u\n", pOutLlocalEnvelopeKeySlot1InformationRecord.KeyLength);
        } else {
            puts("LocalEnvelopeKey 1: Not present");
        }
    }

    return (pOutLlocalEnvelopeKeySlot0InformationRecord.PresenceFlag || pOutLlocalEnvelopeKeySlot1InformationRecord.PresenceFlag);
}

void setup_secure_channel(StSafeA_Handle_t *handle){
    (void) handle;
    uint8_t c_mac_key_and_cipher_key[2U * STSAFEA_HOST_KEY_LENGTH];
    uint8_t InAttributeTag = STSAFEA_TAG_HOST_KEY_SLOT;
    uint16_t InMAXSIZE = 2U * STSAFEA_HOST_KEY_LENGTH; // 32

    memcpy(c_mac_key_and_cipher_key, externHostMacKey, STSAFEA_HOST_KEY_LENGTH);
    memcpy(c_mac_key_and_cipher_key+STSAFEA_HOST_KEY_LENGTH, externHostCipherKey, STSAFEA_HOST_KEY_LENGTH);

    
    if(StSafeA_PutAttribute(
        handle, InAttributeTag, c_mac_key_and_cipher_key, InMAXSIZE, STSAFEA_MAC_NONE
    ) != STSAFEA_OK) {
        puts("ERROR: setup_secure_channel / StSafeA_PutAttribute");
    } else {
        puts("Successfully safed MAC & cipher key:");
        puts("128 Bit MAC:");
        for (int i = 0; i < 8; ++i)
        {
            printf("%x ", c_mac_key_and_cipher_key[i]);
        }
        printf("\n");
        for (int i = 8; i < 16; ++i)
        {
            printf("%x ", c_mac_key_and_cipher_key[i]);
        }
        printf("\n");
        printf("128 Bit cipher key:\n");
        for (int i = 16; i < 24; ++i)
        {
            printf("%x ", c_mac_key_and_cipher_key[i]);
        }
        printf("\n");
        for (int i = 24; i < 32; ++i)
        {
            printf("%x ", c_mac_key_and_cipher_key[i]);
        }
        printf("\n");
    }
}

void productData(StSafeA_Handle_t *handle){
    StSafeA_ProductDataBuffer_t pOutProductData;
    puts("productData");
    if(StSafeA_ProductDataQuery(handle, &pOutProductData, STSAFEA_MAC_HOST_CMAC) != STSAFEA_OK){
        printf("ERROR: StSafeA_ProductDataQuery\n");
    } else {
        printf("The whole length in number of octet: %u\n", pOutProductData.Length);
        printf("The tag for the Mask Identification (0x01): %u\n", pOutProductData.MaskIdentificationTag);
        printf("The length in number of octet for the Mask Identification: %u\n", pOutProductData.MaskIdentificationLength);
        //printf("The data for the Mask Identification: %u\n", pOutProductData.MaskIdentification[STSAFEA_MASK_ID]);
        printf("The tag for the ST Number (0x02): %u\n", pOutProductData.STNumberTag);
        printf("The length in number of octet for the ST Number: %u\n", pOutProductData.STNumberLength);
        //printf("The data for the ST Number: %u\n", pOutProductData.STNumber[STSAFEA_ST_NUMBER_LENGTH]);
        printf("The tag for the InputOutputBuffer Size (0x03): %u\n", pOutProductData.InputOutputBufferSizeTag);
        printf("The length in number of octet for the InputOutputBuffer Size: %u\n", pOutProductData.InputOutputBufferSizeLength);
        printf("The data for the InputOutputBuffer Size: %u\n", pOutProductData.InputOutputBufferSize);
        printf("The tag for the Atomicity Buffer Size (0x04): %u\n", pOutProductData.AtomicityBufferSizeTag);
        printf("The length in number of octet for the Atomicity Buffer Size: %u\n", pOutProductData.AtomicityBufferSizeLength);
        printf("The data for the Atomicity Buffer Size: %u\n", pOutProductData.AtomicityBufferSize);
        printf("The tag for the Non Volatile Memory Size (0x05): %u\n", pOutProductData.NonVolatileMemorySizeTag);
        printf("The length in number of octet for the Non Volatile Memory Size: %u\n", pOutProductData.NonVolatileMemorySizeLength);
        printf("The data for the Non Volatile Memory Size: %u\n", pOutProductData.NonVolatileMemorySize);
        printf("The tag for the Test Date (0x06): %u\n", pOutProductData.TestDateTag);
        printf("The length in number of octet for the Test Date: %u\n", pOutProductData.TestDateLength);
        printf("The data for the Test Date Size: %u\n", pOutProductData.TestDateSize);
        printf("The tag for the Internal Product Version (0x07): %u\n", pOutProductData.InternalProductVersionTag);
        printf("The length in number of octet for the Internal Product Version: %u\n", pOutProductData.InternalProductVersionLength);
        printf("The data for the Internal Product Version: %u\n", pOutProductData.InternalProductVersionSize);
        printf("The tag for the Module Date (0x08): %u\n", pOutProductData.ModuleDateTag);
        printf("The length in number of octet for the Module Date: %u\n", pOutProductData.ModuleDateLength);
        printf("The data for the Module Date: %u\n", pOutProductData.ModuleDateSize);
        printf("The tag for the firmware delivery traceability (0x09): %u\n", pOutProductData.FirmwareDeliveryTraceabilityTag);
        printf("The length in number of octet for the firmware delivery traceability: %u\n", pOutProductData.FirmwareDeliveryTraceabilityLength);
        //printf("The data for the firmware delivery traceability: %u\n", pOutProductData.FirmwareDeliveryTraceability[STSAFEA_FIRMWARE_TRACEABILITY_LENGTH]);
        printf("The tag for the blackbox delivery traceability (0x0A): %u\n", pOutProductData.BlackboxDeliveryTraceabilityTag);
        printf("The length in number of octet for the blackbox delivery traceability: %u\n", pOutProductData.BlackboxDeliveryTraceabilityLength);
        //printf("The data for the blackbox delivery traceability: %u\n", pOutProductData.BlackboxDeliveryTraceability[STSAFEA_BLACKBOX_TRACEABILITY_LENGTH]);
        printf("The tag for the perso ID (0x0B): %u\n", pOutProductData.PersoIdTag);
        printf("The length in number of octet for the perso ID: %u\n", pOutProductData.PersoIdLength);
        //printf("The data for the perso ID: %u\n", pOutProductData.PersoId[STSAFEA_PERSO_ID_LENGTH]);
        printf("The tag for the perso generation batch ID (0x0C): %u\n", pOutProductData.PersoGenerationBatchIdTag);
        printf("The length in number of octet for the perso generation batch ID: %u\n", pOutProductData.PersoGenerationBatchIdLength);
        //printf("The data for the perso generation batch ID: %u\n", pOutProductData.PersoGenerationBatchId[STSAFEA_PERSO_BATCH_ID_LENGTH]);
        printf("The tag for the perso date (0x0D): %u\n", pOutProductData.PersoDateTag);
        printf("The length in number of octet for the perso date: %u\n", pOutProductData.PersoDateLength);
        //printf("The data for the perso date: %u\n", pOutProductData.PersoDate[STSAFEA_PERSO_DATE_LENGTH]);
    }
}

void generate_envelope_key(StSafeA_Handle_t *handle, uint8_t InKeySlotNum){
    uint8_t InKeyType = STSAFEA_KEY_TYPE_AES_256;
    uint8_t *pInSeed = NULL;
    uint16_t InSeedSize = 0;
    if(StSafeA_GenerateLocalEnvelopeKey(handle, InKeySlotNum, InKeyType, pInSeed, InSeedSize, STSAFEA_MAC_HOST_CMAC) != STSAFEA_OK){
        puts("ERROR: StSafeA_GenerateLocalEnvelopeKey");
    } else {
        printf("Successfully generated an envelope key in slot: %u\n", InKeySlotNum);
    }
}

void wrap_unwrap(StSafeA_Handle_t *handle, uint8_t InKeySlotNum, const char * str){
    #define MAXSIZE 480 // Max size supported by A110 is 480
    #define MAXSIZE_RESPONSE MAXSIZE+8 // Max size +  8 for some reason ??
    StSafeA_ResponseCode_t resp;

    uint8_t plainData[MAXSIZE];
    uint8_t encryptedData[MAXSIZE_RESPONSE]; 
    uint8_t decryptedData[MAXSIZE_RESPONSE];

    uint16_t str_len = strlen(str);
    if(str_len % 8 != 0)
        str_len = str_len + 8 - (str_len % 8);
    printf("String length(padded): %u(%u)\n", strlen(str),str_len);
    if(str_len <= MAXSIZE){
        memcpy(plainData, str, strlen(str));
    } else {
        puts("String too long.");
        return;
    }

    // Buffer for holding the result of plain -> encrypted
    StSafeA_LVBuffer_t encryptedEnvelope;
    encryptedEnvelope.Data = encryptedData;
    encryptedEnvelope.Length = MAXSIZE_RESPONSE;

    // Buffer for holding the result of encrypted -> plain
    StSafeA_LVBuffer_t plainEnvelope;
    plainEnvelope.Data = decryptedData;
    plainEnvelope.Length = MAXSIZE_RESPONSE;

    if((resp = StSafeA_WrapLocalEnvelope(
        handle,
        InKeySlotNum,
        plainData,
        str_len,
        &encryptedEnvelope,
        STSAFEA_MAC_HOST_CMAC,
        STSAFEA_ENCRYPTION_COMMAND_RESPONSE
    ))  != STSAFEA_OK){
        printf("ERROR: StSafeA_WrapLocalEnvelope: %x\n", (unsigned)resp);
    } else {
        printf("Successfully wraped %u bytes of data\n", str_len);
        printf("plain: %s\n", str);
        printf("crypt: ");
        for(int i = 0; i < encryptedEnvelope.Length; i++){
            printf("%x ", encryptedEnvelope.Data[i]);
            //printf("%c", 
            //    (encryptedEnvelope.Data[i] > 31 &&  encryptedEnvelope.Data[i] < 127) ? encryptedEnvelope.Data[i] : '.'
            //);
        }
        printf("\n");

        if((resp = StSafeA_UnwrapLocalEnvelope(
            handle,
            InKeySlotNum,
            encryptedEnvelope.Data,
            encryptedEnvelope.Length,
            &plainEnvelope,
            STSAFEA_MAC_HOST_CMAC,
            STSAFEA_ENCRYPTION_RESPONSE
        )) != STSAFEA_OK){
            printf("ERROR: StSafeA_UnwrapLocalEnvelope: %x\n", (unsigned)resp);
        } else {
            printf("Successfully unwraped %u bytes of data\n", plainEnvelope.Length);
            printf("crypt: ");
            for(int i = 0; i < encryptedEnvelope.Length; i++){
                printf("%c", 
                    (encryptedEnvelope.Data[i] > 31 &&  encryptedEnvelope.Data[i] < 127) ? encryptedEnvelope.Data[i] : '.'
                );
            }
            printf("\n");
            printf("plain: ");
            for(int i = 0; i < plainEnvelope.Length; i++){
                printf("%c", 
                    (plainEnvelope.Data[i] > 31 &&  plainEnvelope.Data[i] < 127) ? plainEnvelope.Data[i] : '.'
                );
            }
            printf("\n");
        }
    }
}

void delete_key(StSafeA_Handle_t *handle, uint8_t InKeySlotNum){
    StSafeA_ResponseCode_t resp;

    uint8_t command_buffer[8];
    StSafeA_TLVBuffer_t delete_command;
    delete_command.Header = STSAFEA_CMD_DELETE_KEY;
    delete_command.LV.Data = command_buffer;
    delete_command.LV.Data[0] = STSAFEA_TAG_LOCAL_ENVELOPE_KEY_TABLE;
    delete_command.LV.Data[1] = InKeySlotNum;
    delete_command.LV.Length = 2U;

    uint8_t response_buffer[8];
    StSafeA_TLVBuffer_t response;
    response.LV.Data = response_buffer;
    response.LV.Length = 8;

    if((resp = StSafeA_RawCommand(handle, &delete_command, 8, &response, STSAFEA_MS_WAIT_TIME_CMD_DELETE_KEY, STSAFEA_MAC_HOST_CMAC)) != STSAFEA_OK){
        printf("ERROR: Could not delete key: %x\n", resp);
    } else {
        puts("Key deleted");
    }

    return;
}
