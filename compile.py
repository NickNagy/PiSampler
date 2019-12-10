import os

os.system('gcc gpio_ctrl.c -I/opt/vc/include -L/opt/vc/lib -lbcm_host -o main')
