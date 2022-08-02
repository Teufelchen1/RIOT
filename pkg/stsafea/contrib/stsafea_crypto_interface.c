/**
  ******************************************************************************
  * @file    stsafea_crypto_interface.c
  * @author  SMD application team, Bennet Blischke
  * @version V3.2.0
  * @brief   Crypto Interface file to support the crypto services required by the
  *          STSAFE-A Middleware and offered by the mbedTLS crypto library:
  *           + Key Management
  *           + SHA
  *           + AES
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2016 STMicroelectronics</center></h2>
  *
  * This software is licensed under terms that can be found in the LICENSE file in
  * the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include <stdio.h>
#include <assert.h>
#include "stsafea_crypto.h"
#include "hashes/sha256.h"
#include "hashes/cmac.h"
#include "crypto/ciphers.h"
#include "crypto/modes/ecb.h"
#include "crypto/modes/cbc.h"

extern uint8_t  externHostCipherKey[];
extern uint8_t  externHostMacKey[];

static sha256_context_t sha256_ctx;
static cmac_context_t cmac_ctx;

/**
  * @brief   StSafeA_HostKeys_Init
  *          Initialize STSAFE-Axxx Host MAC and Cipher Keys that will be used by the crypto interface layer
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this Middleware
  *
  * @param   None
  * @retval  0 if success. An error code otherwise
  */
int32_t StSafeA_HostKeys_Init(void)
{
  return 0;
}

/**
  * @brief   StSafeA_SHA_Init
  *          SHA initialization function to initialize the SHA context
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this Middleware
  *
  * @param   InHashType : type of SHA
  *          This parameter can be one of the StSafeA_HashTypes_t enum values:
  *            @arg STSAFEA_SHA_256: 256-bits
  *            @arg STSAFEA_SHA_384: 384-bits
  * @param   ppShaCtx : SHA context to be initialized
  * @retval  None
  */
void StSafeA_SHA_Init(StSafeA_HashTypes_t InHashType, void **ppShaCtx)
{
  assert(InHashType == STSAFEA_SHA_256);
  *ppShaCtx = &sha256_ctx;
  sha256_init(&sha256_ctx);
}

/**
  * @brief   StSafeA_SHA_Update
  *          SHA update function to process SHA over a message data buffer.
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this Middleware
  *
  * @param   InHashType : type of SHA
  *          This parameter can be one of the StSafeA_HashTypes_t enum values:
  *            @arg STSAFEA_SHA_256: 256-bits
  *            @arg STSAFEA_SHA_384: 384-bits
  * @param   pShaCtx : SHA context
  * @param   pInMessage : message data buffer
  * @param   InMessageLength : message data buffer length
  * @retval  None
  */
void StSafeA_SHA_Update(StSafeA_HashTypes_t InHashType, void *pShaCtx, uint8_t *pInMessage, uint32_t InMessageLength)
{
  assert(InHashType == STSAFEA_SHA_256);
  sha256_update(pShaCtx, pInMessage, InMessageLength);
}

/**
  * @brief   StSafeA_SHA_Final
  *          SHA final function to finalize the SHA Digest
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this Middleware
  *
  * @param   InHashType : type of SHA
  *          This parameter can be one of the StSafeA_HashTypes_t enum values:
  *            @arg STSAFEA_SHA_256: 256-bits
  *            @arg STSAFEA_SHA_384: 384-bits
  * @param   ppShaCtx : SHA context to be finalized
  * @param   pMessageDigest : message digest data buffer
  * @retval  None
  */
void StSafeA_SHA_Final(StSafeA_HashTypes_t InHashType, void **ppShaCtx, uint8_t *pMessageDigest)
{
  assert(InHashType == STSAFEA_SHA_256);
  sha256_final(*ppShaCtx, pMessageDigest);
}



/**
  * @brief   StSafeA_AES_MAC_Start
  *          Start AES MAC computation
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this Middleware
  *
  * @param   ppAesMacCtx : AES MAC context
  * @retval  None
  */
void StSafeA_AES_MAC_Start(void **ppAesMacCtx)
{
  *ppAesMacCtx = &cmac_ctx;
  cmac_init(&cmac_ctx, externHostMacKey, STSAFEA_HOST_KEY_LENGTH);
}

/**
  * @brief   StSafeA_AES_MAC_Update
  *          Update / Add data to MAC computation
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this Middleware
  *
  * @param   pInData : data buffer
  * @param   InDataLength : data buffer length
  * @param   pAesMacCtx : AES MAC context
  * @retval  None
  */
void StSafeA_AES_MAC_Update(uint8_t *pInData, uint16_t InDataLength, void *pAesMacCtx)
{
  cmac_update(pAesMacCtx, pInData, InDataLength);
}

/**
  * @brief   StSafeA_AES_MAC_LastUpdate
  *          Update / Add data to MAC computation
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this Middleware
  *
  * @param   pInData : data buffer
  * @param   InDataLength : data buffer length
  * @param   pAesMacCtx : AES MAC context
  * @retval  None
  */
