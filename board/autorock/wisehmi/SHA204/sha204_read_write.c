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

#define KEY_ID 15

/** \brief This function serves as an example for
 *         the SHA204 MAC command.
 *
 *         In an infinite loop, it issues the same command
 *         sequence using the Command Marshaling layer of
 *         the SHA204 library.
 * @return exit status of application
 */
//返回0表示正确，其它值错误
//sha204读配置       zone:区域       address:地址        _data:读取的数据
int sha204_read(unsigned char zone,unsigned int address,unsigned char _data[32])
{
	// declared as "volatile" for easier debugging
	volatile uint8_t ret_code;
	
	uint8_t num;

	// Make the command buffer the size of the MAC command.
	 uint8_t command[MAC_COUNT_LONG];

   // expected MAC response in mode 0

	uint8_t challenge[35];
	 
	// Initialize the hardware interface.
	// Depending on which interface you have linked the
	// library to, it initializes SWI UART, SWI GPIO, or TWI.
	sha204p_init();

	printf("\r\n=====read Configuration/Data Zone===== \r\n");

	// Wake up the device.
	ret_code = sha204c_wakeup(&challenge[0]);
	if (ret_code != SHA204_SUCCESS) 
	{
		printf("WakeUp	FAILED \r\n");
		return  ret_code;
	} 
	else {
		printf("WakeUp	Ok \r\n");
	}

	ret_code = sha204m_execute(SHA204_READ, zone, address, 0, NULL,
					0, NULL, 0, NULL, sizeof(command), &command[0], sizeof(challenge), &challenge[0]);

	if (ret_code != SHA204_SUCCESS) 
	{
		sha204p_sleep();
		printf("SHA204_READ	FAILED \r\n");
	} else {
		printf("SHA204_READ	Ok \r\n");
 	}
	for(num=0;num<32;num++)
	{
		_data[num]=challenge[num+1];
		printf("0x%02x ",challenge[num+1]);
	}

	printf("\r\n");
	sha204p_sleep();

	return (int) ret_code;
}

//返回0表示正确，其它值错误
//sha204写配置         zone:区域       address:地址        _data:写入的数据
int sha204_write(unsigned char zone,unsigned int address,unsigned char _data[32])
{
	// declared as "volatile" for easier debugging
	volatile uint8_t ret_code;
	
	uint8_t num;

	// Make the command buffer the size of the MAC command.
	 uint8_t command[MAC_COUNT_LONG];

   // expected MAC response in mode 0

	uint8_t challenge[35];

	// Initialize the hardware interface.
	// Depending on which interface you have linked the
	// library to, it initializes SWI UART, SWI GPIO, or TWI.
	sha204p_init();

	printf("\r\n=====write Configuration/Data Zone===== \r\n");

	// Wake up the device.
	ret_code = sha204c_wakeup(&challenge[0]);
	if (ret_code != SHA204_SUCCESS) 
	{
	        printf("WakeUp	FAILED \r\n");
		return  ret_code;
	} 
	else {
		printf("WakeUp	Ok \r\n");
	}
		
	num=(zone&0x80)>0?32:4;
	
	ret_code = sha204m_execute(SHA204_WRITE, zone, address, num, &_data[0],
					0, NULL, 0, NULL, sizeof(command), &command[0], sizeof(challenge), &challenge[0]);

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

	printf("\r\n");
	sha204p_sleep();

	return (int) ret_code;
}

//返回0表示正确，其它值错误
//sha204上锁            zone:区域
uint8_t sha204_lock(unsigned char zone)
{
// declared as "volatile" for easier debugging
	volatile uint8_t ret_code;
	
	uint8_t num;

	// Make the command buffer the size of the MAC command.
	 uint8_t command[MAC_COUNT_LONG];

   // expected MAC response in mode 0

	uint8_t challenge[35];

	// Initialize the hardware interface.
	// Depending on which interface you have linked the
	// library to, it initializes SWI UART, SWI GPIO, or TWI.
	sha204p_init();

	printf("\r\n=====lock Configuration/Data Zone===== \r\n");

	// Wake up the device.
	ret_code = sha204c_wakeup(&challenge[0]);
	if (ret_code != SHA204_SUCCESS) 
	{
	        printf("WakeUp	FAILED \r\n");
        return  ret_code;
	} 
	else {
		printf("WakeUp	Ok \r\n");
	}
		
	//ret_code=sha204m_lock(command,challenge,zone,0);
	ret_code = sha204m_execute(SHA204_LOCK, 0x80|zone, 0x00, 0, NULL,
					 0, NULL, 0, NULL, sizeof(command), &command[0], sizeof(challenge), &challenge[0]);
	if (ret_code != SHA204_SUCCESS) 
	{
	        printf("SHA204_LOCK	FAILED \r\n");
	} 
	else {
		printf("SHA204_LOCK	Ok \r\n");
	}
	
	sha204p_sleep();
	return ret_code;
}
