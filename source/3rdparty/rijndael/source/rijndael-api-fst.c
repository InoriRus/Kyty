/**
 * rijndael-api-fst.c
 *
 * @version 2.9 (December 2000)
 *
 * Optimised ANSI C code for the Rijndael cipher (now AES)
 *
 * @author Vincent Rijmen <vincent.rijmen@esat.kuleuven.ac.be>
 * @author Antoon Bosselaers <antoon.bosselaers@esat.kuleuven.ac.be>
 * @author Paulo Barreto <paulo.barreto@terra.com.br>
 *
 * This code is hereby placed in the public domain.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Acknowledgements:
 *
 * We are deeply indebted to the following people for their bug reports,
 * fixes, and improvement suggestions to this implementation. Though we
 * tried to list all contributions, we apologise in advance for any
 * missing reference.
 *
 * Andrew Bales <Andrew.Bales@Honeywell.com>
 * Markus Friedl <markus.friedl@informatik.uni-erlangen.de>
 * John Skodon <skodonj@webquill.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "rijndael-alg-fst.h"
#include "rijndael-api-fst.h"

int makeKey(keyInstance *key, BYTE direction, int keyLen, char *keyMaterial) {
	int i;
	char *keyMat;
	u8 cipherKey[MAXKB];
	
	if (key == NULL) {
		return BAD_KEY_INSTANCE;
	}

	if ((direction == DIR_ENCRYPT) || (direction == DIR_DECRYPT)) {
		key->direction = direction;
	} else {
		return BAD_KEY_DIR;
	}

	if ((keyLen == 128) || (keyLen == 192) || (keyLen == 256)) {
		key->keyLen = keyLen;
	} else {
		return BAD_KEY_MAT;
	}

	if (keyMaterial != NULL) {
		strncpy(key->keyMaterial, keyMaterial, keyLen/4);
	}

	/* initialize key schedule: */
	keyMat = key->keyMaterial;
 	for (i = 0; i < key->keyLen/8; i++) {
		int t, v;

		t = *keyMat++;
		if ((t >= '0') && (t <= '9')) v = (t - '0') << 4;
		else if ((t >= 'a') && (t <= 'f')) v = (t - 'a' + 10) << 4;
		else if ((t >= 'A') && (t <= 'F')) v = (t - 'A' + 10) << 4;
		else return BAD_KEY_MAT;
		
		t = *keyMat++;
		if ((t >= '0') && (t <= '9')) v ^= (t - '0');
		else if ((t >= 'a') && (t <= 'f')) v ^= (t - 'a' + 10);
		else if ((t >= 'A') && (t <= 'F')) v ^= (t - 'A' + 10);
		else return BAD_KEY_MAT;
		
		cipherKey[i] = (u8)v;
	}
	if (direction == DIR_ENCRYPT) {
		key->Nr = rijndaelKeySetupEnc(key->rk, cipherKey, keyLen);
	} else {
		key->Nr = rijndaelKeySetupDec(key->rk, cipherKey, keyLen);
	}
	rijndaelKeySetupEnc(key->ek, cipherKey, keyLen);
	return TRUE;
}

int cipherInit(cipherInstance *cipher, BYTE mode, char *IV) {
	if ((mode == MODE_ECB) || (mode == MODE_CBC) || (mode == MODE_CFB1)) {
		cipher->mode = mode;
	} else {
		return BAD_CIPHER_MODE;
	}
	if (IV != NULL) {
		int i;
 		for (i = 0; i < MAX_IV_SIZE; i++) {
			int t, j;

			t = IV[2*i];
			if ((t >= '0') && (t <= '9')) j = (t - '0') << 4;
			else if ((t >= 'a') && (t <= 'f')) j = (t - 'a' + 10) << 4;
			else if ((t >= 'A') && (t <= 'F')) j = (t - 'A' + 10) << 4;
			else return BAD_CIPHER_INSTANCE;
		
			t = IV[2*i+1];
			if ((t >= '0') && (t <= '9')) j ^= (t - '0');
			else if ((t >= 'a') && (t <= 'f')) j ^= (t - 'a' + 10);
			else if ((t >= 'A') && (t <= 'F')) j ^= (t - 'A' + 10);
			else return BAD_CIPHER_INSTANCE;
			
			cipher->IV[i] = (u8)j;
		}
	} else {
		memset(cipher->IV, 0, MAX_IV_SIZE);
	}
	return TRUE;
}

