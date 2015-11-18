#ifndef __SENSORS_H__
#define __SENSORS_H__

/*
���մ�����:
Power:		3.3v
PB6(42)		SDA	(4.7k����)
PB7(43)		SCL	(4.7k����)
][
�����¶�:
Power:		5v
PA2(12)		DQ
PA3(13)		DQ
PA4(14)		DQ(4.7k����)

����ʪ��:
Power:		5v
PA0:(10)	ADC12_INT0
PA6:(16)	ADC12_INT6
PB0:(18)	ADC12_INT8

����:		100ŷķ����
Power:		12v
PA7:(17)	ADC12_INT7
**
����:
Power:		12v
PA1(11)		TIM2

����:												                                            
Power:		5v
PB8(45)		EXTI    (10K����)

������ʪ��:
Power:		3.3v
PB2(20)		SCL		(4.7k����)
PB1(19)		SDA		(4.7k����)
*/
void SensorManager_config(void);
void Sensor_Notify(const char *dat, int size);

#endif

