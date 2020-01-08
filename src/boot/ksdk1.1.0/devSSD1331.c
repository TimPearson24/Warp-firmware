#include <stdint.h>

#include "fsl_spi_master_driver.h"
#include "fsl_port_hal.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devSSD1331.h"

volatile uint8_t	inBuffer[1];
volatile uint8_t	payloadBytes[1];


/*
 *	Override Warp firmware's use of these pins and define new aliases.
 */
enum
{
	kSSD1331PinMOSI		= GPIO_MAKE_PIN(HW_GPIOA, 8),
	kSSD1331PinSCK		= GPIO_MAKE_PIN(HW_GPIOA, 9),
	kSSD1331PinCSn		= GPIO_MAKE_PIN(HW_GPIOB, 13),
	kSSD1331PinDC		= GPIO_MAKE_PIN(HW_GPIOA, 12),
	kSSD1331PinRST		= GPIO_MAKE_PIN(HW_GPIOB, 0),
};

static int
writeCommand(uint8_t commandByte)
{
	spi_status_t status;

	/*
	 *	Drive /CS low.
	 *
	 *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);
	OSA_TimeDelay(10);
	GPIO_DRV_ClearPinOutput(kSSD1331PinCSn);

	/*
	 *	Drive DC low (command).
	 */
	GPIO_DRV_ClearPinOutput(kSSD1331PinDC);

	payloadBytes[0] = commandByte;
	status = SPI_DRV_MasterTransferBlocking(0	/* master instance */,
					NULL		/* spi_master_user_config_t */,
					(const uint8_t * restrict)&payloadBytes[0],
					(uint8_t * restrict)&inBuffer[0],
					1		/* transfer size */,
					1000		/* timeout in microseconds (unlike I2C which is ms) */);

	/*
	 *	Drive /CS high
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);

	return status;
}



int
devSSD1331init(void)
{
	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Re-configure SPI to be on PTA8 and PTA9 for MOSI and SCK respectively.
	 */
	PORT_HAL_SetMuxMode(PORTA_BASE, 8u, kPortMuxAlt3);
	PORT_HAL_SetMuxMode(PORTA_BASE, 9u, kPortMuxAlt3);

	enableSPIpins();

	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Reconfigure to use as GPIO.
	 */
	PORT_HAL_SetMuxMode(PORTB_BASE, 13u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 12u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 0u, kPortMuxAsGpio);


	/*
	 *	RST high->low->high.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_ClearPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);

	/*
	 *	Initialization sequence, borrowed from https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino
	 */
	writeCommand(kSSD1331CommandDISPLAYOFF);	// 0xAE
	writeCommand(kSSD1331CommandSETREMAP);		// 0xA0
	writeCommand(0x72);				// RGB Color
	writeCommand(kSSD1331CommandSTARTLINE);		// 0xA1
	writeCommand(0x0);
	writeCommand(kSSD1331CommandDISPLAYOFFSET);	// 0xA2
	writeCommand(0x0);
	writeCommand(kSSD1331CommandNORMALDISPLAY);	// 0xA4
	writeCommand(kSSD1331CommandSETMULTIPLEX);	// 0xA8
	writeCommand(0x3F);				// 0x3F 1/64 duty
	writeCommand(kSSD1331CommandSETMASTER);		// 0xAD
	writeCommand(0x8E);
	writeCommand(kSSD1331CommandPOWERMODE);		// 0xB0
	writeCommand(0x0B);
	writeCommand(kSSD1331CommandPRECHARGE);		// 0xB1
	writeCommand(0x31);
	writeCommand(kSSD1331CommandCLOCKDIV);		// 0xB3
	writeCommand(0xF0);				// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8A
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGEB);	// 0x8B
	writeCommand(0x78);
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8C
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGELEVEL);	// 0xBB
	writeCommand(0x3A);
	writeCommand(kSSD1331CommandVCOMH);		// 0xBE
	writeCommand(0x3E);
	writeCommand(kSSD1331CommandMASTERCURRENT);	// 0x87
	writeCommand(0x06);
	writeCommand(kSSD1331CommandCONTRASTA);		// 0x81
	writeCommand(0x91);
	writeCommand(kSSD1331CommandCONTRASTB);		// 0x82
	writeCommand(0x50);
	writeCommand(kSSD1331CommandCONTRASTC);		// 0x83
	writeCommand(0x7D);
	writeCommand(kSSD1331CommandDISPLAYON);		// Turn on oled panel

	/*
	 *	To use fill commands, you will have to issue a command to the display to enable them. See the manual.
	 */
	writeCommand(kSSD1331CommandFILL);
	writeCommand(0x01);

	/*
	 *	Clear Screen
	 */
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);
	
	//The screen is filled with RED to tell the user that the initilisation sequence has been completed

	writeCommand(0x22);	//draw rectangle command
	writeCommand(0x00);	//column address start
	writeCommand(0x00);	//row address start
	writeCommand(0x5F);	//column address end
	writeCommand(0x3F);	//row address end	
	writeCommand(0xFF);	//colour C (RED) of outline (max FF)
	writeCommand(0x00);	//colour B (GREEN) of outline (max 3F)
	writeCommand(0x00);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF);	//colour C of fill
	writeCommand(0x00);	//colour B of fill
	writeCommand(0x00);	//colour A of fill
	OSA_TimeDelay(1000);


	//Clear Screen Again

	writeCommand(kSSD1331CommandCLEAR);
        writeCommand(0x00);
        writeCommand(0x00);
        writeCommand(0x5F);
        writeCommand(0x3F);



	/*
	 *	Any post-initialization drawing commands go here.
	 */
