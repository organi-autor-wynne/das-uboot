//*******************************************************************************************
//Copyright (C),2010-2017,TONYZHAO,All Rights Reserved
// Project name:		    test_d21_atsha204a_hw_i2c/test_d21_atsha204a_sw_i2c
// File name:				atsha204_enc_read.c
// Author:					TonyZhao
// Version:					V1.0
// Created date:			2014-11-16
// Modified date:			2014-11-xx
// Compiler/IDE:			ATMEL sutdio6.2sp1
// Description:
// hardware:				ATSAMD21-XPRO
// Target:					ATSAMSD21J18A-AU
// Frequence:				48.0000Mhz
// Optimization:			-O1
//******************************************************************************************

#include <linux/string.h>

#include "sha204_lib_return_codes.h"
#include "sha204_comm_marshaling.h"
#include "atsha204_defines.h"
//#include "atsha204_device_configuration.h"

#include "sha204_helper.h"

uint8_t atsha204_enc_read(uint16_t read_slot_address, uint8_t* clear_read_data, uint16_t read_key_slot_id, uint8_t* secret_read_key, uint8_t* NumIn) 
{
	static uint8_t sha204_lib_return = SHA204_SUCCESS;			//!< Function execution status, initialized to SUCCES and bitmasked with error codes as needed.
	uint8_t transmit_buffer[SHA204_CMD_SIZE_MAX] = {0};	//!< Transmit data buffer
	uint8_t response_buffer[SHA204_RSP_SIZE_MAX] = {0};	//!< Receive data buffer
	uint8_t wakeup_response_buffer[SHA204_RSP_SIZE_MIN] = {0};	
	uint8_t dec_data[32] = {0};							//!< Buffer to hold the hostdecrypted data
	struct sha204h_nonce_in_out mcu_nonce_param;		//!< Parameter for nonce helper function
	struct sha204h_gen_dig_in_out mcu_gendig_param;		//!< Parameter for gendig helper function
	struct sha204h_temp_key mcu_tempkey;				//!< Tempkey parameter for nonce and mac helper function
	struct sha204h_decrypt_in_out mcu_dec_param;		//!< Parameter for decrypt helper function
	int num;
	
	sha204p_init();

	//tony comment: encrypt read step 1
	// Wake the device, validate its presence and put it back to sleep.
	sha204_lib_return |= sha204c_wakeup(wakeup_response_buffer);
	if(SHA204_SUCCESS!=sha204_lib_return)
	{
		return sha204_lib_return;
	}

	//-----------tony comment:ATSHA204 side operate------
	//tony comment: encrypt read step 2-----ATSH204 side calulate tempkey and return RN(Random Number)	
	// Execute the nonce command - validates TempKey flag.
	sha204_lib_return |= sha204m_nonce(transmit_buffer, response_buffer, NONCE_MODE_NO_SEED_UPDATE, NumIn);	
	if(SHA204_SUCCESS!=sha204_lib_return)
	{
		return sha204_lib_return;
	}
	
	//-----------tony comment:MCU side operate------
	//tony comment: encrypt read step 3-----MCU side calculate tempkey
	// Initialize parameter for helper function
	// Prepare parameters and nonce in host.
	mcu_nonce_param.mode = NONCE_MODE_NO_SEED_UPDATE;
	mcu_nonce_param.num_in = NumIn;	
	mcu_nonce_param.rand_out = &response_buffer[1];	
	mcu_nonce_param.temp_key = &mcu_tempkey;
	sha204_lib_return |= sha204h_nonce(&mcu_nonce_param);
printf("r1\n");
	//-----------tony comment:ATSHA204 side operate------
	//tony comment: encrypt read step 4-----ATSH204 side claulate tempkey useing tempkey and key[key_id] as input for sha256 	
	// Execute GenDig command in device to prepare TempKey
	sha204_lib_return |= sha204m_gen_dig(transmit_buffer, response_buffer,GENDIG_ZONE_DATA, read_key_slot_id, NULL);	
	if(SHA204_SUCCESS!=sha204_lib_return)
	{
		return sha204_lib_return;
	}
printf("r2\n");	
	//-----------tony comment:MCU side operate------
	//tony comment: encrypt read step 5-----MCU side calculate tempkey useing tempkey and key[key_id] as input for sha256 
	// Prepare host software to compute equivalent GenDig information
	// Initialize parameter for helper function
	mcu_gendig_param.zone = GENDIG_ZONE_DATA;
	mcu_gendig_param.key_id = read_key_slot_id;
	mcu_gendig_param.stored_value = secret_read_key;
	mcu_gendig_param.temp_key = &mcu_tempkey;

	sha204_lib_return |= sha204h_gen_dig(&mcu_gendig_param);
printf("r3\n");
	//-----------tony comment:SHA204 side operate------
	//tony comment: encrypt read step 6-----
	sha204_lib_return |= sha204m_read(transmit_buffer, response_buffer, SHA204_ZONE_DATA|SHA204_ZONE_COUNT_FLAG, 4 * read_slot_address);//read 32 bytes	
	if(SHA204_SUCCESS!=sha204_lib_return)
	{
		return sha204_lib_return;
	}
printf("r4\n");
	//tony comment: encrypt read step 7	
	// Capture the read data and store in buffers for decryption and reference
	memcpy(dec_data, &response_buffer[1], 32);

	//tony comment: encrypt read step 8
	// Prepare host helper parameter and decrypt the data
	mcu_dec_param.crypto_data = dec_data;
	mcu_dec_param.temp_key = &mcu_tempkey;
	sha204_lib_return |= sha204h_decrypt(&mcu_dec_param);	
	
	//tony comment: encrypt read step 9	
	memcpy(clear_read_data,dec_data, 32);	
	
	for(num=0;num<32;num++)
	{
		printf("0x%02x ",clear_read_data[num]);
	}
	printf("\r\n");

	//tony comment: encrypt read step 10
	sha204p_sleep();

	return sha204_lib_return;
}