void StSafeA_AES_MAC_LastUpdate(uint8_t *pInData, uint16_t InDataLength, void *pAesMacCtx)
{
  StSafeA_AES_MAC_Update(pInData, InDataLength, pAesMacCtx);
}

/**
  * @brief   StSafeA_AES_MAC_Final
  *          Finalize AES MAC computation
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this Middleware
  *
  * @param   pOutMac : calculated MAC
  * @param   ppAesMacCtx : AES MAC context
  * @retval  None
  */
void StSafeA_AES_MAC_Final(uint8_t *pOutMac, void **ppAesMacCtx)
{
  cmac_final(*ppAesMacCtx, pOutMac);
}

/**
  * @brief   StSafeA_AES_ECB_Encrypt
  *          AES ECB Encryption
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this Middleware
  *
  * @param   pInData : plain data buffer
  * @param   pOutData : encrypted output data buffer
  * @param   InAesType : type of AES. Can be one of the following values:
  *            @arg STSAFEA_KEY_TYPE_AES_128: AES 128-bits
  *            @arg STSAFEA_KEY_TYPE_AES_256: AES 256-bits
  * @retval  0 if success, an error code otherwise
  */
int32_t StSafeA_AES_ECB_Encrypt(uint8_t *pInData, uint8_t *pOutData, uint8_t InAesType)
{
  assert(InAesType == STSAFEA_KEY_TYPE_AES_128);

  cipher_t aes_ecb_cipher;

  if(cipher_init(&aes_ecb_cipher, CIPHER_AES, externHostCipherKey, STSAFEA_HOST_KEY_LENGTH) < 0)
    return 1;

  if(cipher_encrypt_ecb(&aes_ecb_cipher, pInData, 16, pOutData) < 0) // TODO: Fix length assumption
    return 2;
  return 0;
}

/**
  * @brief   StSafeA_AES_CBC_Encrypt
  *          AES CBC Encryption
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this Middleware
  *
  * @param   pInData : plain data buffer
  * @param   InDataLength : plain data buffer length
  * @param   pOutData : encrypted output data buffer
  * @param   InInitialValue : initial value
  * @param   InAesType : type of AES. Can be one of the following values:
  *            @arg STSAFEA_KEY_TYPE_AES_128: AES 128-bits
  *            @arg STSAFEA_KEY_TYPE_AES_256: AES 256-bits
  * @retval  0 if success, an error code otherwise
  */
int32_t StSafeA_AES_CBC_Encrypt(uint8_t *pInData, uint16_t InDataLength, uint8_t *pOutData,
                                uint8_t *InInitialValue, uint8_t InAesType)
{
  assert(InAesType == STSAFEA_KEY_TYPE_AES_128);

  cipher_t aes_cbc_cipher;

  if(cipher_init(&aes_cbc_cipher, CIPHER_AES, externHostCipherKey, STSAFEA_HOST_KEY_LENGTH) < 0)
    return 1;

  if(cipher_encrypt_cbc(&aes_cbc_cipher, InInitialValue, pInData, InDataLength, pOutData) < 0)
    return 2;
  return 0;
}

/**
  * @brief   StSafeA_AES_CBC_Decrypt
  *          AES CBC Decryption
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this Middleware
  *
  * @param   pInData : encrypted data buffer
  * @param   InDataLength : encrypted data buffer length
  * @param   pOutData : plain output data buffer
  * @param   InInitialValue : initial value
  * @param   InAesType : type of AES. Can be one of the following values:
  *            @arg STSAFEA_KEY_TYPE_AES_128: AES 128-bits
  *            @arg STSAFEA_KEY_TYPE_AES_256: AES 256-bits
  * @retval  0 if success, an error code otherwise
  */
int32_t StSafeA_AES_CBC_Decrypt(uint8_t *pInData, uint16_t InDataLength, uint8_t *pOutData,
                                uint8_t *InInitialValue, uint8_t InAesType)
{
  assert(InAesType == STSAFEA_KEY_TYPE_AES_128);

  cipher_t aes_cbc_cipher;

  /* The StSafea API calls this function with pInData == pOutData
   * in other words, the output buffer is also the input buffer
   * Since the RIOT AES CBC implementation reuses the input buffer
   * (for IV) *after* writing to the output buffer, it reads garbage.
   * Thats why it is needed to do this quick memcpy (sadly)
   */
  uint8_t input_buffer[InDataLength];
  memcpy(input_buffer, pInData, InDataLength);

  if(cipher_init(&aes_cbc_cipher, CIPHER_AES, externHostCipherKey, STSAFEA_HOST_KEY_LENGTH) < 0)
    return 1;
  if ((pInData != NULL) && (pOutData != NULL) && (InInitialValue != NULL)) {
    if(cipher_decrypt_cbc(&aes_cbc_cipher, InInitialValue, input_buffer, InDataLength, pOutData) < 0)
      return 2;
  }
  return 0;
}