//	writeCommand(0x22);
//	writeCommand(0x00);
//      writeCommand(0x00);
//      writeCommand(0x5F);
//      writeCommand(0x3F);
//	writeCommand(0x00);
//	writeCommand(0x3F);
//	writeCommand(0x00);
//	writeCommand(0x00);
//	writeCommand(0x3F);
//	writeCommand(0x00);

	return 0;
}

//function to generate a white flash on the OLED screen
int
devSSD1331_flash(int flash_period)
{
	writeCommand(kSSD1331CommandCLEAR);
        writeCommand(0x00);
        writeCommand(0x00);
        writeCommand(0x5F);
        writeCommand(0x3F);
	
	writeCommand(0x22);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);
	writeCommand(0xFF);
	writeCommand(0x3F);
	writeCommand(0xFF);
	writeCommand(0xFF);
	writeCommand(0x3F);
	writeCommand(0xFF);
	
	OSA_TimeDelay(flash_period);
	
	writeCommand(kSSD1331CommandCLEAR);
        writeCommand(0x00);
        writeCommand(0x00);
        writeCommand(0x5F);
        writeCommand(0x3F);
	
	return 0;
}

int
devSSD1331_clearscreen(void)
{
	writeCommand(kSSD1331CommandCLEAR);
        writeCommand(0x00);
        writeCommand(0x00);
        writeCommand(0x5F);
        writeCommand(0x3F);
	
	return 0;
}



