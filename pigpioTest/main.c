#include <stdio.h>
#include <stdlib.h>
#include <pigpio.h>

int main() {
	gpioInitialise();
	gpioHardwareClock(4, 100000);
	printf("Mode of clock 4 = %d\n", gpioGetMode(4));
	gpioTerminate();
	return 0;
}
