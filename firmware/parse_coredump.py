#!/usr/bin/env python

import argparse, os

if __name__ == '__main__':

	parser = argparse.ArgumentParser()
	parser.add_argument("version", help="Version of the firmware, eg. 1.2.3")
	parser.add_argument("board", help="Hardware board revision, eg. RGB_INPUT_REV_A")

	args = parser.parse_args()

	#we need a coredump file
	if not os.path.isfile("coredump.txt"):
		print("Error: coredump.txt not found")
	else:
		#convert to binary format
		print("Converting coredump to binary format.")
		cmd = f'xxd -r -p coredump.txt coredump.bin'
		os.system(cmd)
		
		#keep our ELF file for debugging later on....
		print("Analyzing coredump")
		cmd = f'. /home/hoeken/esp/esp-idf/export.sh && espcoredump.py info_corefile -c coredump.bin -t raw releases/{args.board}-{args.version}.elf'
		os.system(cmd)