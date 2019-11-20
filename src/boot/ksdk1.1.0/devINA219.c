/* C code for INA219*/


#include <stdlib.h>

#include "fsl_misc_utilities.h"
#include "fsl_device_registers.h"
#include "fsl_i2c_master_driver.h"
#include "fsl_spi_master_driver.h"
#include "fsl_rtc_driver.h"
#include "fsl_clock_manager.h"
#include "fsl_power_manager.h"
#include "fsl_mcglite_hal.h"
#include "fsl_port_hal.h"

#include "gpio_pins.h"
#include "SEGGER_RTT.h"
#include "warp.h"


extern volatile WarpI2CDeviceState	deviceINA219State;
extern volatile uint32_t		gWarpI2cBaudRateKbps;
extern volatile uint32_t		gWarpI2cTimeoutMilliseconds;
extern volatile uint32_t		gWarpSupplySettlingDelayMilliseconds;



void
initINA219(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress	= i2cAddress;
	return;
}

WarpStatus
readSensorRegisterINA219(int numberOfBytes)
{
//	uint8_t		cmdBuf[1];
	i2c_status_t	status;

	USED(numberOfBytes);

	i2c_device_t slave =
	{
		.address = deviceINA219State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};


	//cmdBuf[0] = deviceRegister;
	// From the data sheet, read address is set by a pointer on the current sensor chip and
	// must be set by a write command. The device address is therefore not sent during the read
	// command and so cmdBuf size should be 0
	status = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							NULL,
							0,
							(uint8_t *)deviceINA219State.i2cBuffer,
							numberOfBytes,
							gWarpI2cTimeoutMilliseconds);
	if (status != kStatus_I2C_Success)
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}


WarpStatus
writeSensorRegisterINA219(uint8_t deviceRegister, uint8_t payload, uint16_t menuI2cPullupValue)
{
	uint8_t		payloadByte[1], commandByte[1];
	i2c_status_t	status;

	i2c_device_t slave =
	{
		.address = deviceINA219State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	commandByte[0] = deviceRegister;
	payloadByte[0] = payload;

	status = I2C_DRV_MasterSendDataBlocking(
							0 /* I2C instance */,
							&slave,
							commandByte,
							1,
							payloadByte,
							0,
							gWarpI2cTimeoutMilliseconds);

	if (status != kStatus_I2C_Success)
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}


void
printSensorDataINA219()
{
	uint16_t	readSensorRegisterValueLSB;
	uint16_t	readSensorRegisterValueMSB;
	int16_t		readSensorRegisterValueCombined;

	// Set pointer address to Bus Voltage (0x02 from data sheet)
	writeSensorRegisterINA219(0x02,0x00,1);

	// Read both MSB and LSB of Bus Voltage:
	readSensorRegisterINA219(2);
	readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
	readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
	// Taking the top 12 bits as defined by the data sheet:
	readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 5) | ((readSensorRegisterValueLSB & 0xF8) >> 2);
	SEGGER_RTT_printf(0, "\nBus Voltage Reading: %d, giving Bus Voltage: %dmV", readSensorRegisterValueCombined, readSensorRegisterValueCombined * 4);

	// Set pointer address to Shunt Voltage (0x01 from data sheet)
        writeSensorRegisterINA219(0x01,0x00,1);
        // Read both MSB and LSB of Shunt Voltage:
        readSensorRegisterINA219(2);
        readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
        readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
        readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 8) | (readSensorRegisterValueLSB & 0xFF);
        SEGGER_RTT_printf(0, "\nShunt Voltage Reading: %d, giving Shunt Voltage: %duV, giving Current: %duA", readSensorRegisterValueCombined,readSensorRegisterValueCombined*10, readSensorRegisterValueCombined*100);
}




/*
#include <stdlib.h>

#include "fsl_misc_utilities.h"
#include "fsl_device_registers.h"
#include "fsl_i2c_master_driver.h"
#include "fsl_spi_master_driver.h"
#include "fsl_rtc_driver.h"
#include "fsl_clock_manager.h"
#include "fsl_power_manager.h"
#include "fsl_mcglite_hal.h"
#include "fsl_port_hal.h"

#include "gpio_pins.h"
#include "SEGGER_RTT.h"
#include "warp.h"


extern volatile WarpI2CDeviceState	deviceINA219State;
extern volatile uint32_t		gWarpI2cBaudRateKbps;
extern volatile uint32_t		gWarpI2cTimeoutMilliseconds;
extern volatile uint32_t		gWarpSupplySettlingDelayMilliseconds;


void
initINA219(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress	= i2cAddress;
	deviceStatePointer->signalType	= (kWarpTypeMaskCurrent);

	return;
}

WarpStatus
writeSensorRegisterINA219(uint8_t deviceRegister, uint16_t payload, uint16_t menuI2cPullupValue){
}

WarpStatus	
configureSensorINA219(uint8_t payloadConfigReg, uint8_t payloadFrameRateReg, uint16_t menuI2cPullupValue){
}

WarpStatus
readSensorRegisterINA219(uint8_t deviceRegister, int numberOfBytes){
}

void	
printSensorDataINA219(bool hexModeFlag){
}
*/
