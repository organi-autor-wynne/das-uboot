// ----------------------------------------------------------------------------
//         ATMEL Microcontroller Software Support  - 
// ----------------------------------------------------------------------------
// DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
// DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// ----------------------------------------------------------------------------
/** \file
 *  \brief  Example of an Application That Uses the SHA204 Library
 *  \author Atmel Crypto Products
 *  \date   October 7, 2010
*/

#include <stddef.h>                   // data type definitions
#include "sha204_lib_return_codes.h"  // declarations of function return codes
#include "sha204_comm_marshaling.h"   // definitions and declarations for the Command module
#include "sha204_helper.h"
#include "linux/string.h"
#include "atsha204_defines.h"

#define KEY_ID 15

/** \brief This function serves as an example for
 *         the SHA204 MAC command.
 *
 *         In an infinite loop, it issues the same command
 *         sequence using the Command Marshaling layer of
 *         the SHA204 library.
 * @return exit status of application
 */
//TempKey认证
int tempkey_main(uint8_t *__key)
{
	// declared as "volatile" for easier debugging
	volatile uint8_t ret_code;
	struct sha204h_nonce_in_out args_nonce_tempkey;	
	uint8_t rand_out[MAC_RSP_SIZE];
 	struct sha204h_temp_key temp_key;
	struct sha204h_mac_in_out args_mac_tempkey;
	 uint8_t mac_calculated[SHA204_KEY_SIZE];
	 uint8_t mac[MAC_RSP_SIZE];
	
	uint8_t i,num;

	// Make the command buffer the size of the MAC command.
	 uint8_t command[MAC_COUNT_LONG];

	// Make the response buffer the size of a MAC response.
	 uint8_t response[MAC_RSP_SIZE];
#if 1
	//uint8_t __key[32]="12345678123456781234567812345678";
#else
	uint8_t __key[32]={0x66 ,0x66 ,0xD0 ,0x45 ,0x3A ,0xC2 ,0x25 ,0x57 ,
					   0xF6 ,0xD4 ,0x6B ,0x7D ,0xDF ,0x96 ,0x89 ,0xDA ,
					   0x2C ,0xBC ,0xD9 ,0xC3 ,0x5A ,0xD5 ,0x9A ,0x42 ,
					   0xDE ,0x30 ,0x32 ,0xCD ,0x25 ,0xFC ,0x66 ,0x66 ,
					   };
#endif
	// Initialize the hardware interface.
	// Depending on which interface you have linked the
	// library to, it initializes SWI UART, SWI GPIO, or TWI.
	sha204p_init();

	while (1) 
	{
		printf("\r\n=====TempKey Mac===== \r\n");
		// The following code sequence wakes up the device,
		// issues a MAC command in mode 0
		// using the Command Marshaling layer, and puts
		// the device to sleep.
		printf("1\r\n");
		for (i = 0; i < sizeof(response); i++)
			response[i] = 0;
		printf("2\r\n");
		// Wake up the device.
		ret_code = sha204c_wakeup(&response[0]);
		if (ret_code != SHA204_SUCCESS) 
		{
 	        printf("WakeUp	FAILED \r\n");
	        continue ;
		} 
		else {
			printf("WakeUp	Ok \r\n");
		}
//=============================================================
// **********************************************************************************
// **  STEP-1  Start with a Random NONCE command. The output of the NONCE command
// **          is a Random Number used on the calculations. The NONCE number is then
// **          stored inside the SHA204 TempKey
// **********************************************************************************
		// This is a value used for the NONCE command NumIn parameter, 20 bytes long
	 uint8_t *NumIn = (uint8_t *) "_HereIsTheChallenge_";

	ret_code = sha204m_nonce(&command[0],&rand_out[0],NONCE_MODE_SEED_UPDATE,NumIn);
		
	if (ret_code != SHA204_SUCCESS) 
	{
		printf("NONCE	FAILED \r\n");
		sha204p_sleep();
		continue ;
	} 
	else {
	   printf("NONCE	Ok \r\n");
	   printf("Random Out \r\n");		   
	   //ShowBuffer(&rand_out[1], 32);  // Display data in terminal	
	   for(num=0;num<32;num++)
		{

			printf("0x%02x ",rand_out[num+1]);
		}
	   printf("\r\n");	
	}
				
// **********************************************************************************
// **  STEP-2  In this case, the MAC command combines the value previously 
// **          calculated by the NONCE command and stored inside the SHA204 TempKey,
// **          and one of the Key-id/SlotId, and other data (see datasheet) to 
// **          generate the final SHA256 digest.
// **          The output of the MAC command is the Response used to compare later ..
// **********************************************************************************
#if 1
    ret_code = sha204m_mac(&command[0],&mac[0],MAC_MODE_BLOCK2_TEMPKEY,KEY_ID,NULL);

	if (ret_code != SHA204_SUCCESS) 
	{
		sha204p_sleep();
		printf("MAC	FAILED \r\n");
	} else {
		printf("MAC	Ok \r\n");
 	}
#endif
// **********************************************************************************
// **  STEP-3  Next, we need to emulate the calculations inside the SHA204 using
// **          the functions from the helper library. 
// **          Using the sha204h_nonce() and the Random number generated before
// **          stored in rand_out, we can replicate the intermediate value of
// **          TempKey
// **********************************************************************************
#if 1
	args_nonce_tempkey.mode = NONCE_MODE_SEED_UPDATE;
	args_nonce_tempkey.num_in = (uint8_t *) NumIn;
	args_nonce_tempkey.rand_out = &rand_out[SHA204_BUFFER_POS_DATA];
	args_nonce_tempkey.temp_key = &temp_key;

	ret_code = sha204h_nonce(&args_nonce_tempkey);

	if (ret_code != SHA204_SUCCESS) 
	{
		printf("Emulated NONCE	FAILED \r\n");
		sha204p_sleep();
		continue ;
    }  
	else {
	    printf("Emulated NONCE	Ok \r\n");
    }

#endif

// **********************************************************************************
// **  STEP-4    Emulate a MAC Command using sha204h_mac()
// **            The output of this command is the "Response" used for the
// **            Random Challenge-Response
// **            Value stored in "mac_calculated"
// **********************************************************************************

#if 1
  // This is the value already stored on the "Training" SHA204 on Slot/Key 15, -->> change KEY_ID = 15 <<--
   //   const uint8_t *key = (uint8_t *) "CryptoAuthentication!!!!!!!!!!!!"; // 32 bytes long, Key Slot
  // 32 bytes long, Key Slot
  // The calculated key, is stored on the "Training" SHA204 on Slot 14,  -->> change KEY_ID = 14 <<--
  //  uint8_t key[32];
  //  ObscureKeyComputation(key);

     args_mac_tempkey.mode = MAC_MODE_BLOCK2_TEMPKEY;
 	 //args_mac_tempkey.mode = 0;
     args_mac_tempkey.key_id = KEY_ID;
     args_mac_tempkey.challenge = NULL;
	 //args_mac_tempkey.challenge = challenge;
     args_mac_tempkey.key = (uint8_t *) __key;
     args_mac_tempkey.otp = NULL;
     args_mac_tempkey.sn = NULL;
     args_mac_tempkey.response = mac_calculated;
     args_mac_tempkey.temp_key = &temp_key;

 // Emulate MAC.
    ret_code = sha204h_mac(&args_mac_tempkey);

	
    if (ret_code != SHA204_SUCCESS) {
         printf("Emulated MAC	FAILED \r\n");
		 sha204p_sleep();
         continue ;
    } else {
	    printf("Emulated MAC	Ok \r\n");	
    }
#endif
	
// **********************************************************************************
// **  STEP-5  Compare the MACS's to check inauthenticity 
// **********************************************************************************

   // Compare the Mac response with the calculated one.
   // Here we can use memcmp since a timing attack is not possible due to the injection of a random nonce.
   ret_code = (memcmp(&mac[SHA204_BUFFER_POS_DATA], mac_calculated, sizeof(mac_calculated)) ? SHA204_FUNC_FAIL : SHA204_SUCCESS);

  // Display results in terminal
   
	printf("Received Response:\r\n");
	//ShowBuffer(&mac[1], 32);  // Display data in terminal
	for(num=0;num<32;num++)
	{
		printf("0x%02x ",mac[num+1]);
	}	

	printf("\r\n");	
	printf("Calculated Response:\r\n");
	for(num=0;num<32;num++)
	{
		printf("0x%02x ",mac_calculated[num]);
	}		
	printf("\r\n");	
	printf("===============================\r\n");

	printf("\r\n");
	if (ret_code == SHA204_SUCCESS) {
			printf("Random Challenge/Response test --SUCCEEDED-- \r\n");
			sha204p_sleep();
			break;
	} else {
			printf("Random Challenge/Response test **FAILED** \r\n");
	}

	sha204p_sleep();
	mdelay(1000);
	}
	return (int) ret_code;
}

