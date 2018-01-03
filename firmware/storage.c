//////////////////////////////////////////////////////////////
//
//       ///// //   //          /////    /////  /////
//     //     //   //         //    // //      //   //
//    //      // //    //    //    // //      //   //
//   //      // //   ////   //    // //      //   //
//   /////   ///     //     //////   /////  //////
//
// CV.OCD MIDI-TO-CV CONVERTER
// hotchk155/2016
// Sixty Four Pixels Limited
//
// This work is distibuted under terms of Creative Commons 
// License BY-NC-SA (Attribution, Non-commercial, Share-Alike)
// https://creativecommons.org/licenses/by-nc-sa/4.0/
//
//////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
//
// PATCH STORAGE MODULE
//
//////////////////////////////////////////////////////////////

//
// HEADER FILES
//
#include <system.h>
#include <rand.h>
#include <eeprom.h>
#include "cvocd.h"

//
// LOCAL DATA
//

#define MAGIC_COOKIE 0xA9

//
// LOCAL FUNCTIONS
//

////////////////////////////////////////////////////
// SAVE DATA TO EEPROM
static void storage_write(byte *data, int len, int* addr)
{
	while(len > 0) {
		eeprom_write(*addr, *data);
		++(*addr);
		++data;
		--len;
	}
}

////////////////////////////////////////////////////
// READ DATA FROM EEPROM
static void storage_read(byte *data, int len, int* addr)
{
	while(len > 0) {
		*data = eeprom_read(*addr);
		++(*addr);
		++data;
		--len;
	}
}

////////////////////////////////////////////////////
// SAVE ALL CONFIG TO EEPROM
void storage_write_patch()
{
	int len = 0;
	eeprom_write(0, MAGIC_COOKIE);
	int storage_ofs = 1;
	storage_write(global_storage(&len), len, &storage_ofs);
	storage_write(stack_storage(&len), len, &storage_ofs);
	storage_write(cv_storage(&len), len, &storage_ofs);
	storage_write(gate_storage(&len), len, &storage_ofs);
}

////////////////////////////////////////////////////
// READ CONFIG FRM EEPROM
void storage_read_patch()
{
	if(eeprom_read(0) != MAGIC_COOKIE) {
		return;
	}
	int len = 0;
	int storage_ofs = 1;
	storage_read(global_storage(&len), len, &storage_ofs);
	storage_read(stack_storage(&len), len, &storage_ofs);
	storage_read(cv_storage(&len), len, &storage_ofs);
	storage_read(gate_storage(&len), len, &storage_ofs);
}

//
// END
// 