#/usr/bin/python3

import argparse, os, json, re

boards = ["8CH_MOSFET_REV_B", "RGB_INPUT_REV_A"]

if __name__ == '__main__':

	#look up our version #
	version = False
	file = open("src/config.h", "r")
	for line in file:
		result = re.search(r'YB_FIRMWARE_VERSION "(.*)"', line)
		if result:
			version = result.group(1)
			break

	#get our changelog
	with open("CHANGELOG") as f:
		changelog = f.read()

	#only proceed if we found the version
	if (version):
		config = []

		print (f'Making firmware release for version {version}')

		for board in boards:
			bdata = {}
			bdata['type'] = board
			bdata['version'] = version
			bdata['url'] = f'https://raw.githubusercontent.com/hoeken/yarrboard/main/firmware/releases/{board}-{version}.bin'
			bdata['changelog'] = changelog
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

			cmd = f'cp .pio/build/{board}/signed.bin releases/{board}-{version}.bin'
			#print (cmd)
			os.system(cmd)

		#update our config json file
		config_str = json.dumps(config, indent=4)
		#print(config_str)
		with open("firmware.json", "w") as text_file:
		    text_file.write(config_str)

	else:
		print("YB_FIRMWARE_VERSION not #defined in src/config.h")