#/usr/bin/python3

import argparse, os, json

boards = ["8CH_MOSFET_REV_B", "RGB_INPUT_REV_A"]

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
			bdata['url'] = f'https://raw.githubusercontent.com/hoeken/yarrboard/main/firmware/releases/{board}-{v}.bin'
			if args.changelog:
				bdata['changelog'] = args.changelog
			config.append(bdata)

			print (f'Building {board} firmware')
			
			cmd = f'pio run -e "{board}" -s'
			#print (cmd)
			os.system(cmd)

			cmd = f'openssl dgst -sign ~/Dropbox/misc/yarrboard/priv_key.pem -keyform PEM -sha256 -out .pio/build/{board}/firmware.sign -binary .pio/build/{board}/firmware.bin'
			#print (cmd)
			os.system(cmd)

			cmd = f'cat .pio/build/{board}/firmware.sign .pio/build/{board}/firmware.bin > .pio/build/{board}/signed.bin'
			#print (cmd)
			os.system(cmd)

			cmd = f'cp .pio/build/{board}/signed.bin releases/{board}-{v}.bin'
			#print (cmd)
			os.system(cmd)

		#update our config json file
		config_str = json.dumps(config, indent=4)
		#print(config_str)
		with open("firmware.json", "w") as text_file:
		    text_file.write(config_str)

	else:
		print("You must supply a version number in x.y.z format")