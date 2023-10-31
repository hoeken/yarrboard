#/usr/bin/python3

import argparse, os, json

boards = ["8CH_MOSFET_REVB", "RGB_INPUT_REVA"]

if __name__ == '__main__':
	parser = argparse.ArgumentParser(description='Make a new Yarrboard firmware release')
	parser.add_argument('-v', '--version', help='Firmware Version', default=None)
	parser.add_argument('-c', '--changelog', help='Changelog', default=None)

	args = parser.parse_args()

	if (args.version):
		v = args.version
		config = []

		print (f'Making firmware release for version {v}')

		for board in boards:
			bdata = {}
			bdata['type'] = board
			bdata['version'] = v
			bdata['host'] = "raw.githubusercontent.com"
			bdata['port'] = 443
			bdata['url'] = f'/hoeken/yarrboard/main/firmware/releases/{board}-{v}.bin'
			if args.changelog:
				bdata['changelog'] = args.changelog
			config.append(bdata)

			print (f'Building {board} firmware')
			
			cmd = f'pio run -e "{board}" -s'
			#print (cmd)
			#os.system(cmd)

			cmd = f'cp .pio/build/{board}/firmware.bin releases/{board}-{v}.bin'
			#print (cmd)
			#os.system(cmd)

		#update our config json file
		config_str = json.dumps(config, indent=4)
		#print(config_str)
		with open("firmware.json", "w") as text_file:
		    text_file.write(config_str)

	else:
		print("You must supply a version number in x.y.z format")