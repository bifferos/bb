/*
    i2cset.c - A user-space program to write an I2C register.
    Copyright (C) 2001-2003  Frodo Looijaard <frodol@dds.nl>, and
                             Mark D. Studebaker <mdsxyz123@yahoo.com>
    Copyright (C) 2004-2008  Jean Delvare <khali@linux-fr.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA.
*/


//usage:#define i2cset_trivial_usage
//usage:       "<i2cbus> <address> <data-address> [value <mode>]]"
//usage:#define i2cset_full_usage "\n\n"
//usage:       "Get i2c bus devices\n"
//usage:       "\n        <i2cbus>    Choose the bus to scan"
//usage:       "\n        <address>   Choose the chip address"
//usage:       "\n        <data-address>   Choose the data address"
//usage:       "\n        -f      ???"
//usage:       "\n        -y      ???"
//usage:       "\n"
//usage:       "\nValues"
//usage:#define i2cset_example_usage
//usage:       "# i2cset\n"
//usage:       "# i2cset 0 0 0\n"



#include "libbb.h"
#include "i2cbusses.h"
#include "i2c-dev.h"
#include "util.h"
#include "version.h"

static void help(void) __attribute__ ((noreturn));

static void help(void)
{
	fprintf(stderr,
	        "Usage: i2cset [-f] [-y] [-m MASK] I2CBUS CHIP-ADDRESS DATA-ADDRESS [VALUE [MODE]]\n"
		"  I2CBUS is an integer or an I2C bus name\n"
		"  ADDRESS is an integer (0x03 - 0x77)\n"
		"  MODE is one of:\n"
		"    b (byte, default)\n"
		"    w (word)\n"
		"    Append p for SMBus PEC\n");
	exit(1);
}

static int check_funcs(int file, int size, int pec)
{
	unsigned long funcs;

	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		fprintf(stderr, "Error: Could not get the adapter "
			"functionality matrix: %s\n", strerror(errno));
		return -1;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus send byte");
			return -1;
		}
		break;

	case I2C_SMBUS_BYTE_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus write byte");
			return -1;
		}
		break;

	case I2C_SMBUS_WORD_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_WORD_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus write word");
			return -1;
		}
		break;
	}

	if (pec
	 && !(funcs & (I2C_FUNC_SMBUS_PEC | I2C_FUNC_I2C))) {
		fprintf(stderr, "Warning: Adapter does "
			"not seem to support PEC\n");
	}

	return 0;
}

static int confirm(const char *filename, int address, int size, int daddress,
		   int value, int vmask, int pec)
{
	int dont = 0;

	fprintf(stderr, "WARNING! This program can confuse your I2C "
		"bus, cause data loss and worse!\n");

	if (address >= 0x50 && address <= 0x57) {
		fprintf(stderr, "DANGEROUS! Writing to a serial "
			"EEPROM on a memory DIMM\nmay render your "
			"memory USELESS and make your system "
			"UNBOOTABLE!\n");
		dont++;
	}

	fprintf(stderr, "I will write to device file %s, chip address "
		"0x%02x, data address\n0x%02x, ", filename, address, daddress);
	if (size == I2C_SMBUS_BYTE)
		fprintf(stderr, "no data.\n");
	else
		fprintf(stderr, "data 0x%02x%s, mode %s.\n", value,
			vmask ? " (masked)" : "",
			size == I2C_SMBUS_BYTE_DATA ? "byte" : "word");
	if (pec)
		fprintf(stderr, "PEC checking enabled.\n");

	fprintf(stderr, "Continue? [%s] ", dont ? "y/N" : "Y/n");
	fflush(stderr);
	if (!user_ack(!dont)) {
		fprintf(stderr, "Aborting on user request.\n");
		return 0;
	}

	return 1;
}


