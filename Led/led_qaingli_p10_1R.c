#if !defined(QIANGLI_UNIT_X_NUM)
# error "Must define QIANGLI_UNIT_X_NUM"
#endif


#include "QIANGLI_P10_1R_TABLE.c"
void LedDisplayToScan(int x, int y, int xend, int yend) {
	int xv;
	unsigned int *src;
	const unsigned int *dest;
	for (; y <= yend; ++y) {
		src = (unsigned int *)(__displayBufferBit + (y * LED_VIR_DOT_WIDTH + x) * 4);
		dest = &__addrTransferTable[y][x];
		for (xv = x; xv <= xend; ++xv) {
#if LED_DRIVER_LEVEL==0
			*(unsigned int *)(__scanBufferBit + *dest++) = !*src++;
#elif LED_DRIVER_LEVEL==1
			*(unsigned int *)(__scanBufferBit + *dest++) = *src++;
#else
#error "LED_DRIVER_LEVEL MUST be 0 or 1"
#endif
		}
	}
}

void LedScrollDisplayToScan(int dispX, int dispY, int scanX, int scanY) {
	unsigned int *src;
	const unsigned int *dest;
	int vDispX, vScanX;

	for (; dispY <= LED_VIR_DOT_HEIGHT*3/2 && scanY < LED_PHY_DOT_HEIGHT; ++dispY, ++scanY) {
		src = (unsigned int *)(__displayBufferBit + (dispY * LED_VIR_DOT_WIDTH + dispX) * 4);
		dest = &__addrTransferTable[scanY][scanX];
		for (vDispX = dispX, vScanX = scanX; vDispX < LED_VIR_DOT_WIDTH && vScanX < LED_PHY_DOT_WIDTH;	++vDispX, ++vScanX) {
#if LED_DRIVER_LEVEL==0
			*(unsigned int *)(__scanBufferBit + *dest++) = !*src++;
#elif LED_DRIVER_LEVEL==1
			*(unsigned int *)(__scanBufferBit + *dest++) = *src++;
#else
#error "LED_DRIVER_LEVEL MUST be 0 or 1"
#endif
		}
	}
}