//function to generate and manipulate bars
int
devSSD1331_bars(int *time_bin, int *time_bin_indicator, int size)
{	
	int t_array[16] = {0};
	int t_indicator[16] = {0};
	int growth_step = 0x30; //number of pixels grown
	int max_growth_steps = 0x38/growth_step;	//no. of steps the bar can increase before reaching the top of the screen
	
	//create local array of the time bins
	for(int x = 0; x < size; x ++)
    	{
		/* The pointer time_bin is pointing to the first element of
		 * the array and size is the size of the array. In the
		 * loop we are incrementing pointer so that it points to
		 * the next element of the array on each increment.
		 *
		 */
		t_array[x] = *time_bin;
		/*increment pointer for next element fetch*/
		time_bin ++;
    	}
	
	// create local array which will indicate which time bin has been changed
	for(int x = 0; x < size; x ++)
    	{
		t_indicator[x] = *time_bin_indicator;
		/*increment pointer for next element fetch*/
		time_bin_indicator ++;
    	}
	
	//The following code goes through the time array to find the largest value and store it in max_value
	int max_value = t_array[0];
	for(int x = 0; x < size; x ++)
    	{
		if(t_array[x] > max_value)
		{
			max_value = t_array[x];
		}			
    	}
	
	/*
	* The following code will halve the size of the graph if the largest bar reaches the top of the OLED screen
	* It does this by halving the number of pixels per frequency unit
	*/
	if(max_value > max_growth_steps)
	{
		int divisor = 2;
		int n = max_value/max_growth_steps;	//this is the exponent for the divisor
		for (int i = 0; i < n-1; i ++){		//self made power function as no such thing in c code
       			divisor *= 2;
		growth_step = growth_step/divisor;	//make the growth step smaller
	}
		
	
	
	
	//0-19 bin 0
	writeCommand(0x22);	//rectangle command
	writeCommand(0x02);	//column start
	writeCommand(0x38 - (growth_step * t_array[0]));	//row start
	writeCommand(0x06);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[0]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[0]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[0]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[0]);	//colour A of fill
	
	//20-39 bin 1
	writeCommand(0x22);	//rectangle command
	writeCommand(0x08);	//column start
	writeCommand(0x38 - (growth_step * t_array[1]));	//row start
	writeCommand(0x0C);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[1]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[1]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[1]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[1]);	//colour A of fill
	
	//40-59 bin 2
	writeCommand(0x22);	//rectangle command
	writeCommand(0x0E);	//column start
	writeCommand(0x38 - (growth_step * t_array[2]));	//row start
	writeCommand(0x12);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[2]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[2]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[2]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[2]);	//colour A of fill
	
	//60-79 bin 3
	writeCommand(0x22);	//rectangle command
	writeCommand(0x14);	//column start
	writeCommand(0x38 - (growth_step * t_array[3]));	//row start
	writeCommand(0x18);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[3]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[3]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[3]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[3]);	//colour A of fill
	
	//80-99 bin 4
	writeCommand(0x22);	//rectangle command
	writeCommand(0x1A);	//column start
	writeCommand(0x38 - (growth_step * t_array[4]));	//row start
	writeCommand(0x1E);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[4]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[4]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[4]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[4]);	//colour A of fill
	
	//100-119 bin 5
	writeCommand(0x22);	//rectangle command
	writeCommand(0x20);	//column start
	writeCommand(0x38 - (growth_step * t_array[5]));	//row start
	writeCommand(0x24);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[5]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[5]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[5]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[5]);	//colour A of fill
	
	//120-139 bin 6
	writeCommand(0x22);	//rectangle command
	writeCommand(0x26);	//column start
	writeCommand(0x38 - (growth_step * t_array[6]));	//row start
	writeCommand(0x2A);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[6]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[6]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[6]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[6]);	//colour A of fill
	
	//140-159 bin 7
	writeCommand(0x22);	//rectangle command
	writeCommand(0x2C);	//column start
	writeCommand(0x38 - (growth_step * t_array[7]));	//row start
	writeCommand(0x30);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[7]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[7]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[7]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[7]);	//colour A of fill
	
	//160-179 bin 8
	writeCommand(0x22);	//rectangle command
	writeCommand(0x32);	//column start
	writeCommand(0x38 - (growth_step * t_array[8]));	//row start
	writeCommand(0x36);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[8]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[8]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[8]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[8]);	//colour A of fill
	
	//180-199 bin 9
	writeCommand(0x22);	//rectangle command
	writeCommand(0x38);	//column start
	writeCommand(0x38 - (growth_step * t_array[9]));	//row start
	writeCommand(0x3C);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[9]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[9]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[9]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[9]);	//colour A of fill
	
	//200-219 bin 10
	writeCommand(0x22);	//rectangle command
	writeCommand(0x3E);	//column start
	writeCommand(0x38 - (growth_step * t_array[10]));	//row start
	writeCommand(0x42);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[10]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[10]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[10]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[10]);	//colour A of fill

	//220-239 bin 11
	writeCommand(0x22);	//rectangle command
	writeCommand(0x44);	//column start
	writeCommand(0x38 - (growth_step * t_array[11]));	//row start
	writeCommand(0x48);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[11]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[11]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[11]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[11]);	//colour A of fill
	
	//240-259 bin 12
	writeCommand(0x22);	//rectangle command
	writeCommand(0x4A);	//column start
	writeCommand(0x38 - (growth_step * t_array[12]));	//row start
	writeCommand(0x4E);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[12]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[12]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[12]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[12]);	//colour A of fill
	
	//260-279 bin 13
	writeCommand(0x22);	//rectangle command
	writeCommand(0x50);	//column start
	writeCommand(0x38 - (growth_step * t_array[13]));	//row start
	writeCommand(0x54);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[13]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[13]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[13]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[13]);	//colour A of fill
	
	//280-299 bin 14
	writeCommand(0x22);	//rectangle command
	writeCommand(0x56);	//column start
	writeCommand(0x38 - (growth_step * t_array[14]));	//row start
	writeCommand(0x5A);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF * t_indicator[14]);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);			//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF * t_indicator[14]);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF * t_indicator[14]);	//colour C of fill
	writeCommand(0x3F);			//colour B of fill
	writeCommand(0xFF * t_indicator[14]);	//colour A of fill
	
	//300+ bin 15
	writeCommand(0x22);	//rectangle command
	writeCommand(0x5C);	//column start
	writeCommand(0x38 - (growth_step * t_array[15]));	//row start
	writeCommand(0x5F);	//column end
	writeCommand(0x38);	//row end
	writeCommand(0xFF);	//colour C (RED) of outline (max FF)
	writeCommand(0x00);	//colour B (GREEN) of outline (max 3F)
	writeCommand(0x00);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF);	//colour C of fill
	writeCommand(0x00);	//colour B of fill
	writeCommand(0x00);	//colour A of fill
	
	return 0;
}


