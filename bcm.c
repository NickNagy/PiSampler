#include "bcm.h"

unsigned get_bcm_base() {
		unsigned address = ~0;
		FILE *fp = fopen("/proc/device-tree/soc/ranges", "rb");
		if (fp) {
			unsigned char buf[4];
			fseek(fp, 8, SEEK_SET);
			if (fread(buf, 1, sizeof(buf), fp) == sizeof(buf)) {
				address = buf[0] << 24 | buf[1] << 16 | buf [2] << 8 | buf [3];
			}
			fclose(fp);
		}
		return address;
}
