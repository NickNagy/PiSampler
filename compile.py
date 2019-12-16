import os
import sys

if len(sys.argv) > 1 and sys.argv[1]=='--debug':
	print "compiled for GDB."
	os.system('g++ gpio_ctrl.c -I/opt/vc/include -L/opt/vc/lib -lbcm_host -g -o main')
else:
	os.system('gcc gpio_ctrl.c -I/opt/vc/include -L/opt/vc/lib -lbcm_host -o main')
