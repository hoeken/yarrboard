#!/usr/bin/env python

import argparse, os, json, re

boards = [
	"8CH_MOSFET_REV_B",
#	"8CH_MOSFET_REV_C",
	"RGB_INPUT_REV_A",
#	"RGB_INPUT_REV_B"
]

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

			#keep our ELF file for debugging later on....
			cmd = f'cp .pio/build/{board}/firmware.elf releases/{board}-{version}.elf'
			#print (cmd)
			os.system(cmd)

		#update our config json file
		config_str = json.dumps(config, indent=4)
		#print(config_str)
		with open("firmware.json", "w") as text_file:
		    text_file.write(config_str)

		print("Build complete.\n")
		print("Next steps:")
		print(f'1. Add the new firmware files: git add releases')
		print(f'2. Commit the new version: git commit -am "Firmware release v{version}"')
		print(f'3. Push changes to github: git push')
		print(f'4. Create a new tag: git tag -a v{version} -m "Firmware release v{version}"')
		print(f'5. Push your tags: git push origin v{version}')
	else:
		print("YB_FIRMWARE_VERSION not #defined in src/config.h")