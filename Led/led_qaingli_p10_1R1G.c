#if !defined(QIANGLI_UNIT_X_NUM)
# error "Must define QIANGLI_UNIT_X_NUM"
#endif

#if QIANGLI_UNIT_X_NUM==1
/* QIANGLI XEND=032, YEND=016 */
/* A=x/8; B=3-y/4; C=x%8; offset=32*A+8*B+C; offset=offset*32; */
const unsigned short __addrTransferTable[16][32] = {
	{
		/* ================ y = 00,01,02,03 ===================== */
		/*x=000, A=00, B=03, C=00 */
		0x0300, 0x0320, 0x0340, 0x0360, 0x0380, 0x03A0, 0x03C0, 0x03E0, 0x0700, 0x0720, 0x0740, 0x0760, 0x0780, 0x07A0, 0x07C0, 0x07E0,
		/*x=016, A=02, B=03, C=00 */
		0x0B00, 0x0B20, 0x0B40, 0x0B60, 0x0B80, 0x0BA0, 0x0BC0, 0x0BE0, 0x0F00, 0x0F20, 0x0F40, 0x0F60, 0x0F80, 0x0FA0, 0x0FC0, 0x0FE0,
	},
	{
		/* ================ y = 04,05,06,07 ===================== */
		/*x=000, A=00, B=02, C=00 */
		0x0200, 0x0220, 0x0240, 0x0260, 0x0280, 0x02A0, 0x02C0, 0x02E0, 0x0600, 0x0620, 0x0640, 0x0660, 0x0680, 0x06A0, 0x06C0, 0x06E0,
		/*x=016, A=02, B=02, C=00 */
		0x0A00, 0x0A20, 0x0A40, 0x0A60, 0x0A80, 0x0AA0, 0x0AC0, 0x0AE0, 0x0E00, 0x0E20, 0x0E40, 0x0E60, 0x0E80, 0x0EA0, 0x0EC0, 0x0EE0,
	},
	{
		/* ================ y = 08,09,10,11 ===================== */
		/*x=000, A=00, B=01, C=00 */
		0x0100, 0x0120, 0x0140, 0x0160, 0x0180, 0x01A0, 0x01C0, 0x01E0, 0x0500, 0x0520, 0x0540, 0x0560, 0x0580, 0x05A0, 0x05C0, 0x05E0,
		/*x=016, A=02, B=01, C=00 */
		0x0900, 0x0920, 0x0940, 0x0960, 0x0980, 0x09A0, 0x09C0, 0x09E0, 0x0D00, 0x0D20, 0x0D40, 0x0D60, 0x0D80, 0x0DA0, 0x0DC0, 0x0DE0,
	},
	{
		/* ================ y = 12,13,14,15 ===================== */
		/*x=000, A=00, B=00, C=00 */
		0x0000, 0x0020, 0x0040, 0x0060, 0x0080, 0x00A0, 0x00C0, 0x00E0, 0x0400, 0x0420, 0x0440, 0x0460, 0x0480, 0x04A0, 0x04C0, 0x04E0,
		/*x=016, A=02, B=00, C=00 */
		0x0800, 0x0820, 0x0840, 0x0860, 0x0880, 0x08A0, 0x08C0, 0x08E0, 0x0C00, 0x0C20, 0x0C40, 0x0C60, 0x0C80, 0x0CA0, 0x0CC0, 0x0CE0,
	},
};
#endif