int i2cset_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int i2cset_main(int argc, char **argv )
{
	char *end;
	const char *maskp = NULL;
	int res, i2cbus, address, size, file;
	int value, daddress, vmask = 0;
	char filename[20];
	int pec = 0;
	int flags = 0;
	int force = 0, yes = 0, version = 0, readback = 0;

	/* handle (optional) flags first */
	while (1+flags < argc && argv[1+flags][0] == '-') {
		switch (argv[1+flags][1]) {
		case 'V': version = 1; break;
		case 'f': force = 1; break;
		case 'y': yes = 1; break;
		case 'm':
			if (2+flags < argc)
				maskp = argv[2+flags];
			flags++;
			break;
		case 'r': readback = 1; break;
		default:
			fprintf(stderr, "Error: Unsupported option "
				"\"%s\"!\n", argv[1+flags]);
			help();
			exit(1);
		}
		flags++;
	}

	if (version) {
		fprintf(stderr, "i2cset version %s\n", VERSION);
		exit(0);
	}

	if (argc < flags + 4)
		help();

	i2cbus = lookup_i2c_bus(argv[flags+1]);
	if (i2cbus < 0)
		help();

	address = parse_i2c_address(argv[flags+2]);
	if (address < 0)
		help();

	daddress = strtol(argv[flags+3], &end, 0);
	if (*end || daddress < 0 || daddress > 0xff) {
		fprintf(stderr, "Error: Data address invalid!\n");
		help();
	}

	if (argc > flags + 4) {
		size = I2C_SMBUS_BYTE_DATA;
		value = strtol(argv[flags+4], &end, 0);
		if (*end || value < 0) {
			fprintf(stderr, "Error: Data value invalid!\n");
			help();
		}
	} else {
		size = I2C_SMBUS_BYTE;
		value = -1;
	}

	if (argc > flags + 5) {
		switch (argv[flags+5][0]) {
		case 'b': size = I2C_SMBUS_BYTE_DATA; break;
		case 'w': size = I2C_SMBUS_WORD_DATA; break;
		default:
			fprintf(stderr, "Error: Invalid mode!\n");
			help();
		}
		pec = argv[flags+5][1] == 'p';
	}

	/* Old method to provide the value mask, deprecated and no longer
	   documented but still supported for compatibility */
	if (argc > flags + 6) {
		if (maskp) {
			fprintf(stderr, "Error: Data value mask provided twice!\n");
			help();
		}
		fprintf(stderr, "Warning: Using deprecated way to set the data value mask!\n");
		fprintf(stderr, "         Please switch to using -m.\n");
		maskp = argv[flags+6];
	}

	if (maskp) {
		vmask = strtol(maskp, &end, 0);
		if (*end || vmask == 0) {
			fprintf(stderr, "Error: Data value mask invalid!\n");
			help();
		}
	}

	if ((size == I2C_SMBUS_BYTE_DATA && value > 0xff)
	 || (size == I2C_SMBUS_WORD_DATA && value > 0xffff)) {
		fprintf(stderr, "Error: Data value out of range!\n");
		help();
	}

	file = open_i2c_dev(i2cbus, filename, 0);
	if (file < 0
	 || check_funcs(file, size, pec)
	 || set_slave_addr(file, address, force))
		exit(1);

	if (!yes && !confirm(filename, address, size, daddress,
			     value, vmask, pec))
		exit(0);

	if (vmask) {
		int oldvalue;

		switch (size) {
		case I2C_SMBUS_BYTE:
			oldvalue = i2c_smbus_read_byte(file);
			break;
		case I2C_SMBUS_WORD_DATA:
			oldvalue = i2c_smbus_read_word_data(file, daddress);
			break;
		default:
			oldvalue = i2c_smbus_read_byte_data(file, daddress);
		}

		if (oldvalue < 0) {
			fprintf(stderr, "Error: Failed to read old value\n");
			exit(1);
		}

		value = (value & vmask) | (oldvalue & ~vmask);

		if (!yes) {
			fprintf(stderr, "Old value 0x%0*x, write mask "
				"0x%0*x: Will write 0x%0*x to register "
				"0x%02x\n",
				size == I2C_SMBUS_WORD_DATA ? 4 : 2, oldvalue,
				size == I2C_SMBUS_WORD_DATA ? 4 : 2, vmask,
				size == I2C_SMBUS_WORD_DATA ? 4 : 2, value,
				daddress);

			fprintf(stderr, "Continue? [Y/n] ");
			fflush(stderr);
			if (!user_ack(1)) {
				fprintf(stderr, "Aborting on user request.\n");
				exit(0);
			}
		}
	}

	if (pec && ioctl(file, I2C_PEC, 1) < 0) {
		fprintf(stderr, "Error: Could not set PEC: %s\n",
			strerror(errno));
		close(file);
		exit(1);
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		res = i2c_smbus_write_byte(file, daddress);
		break;
	case I2C_SMBUS_WORD_DATA:
		res = i2c_smbus_write_word_data(file, daddress, value);
		break;
	default: /* I2C_SMBUS_BYTE_DATA */
		res = i2c_smbus_write_byte_data(file, daddress, value);
	}
	if (res < 0) {
		fprintf(stderr, "Error: Write failed\n");
		close(file);
		exit(1);
	}

	if (pec) {
		if (ioctl(file, I2C_PEC, 0) < 0) {
			fprintf(stderr, "Error: Could not clear PEC: %s\n",
				strerror(errno));
			close(file);
			exit(1);
		}
	}

	if (!readback) { /* We're done */
		close(file);
		exit(0);
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		res = i2c_smbus_read_byte(file);
		value = daddress;
		break;
	case I2C_SMBUS_WORD_DATA:
		res = i2c_smbus_read_word_data(file, daddress);
		break;
	default: /* I2C_SMBUS_BYTE_DATA */
		res = i2c_smbus_read_byte_data(file, daddress);
	}
	close(file);

	if (res < 0) {
		printf("Warning - readback failed\n");
	} else
	if (res != value) {
		printf("Warning - data mismatch - wrote "
		       "0x%0*x, read back 0x%0*x\n",
		       size == I2C_SMBUS_WORD_DATA ? 4 : 2, value,
		       size == I2C_SMBUS_WORD_DATA ? 4 : 2, res);
	} else {
		printf("Value 0x%0*x written, readback matched\n",
		       size == I2C_SMBUS_WORD_DATA ? 4 : 2, value);
	}

	exit(0);
}
