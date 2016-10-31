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

#define KEY_ID	15

/** \brief This function serves as an example for
 *         the SHA204 MAC command.
 *
 *         In an infinite loop, it issues the same command
 *         sequence using the Command Marshaling layer of
 *         the SHA204 library.
 * @return exit status of application
 */
//随机数认证
int rand_main(uint8_t *__key)
{
	// declared as "volatile" for easier debugging
	volatile uint8_t ret_code;
 	struct sha204h_temp_key temp_key;
	struct sha204h_mac_in_out args_mac_tempkey;
	 uint8_t mac_calculated[SHA204_KEY_SIZE];
	 uint8_t mac[MAC_RSP_SIZE];
	
	uint8_t i,num;

	// Make the command buffer the size of the MAC command.
	 uint8_t command[MAC_COUNT_LONG];

   // expected MAC response in mode 0

	uint8_t challenge[35]="12345678123456781234567812345678";
	 
	//uint8_t __key[32]="12345678123456781234567812345678";
	// Initialize the hardware interface.
	// Depending on which interface you have linked the
	// library to, it initializes SWI UART, SWI GPIO, or TWI.
	sha204p_init();

	while (1) 
	{
		printf("\r\n=====rand challenge===== \r\n");
		// The following code sequence wakes up the device,
		// issues a MAC command in mode 0
		// using the Command Marshaling layer, and puts
		// the device to sleep.
		for (i = 0; i < sizeof(mac); i++)
			mac[i] = 0;
		// Wake up the device.
		ret_code = sha204c_wakeup(&mac[0]);
		if (ret_code != SHA204_SUCCESS) 
		{
	 	        printf("WakeUp	FAILED \r\n");
		        continue ;
		} 
		else {
			printf("WakeUp	Ok \r\n");
		}
		// **********************************************************************************
		// **  STEP-1  产生一个随机数
		// **          calculated by the NONCE command and stored inside the SHA204 TempKey,
		// **          and one of the Key-id/SlotId, and other data (see datasheet) to 
		// **          generate the final SHA256 digest.
		// **          The output of the MAC command is the Response used to compare later ..
		// **********************************************************************************
		ret_code = sha204m_random(&command[0], &challenge[0], 0);
		/*
		ret_code = sha204m_execute(SHA204_WRITE, 0x82, 0x0078, 32, &challenge[0],
							0, NULL, 0, NULL, sizeof(command), &command[0], sizeof(challenge), &challenge[0]);

		ret_code = sha204m_execute(SHA204_READ, 0x82, 0x0078, 0, NULL,
							0, NULL, 0, NULL, sizeof(command), &command[0], sizeof(challenge), &challenge[0]);
		*/
		if (ret_code != SHA204_SUCCESS) 
		{
			sha204p_sleep();
			printf("challenge	FAILED \r\n");
		} else {
			printf("challenge	Ok \r\n");
	 	}
		for(num=0;num<32;num++)
		{
			printf("0x%02x ",challenge[num+1]);
		}
		// **********************************************************************************
		// **  STEP-2  获得MAC值，In this case, the MAC command combines the value previously 
		// **          calculated by the NONCE command and stored inside the SHA204 TempKey,
		// **          and one of the Key-id/SlotId, and other data (see datasheet) to 
		// **          generate the final SHA256 digest.
		// **          The output of the MAC command is the Response used to compare later ..
		// **********************************************************************************
#if 1
		ret_code = sha204m_execute(SHA204_MAC, 0, KEY_ID, MAC_CHALLENGE_SIZE, (uint8_t *) challenge,
						0, NULL, 0, NULL, sizeof(command), &command[0], sizeof(mac), &mac[0]);

		if (ret_code != SHA204_SUCCESS) 
		{
			sha204p_sleep();
			printf("MAC	FAILED \r\n");
		} else {
			printf("MAC	Ok \r\n");
	 	}
#endif
		// **********************************************************************************
		// **  STEP-3    计算理论MAC值，Emulate a MAC Command using sha204h_mac()
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

		//args_mac_tempkey.mode = MAC_MODE_BLOCK2_TEMPKEY;
		args_mac_tempkey.mode = 0;
		args_mac_tempkey.key_id = KEY_ID;
		//args_mac_tempkey.challenge = NULL;
		args_mac_tempkey.challenge = challenge;
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
		// **  STEP-4  比较理论与实际的MAC值是否完全相等，Compare the MACS's to check inauthenticity 
		// **********************************************************************************

		// Compare the Mac response with the calculated one.
		// Here we can use memcmp since a timing attack is not possible due to the injection of a random nonce.
		ret_code = (memcmp(&mac[SHA204_BUFFER_POS_DATA], mac_calculated, sizeof(mac_calculated)) ? SHA204_FUNC_FAIL : SHA204_SUCCESS);

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

		// Display results in terminal
		if (ret_code == SHA204_SUCCESS) {
			printf("Random Challenge/Response test --SUCCEEDED-- \r\n");
			sha204p_sleep();
				break;
		} else {
			printf("Random Challenge/Response test **FAILED** \r\n");
		}
		printf("\r\n");
		sha204p_sleep();

		mdelay(1000);
	}
	return (int) ret_code;
}