//************************************
// Method:    atsha204_mac
// FullName:  atsha204_mac : Performs a Nonce, sends a host challenge, receives and validates the response.
// Access:    public 
// Returns:   uint8_t
// Qualifier:
// Parameter: uint16_t key_id
//					The ATSHA204 key ID for the key to use in this MAC operation
// Parameter: uint8_t key[32]
//					The actual key value.  An authentic host system will know this value.
//					Because the host may not be secure, unprotected storage of this key becomes a source of vulnerability.  Atmel
//					advices operation in a secure MCU or use of another ATSHA204 device in a host system to protect the key AND
//					also securely perform host operations.
//
// Parameter: uint8_t NumIn[NONCE_NUMIN_SIZE_PASSTHROUGH]
//					This is a 20-byte or 32-byte NumIn parameter of the Nonce command.  It is advisable to make this a system
//					provided varying value.
//************************************
uint8_t atsha204_mac(uint16_t key_id,uint8_t* secret_key, uint8_t* NumIn, uint8_t* challenge) 
{
	static uint8_t sha204_lib_return = SHA204_SUCCESS;			//!< Function execution status, initialized to SUCCES and bitmasked with error codes as needed.
	uint8_t transmit_buffer[SHA204_CMD_SIZE_MAX];	//!< Transmit data buffer
	uint8_t response_buffer[SHA204_RSP_SIZE_MAX];	//!< Receive data buffer
	uint8_t wakeup_response_buffer[SHA204_RSP_SIZE_MIN] = {0};	
	uint8_t soft_digest [32];						//!< Software calculated digest
	struct sha204h_nonce_in_out nonce_param;		//!< Parameter for nonce helper function
	struct sha204h_mac_in_out mac_param;			//!< Parameter for mac helper function
	struct sha204h_temp_key tempkey;				//!< tempkey parameter for nonce and mac helper function
	
	sha204p_init();
	//-----------tony comment:MCU side operate------
	//tony comment: MAC step 1	
	// Wake the device, validate its presence and put it back to sleep.
	sha204_lib_return |= sha204c_wakeup(wakeup_response_buffer);
	if(SHA204_SUCCESS!=sha204_lib_return)
	{
		return sha204_lib_return;
	}

	//-----------tony comment:ATSHA204 side operate------
	//tony comment: MAC step 2-----ATSH204 side calulate tempkey and return RN(Random Number)
	// Execute the nonce command - precedes all MAC commands.
	sha204_lib_return |= sha204m_nonce(transmit_buffer, response_buffer, NONCE_MODE_NO_SEED_UPDATE, NumIn);
	if(SHA204_SUCCESS!=sha204_lib_return)
	{
		return sha204_lib_return;
	}
printf("1\n");

	//-----------tony comment:MCU side operate------
	//tony comment: MAC step 3-----MCU side calculate tempkey
	// Initialize parameter for helper function
	// Initialize parameter for helper function
	nonce_param.mode = NONCE_MODE_NO_SEED_UPDATE;
	nonce_param.num_in = NumIn;	
	nonce_param.rand_out = &response_buffer[1];	
	nonce_param.temp_key = &tempkey;
	sha204_lib_return |= sha204h_nonce(&nonce_param);
printf("2\n");
	
	//-----------tony comment:ATSHA204 side operate------
	//tony comment: MAC step 4-----ATSHA204 MAC
	// Execute the MAC command which constitutes sending a challenge. Successful execution will yield a result that contains the "Challenge Response" to be validated later in this function.
	sha204_lib_return |= sha204m_mac(transmit_buffer, response_buffer, MAC_MODE_BLOCK2_TEMPKEY, key_id, challenge);
	if(SHA204_SUCCESS!=sha204_lib_return)
	{
		return sha204_lib_return;
	}
printf("3\n");
	
	//-----------tony comment:MCU side operate------
	//tony comment: MAC step 5-----MCU MAC
	// Collect required information needed by a host system to calculate the expected challenge response in software, then perform the calculation.
	//mac_param.mode = MAC_MODE_BLOCK1_TEMPKEY|MAC_MODE_BLOCK2_TEMPKEY;

	mac_param.mode = MAC_MODE_BLOCK2_TEMPKEY;
	mac_param.key_id = key_id;
	mac_param.challenge = challenge;
	mac_param.key = secret_key;
	mac_param.otp = NULL;
	mac_param.sn = NULL;
	mac_param.response = soft_digest;
	mac_param.temp_key = &tempkey;
	sha204_lib_return |= sha204h_mac(&mac_param);
	
	//-----------tony comment:ATSHA204 side operate------
	//tony comment: MAC step 6-----MCU MAC
	// Send Sleep command.
	sha204_lib_return |= sha204p_sleep();	
	
	// Moment of truth!  Compare the chip generated digest found in 'response_buffer' with the host software calculated digest found in 'soft_digest'.
	sha204_lib_return |= memcmp(soft_digest,&response_buffer[1],32);

	return sha204_lib_return;
}



/*******************************************************************************
功           能: 认证加密芯片
参           数:	pKey:
返  回   值:0=成功, -1=失败, -2=芯片不存在,-3=其他
*********************************************************************************/
int AuthEncryptChip(unsigned char* pKey)
{
	uint8_t sha204_lib_return = SHA204_SUCCESS;			//!< Function execution status, initialized to SUCCES and bitmasked with error codes as needed. 
	uint8_t secret_key_id = KEY_ID_0;
//	uint8_t secret_key[32] = {0x00};
	uint8_t num_in[32] = {0};
	uint8_t challenge[32] = {0};

	//Authentication
	printf("--Test ATSHA204 function:Authentication!\r\n");
	sha204_lib_return |= atsha204_mac(secret_key_id, /*secret_key*/pKey, num_in, challenge);
	if(SHA204_SUCCESS!=sha204_lib_return)
	{
		printf("----Authentication: FAILED!!\r\n");
		return -1;
	}
	printf("----Authentication: SUCCESS!!\r\n");

	return 0;
}
