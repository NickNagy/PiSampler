import os
import sys

os.system("cd ..")
if len(sys.argv) > 1 and sys.argv[1]=='--debug':
	print "compiled for GDB."
	os.system('g++ bcm/gpio.c bcm/clk.c bcm/pcm.c bcm/pimem.c pmod.c main.c -I/opt/vc/include -L/opt/vc/lib -lbcm_host -g -o main')
else:
	os.system('gcc bcm/gpio.c bcm/clk.c bcm/pcm.c bcm/pimem.c pmod.c main.c -I/opt/vc/include -L/opt/vc/lib -lbcm_host -o main')