int
devSSD1331_axes(void)
{
	//draw y axis line
	writeCommand(0x21);	//line command
	writeCommand(0x00);	//column start
	writeCommand(0x00);	//row start
	writeCommand(0x00);	//column end
	writeCommand(0x39);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	//draw x axis line with pointers
	writeCommand(0x21);	//line command
	writeCommand(0x00);	//column start
	writeCommand(0x39);	//row start
	writeCommand(0x5F);	//column end
	writeCommand(0x39);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	//0 pointer
	writeCommand(0x21);	//line command
	writeCommand(0x00);	//column start
	writeCommand(0x3A);	//row start
	writeCommand(0x00);	//column end
	writeCommand(0x3A);	//row end
	writeCommand(0x00);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0x00);	//blue colour
	//100 pointer
	writeCommand(0x21);	//line command
	writeCommand(0x1E);	//column start
	writeCommand(0x3A);	//row start
	writeCommand(0x1E);	//column end
	writeCommand(0x3A);	//row end
	writeCommand(0x00);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0x00);	//blue colour
	//200 pointer
	writeCommand(0x21);	//line command
	writeCommand(0x3C);	//column start
	writeCommand(0x3A);	//row start
	writeCommand(0x3C);	//column end
	writeCommand(0x3A);	//row end
	writeCommand(0x00);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0x00);	//blue colour
	//300 pointer
	writeCommand(0x21);	//line command
	writeCommand(0x5A);	//column start
	writeCommand(0x3A);	//row start
	writeCommand(0x5A);	//column end
	writeCommand(0x3A);	//row end
	writeCommand(0x00);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0x00);	//blue colour
	
	//draw 0
	writeCommand(0x21);	//line command
	writeCommand(0x00);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x00);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x02);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x02);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x01);	//column end
	writeCommand(0x3B);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01);	//column start
	writeCommand(0x3F);	//row start
	writeCommand(0x01);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	//draw 100
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x1a);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x00+0x1a);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x1c);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x00+0x1c);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x02+0x1c);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x02+0x1c);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01+0x1c);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x01+0x1c);	//column end
	writeCommand(0x3B);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01+0x1c);	//column start
	writeCommand(0x3F);	//row start
	writeCommand(0x01+0x1c);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x20);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x00+0x20);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x02+0x20);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x02+0x20);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01+0x20);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x01+0x20);	//column end
	writeCommand(0x3B);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01+0x20);	//column start
	writeCommand(0x3F);	//row start
	writeCommand(0x01+0x20);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	//draw 200
		//2
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x37);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x00+0x39);	//column end
	writeCommand(0x3B);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x37);	//column start
	writeCommand(0x3D);	//row start
	writeCommand(0x00+0x39);	//column end
	writeCommand(0x3D);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x37);	//column start
	writeCommand(0x3F);	//row start
	writeCommand(0x00+0x39);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x39);	//column start
	writeCommand(0x3C);	//row start
	writeCommand(0x00+0x39);	//column end
	writeCommand(0x3C);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x37);	//column start
	writeCommand(0x3E);	//row start
	writeCommand(0x00+0x37);	//column end
	writeCommand(0x3E);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
		//0 (tens)
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x3B);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x00+0x3B);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x02+0x3B);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x02+0x3B);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01+0x3B);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x01+0x3B);	//column end
	writeCommand(0x3B);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01+0x3B);	//column start
	writeCommand(0x3F);	//row start
	writeCommand(0x01+0x3B);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
		//0 (ones)
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x3F);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x00+0x3F);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x02+0x3F);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x02+0x3F);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01+0x3F);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x01+0x3F);	//column end
	writeCommand(0x3B);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01+0x3F);	//column start
	writeCommand(0x3F);	//row start
	writeCommand(0x01+0x3F);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	//draw 300
		//3
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x37+0x1E);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x00+0x39+0x1E);	//column end
	writeCommand(0x3B);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x37+0x1E);	//column start
	writeCommand(0x3D);	//row start
	writeCommand(0x00+0x39+0x1E);	//column end
	writeCommand(0x3D);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x37+0x1E);	//column start
	writeCommand(0x3F);	//row start
	writeCommand(0x00+0x39+0x1E);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x39+0x1E);	//column start
	writeCommand(0x3C);	//row start
	writeCommand(0x00+0x39+0x1E);	//column end
	writeCommand(0x3C);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x39+0x1E);	//column start
	writeCommand(0x3E);	//row start
	writeCommand(0x00+0x39+0x1E);	//column end
	writeCommand(0x3E);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
		//0 (tens)
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x59);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x00+0x59);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x02+0x59);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x02+0x59);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01+0x59);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x01+0x59);	//column end
	writeCommand(0x3B);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01+0x59);	//column start
	writeCommand(0x3F);	//row start
	writeCommand(0x01+0x59);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
		//0 (ones)
	writeCommand(0x21);	//line command
	writeCommand(0x00+0x5D);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x00+0x5D);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x02+0x5D);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x02+0x5D);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01+0x5D);	//column start
	writeCommand(0x3B);	//row start
	writeCommand(0x01+0x5D);	//column end
	writeCommand(0x3B);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	writeCommand(0x21);	//line command
	writeCommand(0x01+0x5D);	//column start
	writeCommand(0x3F);	//row start
	writeCommand(0x01+0x5D);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//red colour
	writeCommand(0x3F);	//green colour
	writeCommand(0xFF);	//blue colour
	
	return 0;
}