int blockEncrypt(cipherInstance *cipher, keyInstance *key,
		BYTE *input, int inputLen, BYTE *outBuffer) {
	int i, k, t, numBlocks;
	u8 block[16], *iv;

	if (cipher == NULL ||
		key == NULL ||
		key->direction == DIR_DECRYPT) {
		return BAD_CIPHER_STATE;
	}
	if (input == NULL || inputLen <= 0) {
		return 0; /* nothing to do */
	}

	numBlocks = inputLen/128;
	
	switch (cipher->mode) {
	case MODE_ECB:
		for (i = numBlocks; i > 0; i--) {
			rijndaelEncrypt(key->rk, key->Nr, input, outBuffer);
			input += 16;
			outBuffer += 16;
		}
		break;
		
	case MODE_CBC:
		iv = cipher->IV;
		for (i = numBlocks; i > 0; i--) {
			((u32*)block)[0] = ((u32*)input)[0] ^ ((u32*)iv)[0];
			((u32*)block)[1] = ((u32*)input)[1] ^ ((u32*)iv)[1];
			((u32*)block)[2] = ((u32*)input)[2] ^ ((u32*)iv)[2];
			((u32*)block)[3] = ((u32*)input)[3] ^ ((u32*)iv)[3];
			rijndaelEncrypt(key->rk, key->Nr, block, outBuffer);
			iv = outBuffer;
			input += 16;
			outBuffer += 16;
		}
		break;

    case MODE_CFB1:
		iv = cipher->IV;
        for (i = numBlocks; i > 0; i--) {
			memcpy(outBuffer, input, 16);
            for (k = 0; k < 128; k++) {
				rijndaelEncrypt(key->ek, key->Nr, iv, block);
                outBuffer[k >> 3] ^= (block[0] & 0x80U) >> (k & 7);
                for (t = 0; t < 15; t++) {
                	iv[t] = (iv[t] << 1) | (iv[t + 1] >> 7);
                }
               	iv[15] = (iv[15] << 1) | ((outBuffer[k >> 3] >> (7 - (k & 7))) & 1);
            }
            outBuffer += 16;
            input += 16;
        }
        break;

	default:
		return BAD_CIPHER_STATE;
	}
	
	return 128*numBlocks;
}

/**
 * Encrypt data partitioned in octets, using RFC 2040-like padding.
 *
 * @param   input           data to be encrypted (octet sequence)
 * @param   inputOctets		input length in octets (not bits)
 * @param   outBuffer       encrypted output data
 *
 * @return	length in octets (not bits) of the encrypted output buffer.
 */
int padEncrypt(cipherInstance *cipher, keyInstance *key,
		BYTE *input, int inputOctets, BYTE *outBuffer) {
	int i, numBlocks, padLen;
	u8 block[16], *iv;

	if (cipher == NULL ||
		key == NULL ||
		key->direction == DIR_DECRYPT) {
		return BAD_CIPHER_STATE;
	}
	if (input == NULL || inputOctets <= 0) {
		return 0; /* nothing to do */
	}

	numBlocks = inputOctets/16;

	switch (cipher->mode) {
	case MODE_ECB:
		for (i = numBlocks; i > 0; i--) {
			rijndaelEncrypt(key->rk, key->Nr, input, outBuffer);
			input += 16;
			outBuffer += 16;
		}
		padLen = 16 - (inputOctets - 16*numBlocks);
		assert(padLen > 0 && padLen <= 16);
		memcpy(block, input, 16 - padLen);
		memset(block + 16 - padLen, padLen, padLen);
		rijndaelEncrypt(key->rk, key->Nr, block, outBuffer);
		break;

	case MODE_CBC:
		iv = cipher->IV;
		for (i = numBlocks; i > 0; i--) {
			((u32*)block)[0] = ((u32*)input)[0] ^ ((u32*)iv)[0];
			((u32*)block)[1] = ((u32*)input)[1] ^ ((u32*)iv)[1];
			((u32*)block)[2] = ((u32*)input)[2] ^ ((u32*)iv)[2];
			((u32*)block)[3] = ((u32*)input)[3] ^ ((u32*)iv)[3];
			rijndaelEncrypt(key->rk, key->Nr, block, outBuffer);
			iv = outBuffer;
			input += 16;
			outBuffer += 16;
		}
		padLen = 16 - (inputOctets - 16*numBlocks);
		assert(padLen > 0 && padLen <= 16);
		for (i = 0; i < 16 - padLen; i++) {
			block[i] = input[i] ^ iv[i];
		}
		for (i = 16 - padLen; i < 16; i++) {
			block[i] = (BYTE)padLen ^ iv[i];
		}
		rijndaelEncrypt(key->rk, key->Nr, block, outBuffer);
		break;

	default:
		return BAD_CIPHER_STATE;
	}

	return 16*(numBlocks + 1);
}