#if QIANGLI_UNIT_X_NUM==12
/* QIANGLI XEND=384, YEND=016 */
/* A=x/8; B=3-y/4; C=x%8; offset=32*A+8*B+C; offset=offset*32; */
const unsigned short __addrTransferTable[16][384] = {
	{
		/* ================ y = 00,01,02,03 ===================== */
		/*x=000, A=00, B=03, C=00 */
		0x0300, 0x0320, 0x0340, 0x0360, 0x0380, 0x03A0, 0x03C0, 0x03E0, 0x0700, 0x0720, 0x0740, 0x0760, 0x0780, 0x07A0, 0x07C0, 0x07E0,
		/*x=016, A=02, B=03, C=00 */
		0x0B00, 0x0B20, 0x0B40, 0x0B60, 0x0B80, 0x0BA0, 0x0BC0, 0x0BE0, 0x0F00, 0x0F20, 0x0F40, 0x0F60, 0x0F80, 0x0FA0, 0x0FC0, 0x0FE0,
		/*x=032, A=04, B=03, C=00 */
		0x1300, 0x1320, 0x1340, 0x1360, 0x1380, 0x13A0, 0x13C0, 0x13E0, 0x1700, 0x1720, 0x1740, 0x1760, 0x1780, 0x17A0, 0x17C0, 0x17E0,
		/*x=048, A=06, B=03, C=00 */
		0x1B00, 0x1B20, 0x1B40, 0x1B60, 0x1B80, 0x1BA0, 0x1BC0, 0x1BE0, 0x1F00, 0x1F20, 0x1F40, 0x1F60, 0x1F80, 0x1FA0, 0x1FC0, 0x1FE0,
		/*x=064, A=08, B=03, C=00 */
		0x2300, 0x2320, 0x2340, 0x2360, 0x2380, 0x23A0, 0x23C0, 0x23E0, 0x2700, 0x2720, 0x2740, 0x2760, 0x2780, 0x27A0, 0x27C0, 0x27E0,
		/*x=080, A=10, B=03, C=00 */
		0x2B00, 0x2B20, 0x2B40, 0x2B60, 0x2B80, 0x2BA0, 0x2BC0, 0x2BE0, 0x2F00, 0x2F20, 0x2F40, 0x2F60, 0x2F80, 0x2FA0, 0x2FC0, 0x2FE0,
		/*x=096, A=12, B=03, C=00 */
		0x3300, 0x3320, 0x3340, 0x3360, 0x3380, 0x33A0, 0x33C0, 0x33E0, 0x3700, 0x3720, 0x3740, 0x3760, 0x3780, 0x37A0, 0x37C0, 0x37E0,
		/*x=112, A=14, B=03, C=00 */
		0x3B00, 0x3B20, 0x3B40, 0x3B60, 0x3B80, 0x3BA0, 0x3BC0, 0x3BE0, 0x3F00, 0x3F20, 0x3F40, 0x3F60, 0x3F80, 0x3FA0, 0x3FC0, 0x3FE0,
		/*x=128, A=16, B=03, C=00 */
		0x4300, 0x4320, 0x4340, 0x4360, 0x4380, 0x43A0, 0x43C0, 0x43E0, 0x4700, 0x4720, 0x4740, 0x4760, 0x4780, 0x47A0, 0x47C0, 0x47E0,
		/*x=144, A=18, B=03, C=00 */
		0x4B00, 0x4B20, 0x4B40, 0x4B60, 0x4B80, 0x4BA0, 0x4BC0, 0x4BE0, 0x4F00, 0x4F20, 0x4F40, 0x4F60, 0x4F80, 0x4FA0, 0x4FC0, 0x4FE0,
		/*x=160, A=20, B=03, C=00 */
		0x5300, 0x5320, 0x5340, 0x5360, 0x5380, 0x53A0, 0x53C0, 0x53E0, 0x5700, 0x5720, 0x5740, 0x5760, 0x5780, 0x57A0, 0x57C0, 0x57E0,
		/*x=176, A=22, B=03, C=00 */
		0x5B00, 0x5B20, 0x5B40, 0x5B60, 0x5B80, 0x5BA0, 0x5BC0, 0x5BE0, 0x5F00, 0x5F20, 0x5F40, 0x5F60, 0x5F80, 0x5FA0, 0x5FC0, 0x5FE0,
		/*x=192, A=24, B=03, C=00 */
		0x6300, 0x6320, 0x6340, 0x6360, 0x6380, 0x63A0, 0x63C0, 0x63E0, 0x6700, 0x6720, 0x6740, 0x6760, 0x6780, 0x67A0, 0x67C0, 0x67E0,
		/*x=208, A=26, B=03, C=00 */
		0x6B00, 0x6B20, 0x6B40, 0x6B60, 0x6B80, 0x6BA0, 0x6BC0, 0x6BE0, 0x6F00, 0x6F20, 0x6F40, 0x6F60, 0x6F80, 0x6FA0, 0x6FC0, 0x6FE0,
		/*x=224, A=28, B=03, C=00 */
		0x7300, 0x7320, 0x7340, 0x7360, 0x7380, 0x73A0, 0x73C0, 0x73E0, 0x7700, 0x7720, 0x7740, 0x7760, 0x7780, 0x77A0, 0x77C0, 0x77E0,
		/*x=240, A=30, B=03, C=00 */
		0x7B00, 0x7B20, 0x7B40, 0x7B60, 0x7B80, 0x7BA0, 0x7BC0, 0x7BE0, 0x7F00, 0x7F20, 0x7F40, 0x7F60, 0x7F80, 0x7FA0, 0x7FC0, 0x7FE0,
		/*x=256, A=32, B=03, C=00 */
		0x8300, 0x8320, 0x8340, 0x8360, 0x8380, 0x83A0, 0x83C0, 0x83E0, 0x8700, 0x8720, 0x8740, 0x8760, 0x8780, 0x87A0, 0x87C0, 0x87E0,
		/*x=272, A=34, B=03, C=00 */
		0x8B00, 0x8B20, 0x8B40, 0x8B60, 0x8B80, 0x8BA0, 0x8BC0, 0x8BE0, 0x8F00, 0x8F20, 0x8F40, 0x8F60, 0x8F80, 0x8FA0, 0x8FC0, 0x8FE0,
		/*x=288, A=36, B=03, C=00 */
		0x9300, 0x9320, 0x9340, 0x9360, 0x9380, 0x93A0, 0x93C0, 0x93E0, 0x9700, 0x9720, 0x9740, 0x9760, 0x9780, 0x97A0, 0x97C0, 0x97E0,
		/*x=304, A=38, B=03, C=00 */
		0x9B00, 0x9B20, 0x9B40, 0x9B60, 0x9B80, 0x9BA0, 0x9BC0, 0x9BE0, 0x9F00, 0x9F20, 0x9F40, 0x9F60, 0x9F80, 0x9FA0, 0x9FC0, 0x9FE0,
		/*x=320, A=40, B=03, C=00 */
		0xA300, 0xA320, 0xA340, 0xA360, 0xA380, 0xA3A0, 0xA3C0, 0xA3E0, 0xA700, 0xA720, 0xA740, 0xA760, 0xA780, 0xA7A0, 0xA7C0, 0xA7E0,
		/*x=336, A=42, B=03, C=00 */
		0xAB00, 0xAB20, 0xAB40, 0xAB60, 0xAB80, 0xABA0, 0xABC0, 0xABE0, 0xAF00, 0xAF20, 0xAF40, 0xAF60, 0xAF80, 0xAFA0, 0xAFC0, 0xAFE0,
		/*x=352, A=44, B=03, C=00 */
		0xB300, 0xB320, 0xB340, 0xB360, 0xB380, 0xB3A0, 0xB3C0, 0xB3E0, 0xB700, 0xB720, 0xB740, 0xB760, 0xB780, 0xB7A0, 0xB7C0, 0xB7E0,
		/*x=368, A=46, B=03, C=00 */
		0xBB00, 0xBB20, 0xBB40, 0xBB60, 0xBB80, 0xBBA0, 0xBBC0, 0xBBE0, 0xBF00, 0xBF20, 0xBF40, 0xBF60, 0xBF80, 0xBFA0, 0xBFC0, 0xBFE0,
	},
	{
		/* ================ y = 04,05,06,07 ===================== */
		/*x=000, A=00, B=02, C=00 */
		0x0200, 0x0220, 0x0240, 0x0260, 0x0280, 0x02A0, 0x02C0, 0x02E0, 0x0600, 0x0620, 0x0640, 0x0660, 0x0680, 0x06A0, 0x06C0, 0x06E0,
		/*x=016, A=02, B=02, C=00 */
		0x0A00, 0x0A20, 0x0A40, 0x0A60, 0x0A80, 0x0AA0, 0x0AC0, 0x0AE0, 0x0E00, 0x0E20, 0x0E40, 0x0E60, 0x0E80, 0x0EA0, 0x0EC0, 0x0EE0,
		/*x=032, A=04, B=02, C=00 */
		0x1200, 0x1220, 0x1240, 0x1260, 0x1280, 0x12A0, 0x12C0, 0x12E0, 0x1600, 0x1620, 0x1640, 0x1660, 0x1680, 0x16A0, 0x16C0, 0x16E0,
		/*x=048, A=06, B=02, C=00 */
		0x1A00, 0x1A20, 0x1A40, 0x1A60, 0x1A80, 0x1AA0, 0x1AC0, 0x1AE0, 0x1E00, 0x1E20, 0x1E40, 0x1E60, 0x1E80, 0x1EA0, 0x1EC0, 0x1EE0,
		/*x=064, A=08, B=02, C=00 */
		0x2200, 0x2220, 0x2240, 0x2260, 0x2280, 0x22A0, 0x22C0, 0x22E0, 0x2600, 0x2620, 0x2640, 0x2660, 0x2680, 0x26A0, 0x26C0, 0x26E0,
		/*x=080, A=10, B=02, C=00 */
		0x2A00, 0x2A20, 0x2A40, 0x2A60, 0x2A80, 0x2AA0, 0x2AC0, 0x2AE0, 0x2E00, 0x2E20, 0x2E40, 0x2E60, 0x2E80, 0x2EA0, 0x2EC0, 0x2EE0,
		/*x=096, A=12, B=02, C=00 */
		0x3200, 0x3220, 0x3240, 0x3260, 0x3280, 0x32A0, 0x32C0, 0x32E0, 0x3600, 0x3620, 0x3640, 0x3660, 0x3680, 0x36A0, 0x36C0, 0x36E0,
		/*x=112, A=14, B=02, C=00 */
		0x3A00, 0x3A20, 0x3A40, 0x3A60, 0x3A80, 0x3AA0, 0x3AC0, 0x3AE0, 0x3E00, 0x3E20, 0x3E40, 0x3E60, 0x3E80, 0x3EA0, 0x3EC0, 0x3EE0,
		/*x=128, A=16, B=02, C=00 */
		0x4200, 0x4220, 0x4240, 0x4260, 0x4280, 0x42A0, 0x42C0, 0x42E0, 0x4600, 0x4620, 0x4640, 0x4660, 0x4680, 0x46A0, 0x46C0, 0x46E0,
		/*x=144, A=18, B=02, C=00 */
		0x4A00, 0x4A20, 0x4A40, 0x4A60, 0x4A80, 0x4AA0, 0x4AC0, 0x4AE0, 0x4E00, 0x4E20, 0x4E40, 0x4E60, 0x4E80, 0x4EA0, 0x4EC0, 0x4EE0,
		/*x=160, A=20, B=02, C=00 */
		0x5200, 0x5220, 0x5240, 0x5260, 0x5280, 0x52A0, 0x52C0, 0x52E0, 0x5600, 0x5620, 0x5640, 0x5660, 0x5680, 0x56A0, 0x56C0, 0x56E0,
		/*x=176, A=22, B=02, C=00 */
		0x5A00, 0x5A20, 0x5A40, 0x5A60, 0x5A80, 0x5AA0, 0x5AC0, 0x5AE0, 0x5E00, 0x5E20, 0x5E40, 0x5E60, 0x5E80, 0x5EA0, 0x5EC0, 0x5EE0,
		/*x=192, A=24, B=02, C=00 */
		0x6200, 0x6220, 0x6240, 0x6260, 0x6280, 0x62A0, 0x62C0, 0x62E0, 0x6600, 0x6620, 0x6640, 0x6660, 0x6680, 0x66A0, 0x66C0, 0x66E0,
		/*x=208, A=26, B=02, C=00 */
		0x6A00, 0x6A20, 0x6A40, 0x6A60, 0x6A80, 0x6AA0, 0x6AC0, 0x6AE0, 0x6E00, 0x6E20, 0x6E40, 0x6E60, 0x6E80, 0x6EA0, 0x6EC0, 0x6EE0,
		/*x=224, A=28, B=02, C=00 */
		0x7200, 0x7220, 0x7240, 0x7260, 0x7280, 0x72A0, 0x72C0, 0x72E0, 0x7600, 0x7620, 0x7640, 0x7660, 0x7680, 0x76A0, 0x76C0, 0x76E0,
		/*x=240, A=30, B=02, C=00 */
		0x7A00, 0x7A20, 0x7A40, 0x7A60, 0x7A80, 0x7AA0, 0x7AC0, 0x7AE0, 0x7E00, 0x7E20, 0x7E40, 0x7E60, 0x7E80, 0x7EA0, 0x7EC0, 0x7EE0,
		/*x=256, A=32, B=02, C=00 */
		0x8200, 0x8220, 0x8240, 0x8260, 0x8280, 0x82A0, 0x82C0, 0x82E0, 0x8600, 0x8620, 0x8640, 0x8660, 0x8680, 0x86A0, 0x86C0, 0x86E0,
		/*x=272, A=34, B=02, C=00 */
		0x8A00, 0x8A20, 0x8A40, 0x8A60, 0x8A80, 0x8AA0, 0x8AC0, 0x8AE0, 0x8E00, 0x8E20, 0x8E40, 0x8E60, 0x8E80, 0x8EA0, 0x8EC0, 0x8EE0,
		/*x=288, A=36, B=02, C=00 */
		0x9200, 0x9220, 0x9240, 0x9260, 0x9280, 0x92A0, 0x92C0, 0x92E0, 0x9600, 0x9620, 0x9640, 0x9660, 0x9680, 0x96A0, 0x96C0, 0x96E0,
		/*x=304, A=38, B=02, C=00 */
		0x9A00, 0x9A20, 0x9A40, 0x9A60, 0x9A80, 0x9AA0, 0x9AC0, 0x9AE0, 0x9E00, 0x9E20, 0x9E40, 0x9E60, 0x9E80, 0x9EA0, 0x9EC0, 0x9EE0,
		/*x=320, A=40, B=02, C=00 */
		0xA200, 0xA220, 0xA240, 0xA260, 0xA280, 0xA2A0, 0xA2C0, 0xA2E0, 0xA600, 0xA620, 0xA640, 0xA660, 0xA680, 0xA6A0, 0xA6C0, 0xA6E0,
		/*x=336, A=42, B=02, C=00 */
		0xAA00, 0xAA20, 0xAA40, 0xAA60, 0xAA80, 0xAAA0, 0xAAC0, 0xAAE0, 0xAE00, 0xAE20, 0xAE40, 0xAE60, 0xAE80, 0xAEA0, 0xAEC0, 0xAEE0,
		/*x=352, A=44, B=02, C=00 */
		0xB200, 0xB220, 0xB240, 0xB260, 0xB280, 0xB2A0, 0xB2C0, 0xB2E0, 0xB600, 0xB620, 0xB640, 0xB660, 0xB680, 0xB6A0, 0xB6C0, 0xB6E0,
		/*x=368, A=46, B=02, C=00 */
		0xBA00, 0xBA20, 0xBA40, 0xBA60, 0xBA80, 0xBAA0, 0xBAC0, 0xBAE0, 0xBE00, 0xBE20, 0xBE40, 0xBE60, 0xBE80, 0xBEA0, 0xBEC0, 0xBEE0,
	},
	{
		/* ================ y = 08,09,10,11 ===================== */
		/*x=000, A=00, B=01, C=00 */
		0x0100, 0x0120, 0x0140, 0x0160, 0x0180, 0x01A0, 0x01C0, 0x01E0, 0x0500, 0x0520, 0x0540, 0x0560, 0x0580, 0x05A0, 0x05C0, 0x05E0,
		/*x=016, A=02, B=01, C=00 */
		0x0900, 0x0920, 0x0940, 0x0960, 0x0980, 0x09A0, 0x09C0, 0x09E0, 0x0D00, 0x0D20, 0x0D40, 0x0D60, 0x0D80, 0x0DA0, 0x0DC0, 0x0DE0,
		/*x=032, A=04, B=01, C=00 */
		0x1100, 0x1120, 0x1140, 0x1160, 0x1180, 0x11A0, 0x11C0, 0x11E0, 0x1500, 0x1520, 0x1540, 0x1560, 0x1580, 0x15A0, 0x15C0, 0x15E0,
		/*x=048, A=06, B=01, C=00 */
		0x1900, 0x1920, 0x1940, 0x1960, 0x1980, 0x19A0, 0x19C0, 0x19E0, 0x1D00, 0x1D20, 0x1D40, 0x1D60, 0x1D80, 0x1DA0, 0x1DC0, 0x1DE0,
		/*x=064, A=08, B=01, C=00 */
		0x2100, 0x2120, 0x2140, 0x2160, 0x2180, 0x21A0, 0x21C0, 0x21E0, 0x2500, 0x2520, 0x2540, 0x2560, 0x2580, 0x25A0, 0x25C0, 0x25E0,
		/*x=080, A=10, B=01, C=00 */
		0x2900, 0x2920, 0x2940, 0x2960, 0x2980, 0x29A0, 0x29C0, 0x29E0, 0x2D00, 0x2D20, 0x2D40, 0x2D60, 0x2D80, 0x2DA0, 0x2DC0, 0x2DE0,
		/*x=096, A=12, B=01, C=00 */
		0x3100, 0x3120, 0x3140, 0x3160, 0x3180, 0x31A0, 0x31C0, 0x31E0, 0x3500, 0x3520, 0x3540, 0x3560, 0x3580, 0x35A0, 0x35C0, 0x35E0,
		/*x=112, A=14, B=01, C=00 */
		0x3900, 0x3920, 0x3940, 0x3960, 0x3980, 0x39A0, 0x39C0, 0x39E0, 0x3D00, 0x3D20, 0x3D40, 0x3D60, 0x3D80, 0x3DA0, 0x3DC0, 0x3DE0,
		/*x=128, A=16, B=01, C=00 */
		0x4100, 0x4120, 0x4140, 0x4160, 0x4180, 0x41A0, 0x41C0, 0x41E0, 0x4500, 0x4520, 0x4540, 0x4560, 0x4580, 0x45A0, 0x45C0, 0x45E0,
		/*x=144, A=18, B=01, C=00 */
		0x4900, 0x4920, 0x4940, 0x4960, 0x4980, 0x49A0, 0x49C0, 0x49E0, 0x4D00, 0x4D20, 0x4D40, 0x4D60, 0x4D80, 0x4DA0, 0x4DC0, 0x4DE0,
		/*x=160, A=20, B=01, C=00 */
		0x5100, 0x5120, 0x5140, 0x5160, 0x5180, 0x51A0, 0x51C0, 0x51E0, 0x5500, 0x5520, 0x5540, 0x5560, 0x5580, 0x55A0, 0x55C0, 0x55E0,
		/*x=176, A=22, B=01, C=00 */
		0x5900, 0x5920, 0x5940, 0x5960, 0x5980, 0x59A0, 0x59C0, 0x59E0, 0x5D00, 0x5D20, 0x5D40, 0x5D60, 0x5D80, 0x5DA0, 0x5DC0, 0x5DE0,
		/*x=192, A=24, B=01, C=00 */
		0x6100, 0x6120, 0x6140, 0x6160, 0x6180, 0x61A0, 0x61C0, 0x61E0, 0x6500, 0x6520, 0x6540, 0x6560, 0x6580, 0x65A0, 0x65C0, 0x65E0,
		/*x=208, A=26, B=01, C=00 */
		0x6900, 0x6920, 0x6940, 0x6960, 0x6980, 0x69A0, 0x69C0, 0x69E0, 0x6D00, 0x6D20, 0x6D40, 0x6D60, 0x6D80, 0x6DA0, 0x6DC0, 0x6DE0,
		/*x=224, A=28, B=01, C=00 */
		0x7100, 0x7120, 0x7140, 0x7160, 0x7180, 0x71A0, 0x71C0, 0x71E0, 0x7500, 0x7520, 0x7540, 0x7560, 0x7580, 0x75A0, 0x75C0, 0x75E0,
		/*x=240, A=30, B=01, C=00 */
		0x7900, 0x7920, 0x7940, 0x7960, 0x7980, 0x79A0, 0x79C0, 0x79E0, 0x7D00, 0x7D20, 0x7D40, 0x7D60, 0x7D80, 0x7DA0, 0x7DC0, 0x7DE0,
		/*x=256, A=32, B=01, C=00 */
		0x8100, 0x8120, 0x8140, 0x8160, 0x8180, 0x81A0, 0x81C0, 0x81E0, 0x8500, 0x8520, 0x8540, 0x8560, 0x8580, 0x85A0, 0x85C0, 0x85E0,
		/*x=272, A=34, B=01, C=00 */
		0x8900, 0x8920, 0x8940, 0x8960, 0x8980, 0x89A0, 0x89C0, 0x89E0, 0x8D00, 0x8D20, 0x8D40, 0x8D60, 0x8D80, 0x8DA0, 0x8DC0, 0x8DE0,
		/*x=288, A=36, B=01, C=00 */
		0x9100, 0x9120, 0x9140, 0x9160, 0x9180, 0x91A0, 0x91C0, 0x91E0, 0x9500, 0x9520, 0x9540, 0x9560, 0x9580, 0x95A0, 0x95C0, 0x95E0,
		/*x=304, A=38, B=01, C=00 */
		0x9900, 0x9920, 0x9940, 0x9960, 0x9980, 0x99A0, 0x99C0, 0x99E0, 0x9D00, 0x9D20, 0x9D40, 0x9D60, 0x9D80, 0x9DA0, 0x9DC0, 0x9DE0,
		/*x=320, A=40, B=01, C=00 */
		0xA100, 0xA120, 0xA140, 0xA160, 0xA180, 0xA1A0, 0xA1C0, 0xA1E0, 0xA500, 0xA520, 0xA540, 0xA560, 0xA580, 0xA5A0, 0xA5C0, 0xA5E0,
		/*x=336, A=42, B=01, C=00 */
		0xA900, 0xA920, 0xA940, 0xA960, 0xA980, 0xA9A0, 0xA9C0, 0xA9E0, 0xAD00, 0xAD20, 0xAD40, 0xAD60, 0xAD80, 0xADA0, 0xADC0, 0xADE0,
		/*x=352, A=44, B=01, C=00 */
		0xB100, 0xB120, 0xB140, 0xB160, 0xB180, 0xB1A0, 0xB1C0, 0xB1E0, 0xB500, 0xB520, 0xB540, 0xB560, 0xB580, 0xB5A0, 0xB5C0, 0xB5E0,
		/*x=368, A=46, B=01, C=00 */
		0xB900, 0xB920, 0xB940, 0xB960, 0xB980, 0xB9A0, 0xB9C0, 0xB9E0, 0xBD00, 0xBD20, 0xBD40, 0xBD60, 0xBD80, 0xBDA0, 0xBDC0, 0xBDE0,
	},
	{
		/* ================ y = 12,13,14,15 ===================== */
		/*x=000, A=00, B=00, C=00 */
		0x0000, 0x0020, 0x0040, 0x0060, 0x0080, 0x00A0, 0x00C0, 0x00E0, 0x0400, 0x0420, 0x0440, 0x0460, 0x0480, 0x04A0, 0x04C0, 0x04E0,
		/*x=016, A=02, B=00, C=00 */
		0x0800, 0x0820, 0x0840, 0x0860, 0x0880, 0x08A0, 0x08C0, 0x08E0, 0x0C00, 0x0C20, 0x0C40, 0x0C60, 0x0C80, 0x0CA0, 0x0CC0, 0x0CE0,
		/*x=032, A=04, B=00, C=00 */
		0x1000, 0x1020, 0x1040, 0x1060, 0x1080, 0x10A0, 0x10C0, 0x10E0, 0x1400, 0x1420, 0x1440, 0x1460, 0x1480, 0x14A0, 0x14C0, 0x14E0,
		/*x=048, A=06, B=00, C=00 */
		0x1800, 0x1820, 0x1840, 0x1860, 0x1880, 0x18A0, 0x18C0, 0x18E0, 0x1C00, 0x1C20, 0x1C40, 0x1C60, 0x1C80, 0x1CA0, 0x1CC0, 0x1CE0,
		/*x=064, A=08, B=00, C=00 */
		0x2000, 0x2020, 0x2040, 0x2060, 0x2080, 0x20A0, 0x20C0, 0x20E0, 0x2400, 0x2420, 0x2440, 0x2460, 0x2480, 0x24A0, 0x24C0, 0x24E0,
		/*x=080, A=10, B=00, C=00 */
		0x2800, 0x2820, 0x2840, 0x2860, 0x2880, 0x28A0, 0x28C0, 0x28E0, 0x2C00, 0x2C20, 0x2C40, 0x2C60, 0x2C80, 0x2CA0, 0x2CC0, 0x2CE0,
		/*x=096, A=12, B=00, C=00 */
		0x3000, 0x3020, 0x3040, 0x3060, 0x3080, 0x30A0, 0x30C0, 0x30E0, 0x3400, 0x3420, 0x3440, 0x3460, 0x3480, 0x34A0, 0x34C0, 0x34E0,
		/*x=112, A=14, B=00, C=00 */
		0x3800, 0x3820, 0x3840, 0x3860, 0x3880, 0x38A0, 0x38C0, 0x38E0, 0x3C00, 0x3C20, 0x3C40, 0x3C60, 0x3C80, 0x3CA0, 0x3CC0, 0x3CE0,
		/*x=128, A=16, B=00, C=00 */
		0x4000, 0x4020, 0x4040, 0x4060, 0x4080, 0x40A0, 0x40C0, 0x40E0, 0x4400, 0x4420, 0x4440, 0x4460, 0x4480, 0x44A0, 0x44C0, 0x44E0,
		/*x=144, A=18, B=00, C=00 */
		0x4800, 0x4820, 0x4840, 0x4860, 0x4880, 0x48A0, 0x48C0, 0x48E0, 0x4C00, 0x4C20, 0x4C40, 0x4C60, 0x4C80, 0x4CA0, 0x4CC0, 0x4CE0,
		/*x=160, A=20, B=00, C=00 */
		0x5000, 0x5020, 0x5040, 0x5060, 0x5080, 0x50A0, 0x50C0, 0x50E0, 0x5400, 0x5420, 0x5440, 0x5460, 0x5480, 0x54A0, 0x54C0, 0x54E0,
		/*x=176, A=22, B=00, C=00 */
		0x5800, 0x5820, 0x5840, 0x5860, 0x5880, 0x58A0, 0x58C0, 0x58E0, 0x5C00, 0x5C20, 0x5C40, 0x5C60, 0x5C80, 0x5CA0, 0x5CC0, 0x5CE0,
		/*x=192, A=24, B=00, C=00 */
		0x6000, 0x6020, 0x6040, 0x6060, 0x6080, 0x60A0, 0x60C0, 0x60E0, 0x6400, 0x6420, 0x6440, 0x6460, 0x6480, 0x64A0, 0x64C0, 0x64E0,
		/*x=208, A=26, B=00, C=00 */
		0x6800, 0x6820, 0x6840, 0x6860, 0x6880, 0x68A0, 0x68C0, 0x68E0, 0x6C00, 0x6C20, 0x6C40, 0x6C60, 0x6C80, 0x6CA0, 0x6CC0, 0x6CE0,
		/*x=224, A=28, B=00, C=00 */
		0x7000, 0x7020, 0x7040, 0x7060, 0x7080, 0x70A0, 0x70C0, 0x70E0, 0x7400, 0x7420, 0x7440, 0x7460, 0x7480, 0x74A0, 0x74C0, 0x74E0,
		/*x=240, A=30, B=00, C=00 */
		0x7800, 0x7820, 0x7840, 0x7860, 0x7880, 0x78A0, 0x78C0, 0x78E0, 0x7C00, 0x7C20, 0x7C40, 0x7C60, 0x7C80, 0x7CA0, 0x7CC0, 0x7CE0,
		/*x=256, A=32, B=00, C=00 */
		0x8000, 0x8020, 0x8040, 0x8060, 0x8080, 0x80A0, 0x80C0, 0x80E0, 0x8400, 0x8420, 0x8440, 0x8460, 0x8480, 0x84A0, 0x84C0, 0x84E0,
		/*x=272, A=34, B=00, C=00 */
		0x8800, 0x8820, 0x8840, 0x8860, 0x8880, 0x88A0, 0x88C0, 0x88E0, 0x8C00, 0x8C20, 0x8C40, 0x8C60, 0x8C80, 0x8CA0, 0x8CC0, 0x8CE0,
		/*x=288, A=36, B=00, C=00 */
		0x9000, 0x9020, 0x9040, 0x9060, 0x9080, 0x90A0, 0x90C0, 0x90E0, 0x9400, 0x9420, 0x9440, 0x9460, 0x9480, 0x94A0, 0x94C0, 0x94E0,
		/*x=304, A=38, B=00, C=00 */
		0x9800, 0x9820, 0x9840, 0x9860, 0x9880, 0x98A0, 0x98C0, 0x98E0, 0x9C00, 0x9C20, 0x9C40, 0x9C60, 0x9C80, 0x9CA0, 0x9CC0, 0x9CE0,
		/*x=320, A=40, B=00, C=00 */
		0xA000, 0xA020, 0xA040, 0xA060, 0xA080, 0xA0A0, 0xA0C0, 0xA0E0, 0xA400, 0xA420, 0xA440, 0xA460, 0xA480, 0xA4A0, 0xA4C0, 0xA4E0,
		/*x=336, A=42, B=00, C=00 */
		0xA800, 0xA820, 0xA840, 0xA860, 0xA880, 0xA8A0, 0xA8C0, 0xA8E0, 0xAC00, 0xAC20, 0xAC40, 0xAC60, 0xAC80, 0xACA0, 0xACC0, 0xACE0,
		/*x=352, A=44, B=00, C=00 */
		0xB000, 0xB020, 0xB040, 0xB060, 0xB080, 0xB0A0, 0xB0C0, 0xB0E0, 0xB400, 0xB420, 0xB440, 0xB460, 0xB480, 0xB4A0, 0xB4C0, 0xB4E0,
		/*x=368, A=46, B=00, C=00 */
		0xB800, 0xB820, 0xB840, 0xB860, 0xB880, 0xB8A0, 0xB8C0, 0xB8E0, 0xBC00, 0xBC20, 0xBC40, 0xBC60, 0xBC80, 0xBCA0, 0xBCC0, 0xBCE0,
	},
};

#endif



void LedDisplayToScan(int x, int y, int xend, int yend) {
	int vx;
	unsigned int dest;
	const unsigned short *pOffset;
	int *src;

	for (; y <= yend; ++y) {
		src = __displayBufferBit + y * LED_DOT_WIDTH + x;
		dest = (unsigned int)__scanBufferBit[y % LED_SCAN_MUX];
		pOffset = &__addrTransferTable[y % 16 / 4][x];
		if (y >= LED_DOT_HEIGHT / 2) {
			dest += 4;
		}
		dest += (y >> 1) & (((LED_DOT_HEIGHT / 2 - 1) >> 1) & 0x18);
		for (vx = x; vx <= xend; ++vx) {
#if LED_DRIVER_LEVEL==0
			*((int *)(dest + *pOffset++)) = !(*src++);
#elif LED_DRIVER_LEVEL==1
			*((int *)(dest + *pOffset++)) = *src++;
#else
#error "LED_DRIVER_LEVEL MUST be 0 or 1"
#endif
		}
	}
}