int
devSSD1331_countdown(void)
{
	writeCommand(kSSD1331CommandCLEAR);
        writeCommand(0x00);
        writeCommand(0x00);
        writeCommand(0x5F);
        writeCommand(0x3F);
	
	//3
	writeCommand(0x22);	//rectangle command
	writeCommand(0x20);	//column start
	writeCommand(0x00);	//row start
	writeCommand(0x3F);	//column end
	writeCommand(0x02);	//row end
	writeCommand(0xFF);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);	//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF);	//colour C of fill
	writeCommand(0x3F);	//colour B of fill
	writeCommand(0xFF);	//colour A of fill
	
	writeCommand(0x22);	//rectangle command
	writeCommand(0x3D);	//column start
	writeCommand(0x00);	//row start
	writeCommand(0x3F);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);	//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF);	//colour C of fill
	writeCommand(0x3F);	//colour B of fill
	writeCommand(0xFF);	//colour A of fill
	
	writeCommand(0x22);	//rectangle command
	writeCommand(0x20);	//column start
	writeCommand(0x1F);	//row start
	writeCommand(0x3F);	//column end
	writeCommand(0x21);	//row end
	writeCommand(0xFF);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);	//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF);	//colour C of fill
	writeCommand(0x3F);	//colour B of fill
	writeCommand(0xFF);	//colour A of fill
	
	writeCommand(0x22);	//rectangle command
	writeCommand(0x20);	//column start
	writeCommand(0x3D);	//row start
	writeCommand(0x3F);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);	//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF);	//colour C of fill
	writeCommand(0x3F);	//colour B of fill
	writeCommand(0xFF);	//colour A of fill
	
	OSA_TimeDelay(700);
	
	writeCommand(kSSD1331CommandCLEAR);
        writeCommand(0x00);
        writeCommand(0x00);
        writeCommand(0x5F);
        writeCommand(0x3F);
	
	OSA_TimeDelay(300);
	
	//2
	writeCommand(0x22);	//rectangle command
	writeCommand(0x20);	//column start
	writeCommand(0x00);	//row start
	writeCommand(0x3F);	//column end
	writeCommand(0x02);	//row end
	writeCommand(0xFF);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);	//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF);	//colour C of fill
	writeCommand(0x3F);	//colour B of fill
	writeCommand(0xFF);	//colour A of fill
	
	writeCommand(0x22);	//rectangle command
	writeCommand(0x3D);	//column start
	writeCommand(0x00);	//row start
	writeCommand(0x3F);	//column end
	writeCommand(0x20);	//row end
	writeCommand(0xFF);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);	//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF);	//colour C of fill
	writeCommand(0x3F);	//colour B of fill
	writeCommand(0xFF);	//colour A of fill
	
	writeCommand(0x22);	//rectangle command
	writeCommand(0x20);	//column start
	writeCommand(0x1F);	//row start
	writeCommand(0x3F);	//column end
	writeCommand(0x21);	//row end
	writeCommand(0xFF);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);	//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF);	//colour C of fill
	writeCommand(0x3F);	//colour B of fill
	writeCommand(0xFF);	//colour A of fill
	
	writeCommand(0x22);	//rectangle command
	writeCommand(0x20);	//column start
	writeCommand(0x20);	//row start
	writeCommand(0x22);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);	//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF);	//colour C of fill
	writeCommand(0x3F);	//colour B of fill
	writeCommand(0xFF);	//colour A of fill
	
	writeCommand(0x22);	//rectangle command
	writeCommand(0x20);	//column start
	writeCommand(0x3D);	//row start
	writeCommand(0x3F);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);	//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF);	//colour C of fill
	writeCommand(0x3F);	//colour B of fill
	writeCommand(0xFF);	//colour A of fill
	
	OSA_TimeDelay(700);
	
	writeCommand(kSSD1331CommandCLEAR);
        writeCommand(0x00);
        writeCommand(0x00);
        writeCommand(0x5F);
        writeCommand(0x3F);
	
	OSA_TimeDelay(300);
	
	//1
	
	writeCommand(0x22);	//rectangle command
	writeCommand(0x3D);	//column start
	writeCommand(0x00);	//row start
	writeCommand(0x3F);	//column end
	writeCommand(0x3F);	//row end
	writeCommand(0xFF);	//colour C (RED) of outline (max FF)
	writeCommand(0x3F);	//colour B (GREEN) of outline (max 3F)
	writeCommand(0xFF);	//colour A (BLUE) of outline (max FF)
	writeCommand(0xFF);	//colour C of fill
	writeCommand(0x3F);	//colour B of fill
	writeCommand(0xFF);	//colour A of fill
	
	OSA_TimeDelay(700);
	
	writeCommand(kSSD1331CommandCLEAR);
        writeCommand(0x00);
        writeCommand(0x00);
        writeCommand(0x5F);
        writeCommand(0x3F);
	
	
	return 0;
}