int blockDecrypt(cipherInstance *cipher, keyInstance *key,
		BYTE *input, int inputLen, BYTE *outBuffer) {
	int i, k, t, numBlocks;
	u8 block[16], *iv;

	if (cipher == NULL ||
		key == NULL ||
		cipher->mode != MODE_CFB1 && key->direction == DIR_ENCRYPT) {
		return BAD_CIPHER_STATE;
	}
	if (input == NULL || inputLen <= 0) {
		return 0; /* nothing to do */
	}

	numBlocks = inputLen/128;

	switch (cipher->mode) {
	case MODE_ECB:
		for (i = numBlocks; i > 0; i--) {
			rijndaelDecrypt(key->rk, key->Nr, input, outBuffer);
			input += 16;
			outBuffer += 16;
		}
		break;
		
	case MODE_CBC:
		iv = cipher->IV;
		for (i = numBlocks; i > 0; i--) {
			rijndaelDecrypt(key->rk, key->Nr, input, block);
			((u32*)block)[0] ^= ((u32*)iv)[0];
			((u32*)block)[1] ^= ((u32*)iv)[1];
			((u32*)block)[2] ^= ((u32*)iv)[2];
			((u32*)block)[3] ^= ((u32*)iv)[3];
			memcpy(cipher->IV, input, 16);
			memcpy(outBuffer, block, 16);
			input += 16;
			outBuffer += 16;
		}
		break;

    case MODE_CFB1:
		iv = cipher->IV;
        for (i = numBlocks; i > 0; i--) {
			memcpy(outBuffer, input, 16);
            for (k = 0; k < 128; k++) {
				rijndaelEncrypt(key->ek, key->Nr, iv, block);
                for (t = 0; t < 15; t++) {
                	iv[t] = (iv[t] << 1) | (iv[t + 1] >> 7);
                }
               	iv[15] = (iv[15] << 1) | ((input[k >> 3] >> (7 - (k & 7))) & 1);
                outBuffer[k >> 3] ^= (block[0] & 0x80U) >> (k & 7);
            }
            outBuffer += 16;
            input += 16;
        }
        break;

	default:
		return BAD_CIPHER_STATE;
	}
	
	return 128*numBlocks;
}

int padDecrypt(cipherInstance *cipher, keyInstance *key,
		BYTE *input, int inputOctets, BYTE *outBuffer) {
	int i, numBlocks, padLen;
	u8 block[16];

	if (cipher == NULL ||
		key == NULL ||
		key->direction == DIR_ENCRYPT) {
		return BAD_CIPHER_STATE;
	}
	if (input == NULL || inputOctets <= 0) {
		return 0; /* nothing to do */
	}
	if (inputOctets % 16 != 0) {
		return BAD_DATA;
	}

	numBlocks = inputOctets/16;

	switch (cipher->mode) {
	case MODE_ECB:
		/* all blocks but last */
		for (i = numBlocks - 1; i > 0; i--) {
			rijndaelDecrypt(key->rk, key->Nr, input, outBuffer);
			input += 16;
			outBuffer += 16;
		}
		/* last block */
		rijndaelDecrypt(key->rk, key->Nr, input, block);
		padLen = block[15];
		if (padLen >= 16) {
			return BAD_DATA;
		}
		for (i = 16 - padLen; i < 16; i++) {
			if (block[i] != padLen) {
				return BAD_DATA;
			}
		}
		memcpy(outBuffer, block, 16 - padLen);
		break;
		
	case MODE_CBC:
		/* all blocks but last */
		for (i = numBlocks - 1; i > 0; i--) {
			rijndaelDecrypt(key->rk, key->Nr, input, block);
			((u32*)block)[0] ^= ((u32*)cipher->IV)[0];
			((u32*)block)[1] ^= ((u32*)cipher->IV)[1];
			((u32*)block)[2] ^= ((u32*)cipher->IV)[2];
			((u32*)block)[3] ^= ((u32*)cipher->IV)[3];
			memcpy(cipher->IV, input, 16);
			memcpy(outBuffer, block, 16);
			input += 16;
			outBuffer += 16;
		}
		/* last block */
		rijndaelDecrypt(key->rk, key->Nr, input, block);
		((u32*)block)[0] ^= ((u32*)cipher->IV)[0];
		((u32*)block)[1] ^= ((u32*)cipher->IV)[1];
		((u32*)block)[2] ^= ((u32*)cipher->IV)[2];
		((u32*)block)[3] ^= ((u32*)cipher->IV)[3];
		padLen = block[15];
		if (padLen <= 0 || padLen > 16) {
			return BAD_DATA;
		}
		for (i = 16 - padLen; i < 16; i++) {
			if (block[i] != padLen) {
				return BAD_DATA;
			}
		}
		memcpy(outBuffer, block, 16 - padLen);
		break;
	
	default:
		return BAD_CIPHER_STATE;
	}
	
	return 16*numBlocks - padLen;
}

#ifdef INTERMEDIATE_VALUE_KAT
/**
 *	cipherUpdateRounds:
 *
 *	Encrypts/Decrypts exactly one full block a specified number of rounds.
 *	Only used in the Intermediate Value Known Answer Test.	
 *
 *	Returns:
 *		TRUE - on success
 *		BAD_CIPHER_STATE - cipher in bad state (e.g., not initialized)
 */
int cipherUpdateRounds(cipherInstance *cipher, keyInstance *key,
		BYTE *input, int inputLen, BYTE *outBuffer, int rounds) {
	u8 block[16];

	if (cipher == NULL || key == NULL) {
		return BAD_CIPHER_STATE;
	}

	memcpy(block, input, 16);

	switch (key->direction) {
	case DIR_ENCRYPT:
		rijndaelEncryptRound(key->rk, key->Nr, block, rounds);
		break;
		
	case DIR_DECRYPT:
		rijndaelDecryptRound(key->rk, key->Nr, block, rounds);
		break;
		
	default:
		return BAD_KEY_DIR;
	} 

	memcpy(outBuffer, block, 16);
	
	return TRUE;
}
#endif /* INTERMEDIATE_VALUE_KAT */
