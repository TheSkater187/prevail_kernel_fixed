/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <mach/irqs.h>
#include <linux/spinlock.h>
#include <mach/gpio.h>
#include "msm_fb.h"

// quattro_jiny46kim
#include <mach/vreg.h>

#define LCDC_DEBUG

#if defined(CONFIG_MACH_VINO) || defined(CONFIG_MACH_GIOS)
#define FEATURE_LCD_ESD_DET
#else
//#define FEATURE_LCD_ESD_DET
#endif

#ifdef LCDC_DEBUG
#define DPRINT(x...)	printk("s6d04d1 " x)
#else
#define DPRINT(x...)	
#endif

#define GPIO_BL_CTRL	26

extern unsigned char hw_version;

#ifdef FEATURE_LCD_ESD_DET
#if defined(CONFIG_MACH_RANT3)
#define GPIO_ESD_DET 		((hw_version>=2)?(91):(94))
#elif defined(CONFIG_MACH_VINO) || defined(CONFIG_MACH_GIOS)
#define GPIO_ESD_DET 		((hw_version>2)?(91):(94))
#define USED_ESD_DETET  ((hw_version>=5)?(1):(0))
static struct timer_list g_lcd_esd_timer;
static int esd_repeat_count = 0;
#define ESD_REPEAT_MAX    3
struct delayed_work lcd_esd_work;
void s6d04d1_esd(void);
#else
#define GPIO_ESD_DET 	94
#endif
#endif


/*
 * Serial Interface
 */
#define LCD_CSX_HIGH	gpio_set_value(spi_cs, 1);
#define LCD_CSX_LOW		gpio_set_value(spi_cs, 0);

#define LCD_SCL_HIGH	gpio_set_value(spi_sclk, 1);
#define LCD_SCL_LOW		gpio_set_value(spi_sclk, 0);

#define LCD_SDI_HIGH	gpio_set_value(spi_sdi, 1);
#define LCD_SDI_LOW		gpio_set_value(spi_sdi, 0);

#define DEFAULT_USLEEP	1	
#define DEFAUTL_NSLEEP 110
#define PWRCTL			0xF3
#define VCMCTL			0xF4
#define SRCCTL			0xF5
#define SLPOUT			0x11
#define MADCTL			0x36
#define COLMOD			0x3A
#define DISCTL			0xF2
#define PGAMCTL			0xF7
#define IFCTL			0xF6
#define GATECTL			0xFD
#define WRDISBV			0x51
#define WRCABCMB		0x5E
#define MIECTL1			0xCA
#define BCMODE			0xCB
#define MIECTL2			0xCC
#define MIDCTL3			0xCD
#define RNGAMCTL		0xF8
#define GPGAMCTL		0xF9
#define GNGAMCTL		0xFA
#define BPGAMCTL		0xFB
#define BNGAMCTL		0xFC
#define CASET			0x2A
#define PASET			0x2B
#define RAMWR           0x2C
#define WRCTRLD			0x53
#define WRCABC			0x55
#define DISPON			0x29
#define DISPOFF			0x28
#define SLPIN			0x10
#define DCON				0xD9
#define WRPWD			0xF0
#define EDSTEST			0xFF
#define TEON			      0x35

struct setting_table {
	unsigned char command;	
	unsigned char parameters;
	unsigned char parameter[34];


	long wait;
};

static struct setting_table power_on_setting_table[] = {
    //Sleep out
    { 0x11, 0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }, 10 },
    //Power Sequence 
    { 0xEF, 2, { 0x74, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0xF1, 1, { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0xB1, 9, { 0x01, 0x00, 0x22, 0x11, 0x73, 0x70, 0xEC, 0x15, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0xB2, 8, { 0x66, 0x06, 0xAA, 0x88, 0x88, 0x08, 0x08, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    //Initializing Sequence
    { 0xB4, 5, { 0x10, 0x00, 0x32, 0x32, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0xB6, 8, { 0x30, 0x66, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0xD5, 3, { 0x02, 0x43, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0x36, 1, { 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0x3A, 1, { 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0xF2, 6, { 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    // Gamma setting Sequence
    { 0xE0, 34, { 0x17, 0x13, 0x1F, 0x10, 0x20, 0x35, 0x29, 0x2A, 0x06, 0xC7, 0x8B, 0xCD, 0x12, 0x94, 0x92, 0x0E, 0x16,\
      0x00, 0x08, 0x06, 0x0C, 0x1C, 0x3D, 0x0E, 0x20, 0x0B, 0xD5, 0x98, 0x1D, 0x1F, 0x1B, 0x9C, 0x14, 0x17},  0 },
    { 0xE1, 34, { 0x17, 0x13, 0x1F, 0x10, 0x20, 0x35, 0x29, 0x2A, 0x06, 0xC7, 0x8B, 0xCD, 0x12, 0x94, 0x92, 0x0E, 0x16,\
      0x00, 0x08, 0x06, 0x0C, 0x1C, 0x3D, 0x0E, 0x20, 0x0B, 0xD5, 0x98, 0x1D, 0x1F, 0x1B, 0x9C, 0x14, 0x17},  0 },
    { 0xE3, 34, { 0x17, 0x13, 0x1F, 0x10, 0x20, 0x35, 0x29, 0x2A, 0x06, 0xC7, 0x8B, 0xCD, 0x12, 0x94, 0x92, 0x0E, 0x16,\
      0x00, 0x08, 0x06, 0x0C, 0x1C, 0x3D, 0x0E, 0x20, 0x0B, 0xD5, 0x98, 0x1D, 0x1F, 0x1B, 0x9C, 0x14, 0x17},  0 },
    //Display On
    { 0x29, 0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }, 0 },
};

static struct setting_table power_on_setting_table_rev05[] = {
    //Sleep out
    {  0x11, 0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }, 10 },
    //Power Sequence 
    { 0xEF, 2, { 0x74, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0xF2, 6, { 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 }, 
    { 0xB1, 9, { 0x01, 0x00, 0x22, 0x11, 0x73, 0x70, 0xEC, 0x15, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0xB2, 8, { 0x66, 0x06, 0xAA, 0x88, 0x88, 0x08, 0x08, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    //Initializing Sequence
    { 0xB4, 5, { 0x10, 0x00, 0x32, 0x32, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0xB6, 8, { 0x30, 0x66, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0xD5, 3, { 0x02, 0x43, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0x36, 1, { 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0x3A, 1, { 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    { 0xF3, 2, { 0x10, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },  0 },
    // Gamma setting Sequence
    { 0xE0, 34, {0x22, 0x1C, 0x21, 0x10, 0x20, 0x37, 0x2E, 0x2D, 0x0B, 0xCA, 0x87, 0xCC, 0x10, 0x8F, 0x8D, 0x01, 0x16,\
     0x0A, 0x16, 0x14, 0x0C, 0x1C, 0x3C, 0x13, 0x23, 0x08, 0xD4, 0x94, 0x1C, 0x1F, 0x1D, 0x9D, 0x0F, 0x17},  0 },
    { 0xE1, 34, { 0x22, 0x1C, 0x21, 0x10, 0x20, 0x37, 0x2B, 0x2B, 0x0A, 0xC9, 0x85, 0xCB, 0x0F, 0x8E, 0x8D, 0x02, 0x16,\
      0x0A, 0x16, 0x14, 0x0C, 0x1C, 0x3C, 0x10, 0x21, 0x07, 0xD3, 0x98, 0x1B, 0x1E, 0x1C, 0x9D, 0x10, 0x17},  0 },
    { 0xE2, 34, { 0x22, 0x1C, 0x21, 0x10, 0x20, 0x37, 0x2B, 0x2B, 0x0A, 0xC9, 0x85, 0xCB, 0x0F, 0x8E, 0x8D, 0x02, 0x16,\
      0x0A, 0x16, 0x14, 0x0C, 0x1C, 0x3C, 0x10, 0x21, 0x07, 0xD3, 0x98, 0x1B, 0x1E, 0x1C, 0x9D, 0x10, 0x17},  0 },
    //Display On
    {	0x29, 0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },	25 },
};

#define POWER_ON_SETTINGS (int)((hw_version>=5&&hw_version < 90)?(sizeof(power_on_setting_table_rev05)/sizeof(struct setting_table)):(sizeof(power_on_setting_table)/sizeof(struct setting_table)))//

static struct setting_table power_off_setting_table[] = {
	{ DISPOFF,   0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },
	{ SLPIN,   0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 100 },
};

static struct setting_table power_off_setting_table_rev05[] = {
	{ DISPOFF,   0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },
  //{ SLPIN,   0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 100 },
	{ 0xDE,   1, { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 120 },
};
#if defined(FEATURE_LCD_ESD_DET)
static struct setting_table power_off_setting_table_rev05_temp[] = {
	{ DISPOFF,   0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },
  //{ SLPIN,   0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 100 },
	{ 0xDE,   1, { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },
};
#endif

#define POWER_OFF_SETTINGS (int)(sizeof(power_off_setting_table)/sizeof(struct setting_table))

static int lcdc_s6d04d1_panel_off(struct platform_device *pdev);
#ifdef FEATURE_LCD_ESD_DET
static int phone_power_off = 0;
static void s6d04d1_esd_irq_handler_cancel(void);
static irqreturn_t s6d04d1_esd_irq_handler(int irq, void *handle);
#endif

static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int lcd_en;
static int lcd_reset;

static char lcd_brightness = 0;

/* To synchronize between gp2a and platform to control backlight */
spinlock_t lcd_lock;
/* to store platform bl level */
int app_bl_level;
static void control_backlight(int bl_level );

#if 0
static unsigned char bit_shift[8] = { (1 << 7),	/* MSB */
	(1 << 6),
	(1 << 5),
	(1 << 4),
	(1 << 3),
	(1 << 2),
	(1 << 1),
	(1 << 0)		               /* LSB */
};
#endif

// quattro_jiny46kim
struct vreg *vreg_ldo14;
struct vreg *vreg_ldo15;


struct s6d04d1_state_type{
	boolean disp_initialized;
	boolean display_on;
	boolean disp_powered_up;
	boolean panel_initialized;
#ifdef FEATURE_LCD_ESD_DET
	boolean irq_disabled;
#endif
};


static void s6d0d1_esd_timer_handler(unsigned long data);
static struct s6d04d1_state_type s6d04d1_state = { 0 };
static struct msm_panel_common_pdata *lcdc_s6d04d1_pdata;

static void setting_table_write(struct setting_table *table)
{
	long i, j;

	LCD_CSX_HIGH
	ndelay(DEFAUTL_NSLEEP);
	LCD_SCL_HIGH 
	ndelay(DEFAUTL_NSLEEP);

	/* Write Command */
	LCD_CSX_LOW
	ndelay(DEFAUTL_NSLEEP);
	LCD_SCL_LOW 
	ndelay(DEFAUTL_NSLEEP);		
	LCD_SDI_LOW 
	ndelay(DEFAUTL_NSLEEP);
	
	LCD_SCL_HIGH 
	ndelay(DEFAUTL_NSLEEP); 

   	for (i = 7; i >= 0; i--) { 
		LCD_SCL_LOW
		ndelay(DEFAUTL_NSLEEP);
		if ((table->command >> i) & 0x1)
			LCD_SDI_HIGH
		else
			LCD_SDI_LOW
		ndelay(DEFAUTL_NSLEEP);	
		LCD_SCL_HIGH
		ndelay(DEFAUTL_NSLEEP);	
	}

	LCD_CSX_HIGH
	ndelay(DEFAUTL_NSLEEP);	

	/* Write Parameter */
	if ((table->parameters) > 0) {
	for (j = 0; j < table->parameters; j++) {
		LCD_CSX_LOW 
		ndelay(DEFAUTL_NSLEEP); 	
		
		LCD_SCL_LOW 
		ndelay(DEFAUTL_NSLEEP); 	
		LCD_SDI_HIGH 
		ndelay(DEFAUTL_NSLEEP);
		LCD_SCL_HIGH 
		ndelay(DEFAUTL_NSLEEP); 	

		for (i = 7; i >= 0; i--) { 
			LCD_SCL_LOW
			ndelay(DEFAUTL_NSLEEP);	
			if ((table->parameter[j] >> i) & 0x1)
				LCD_SDI_HIGH
			else
				LCD_SDI_LOW
			ndelay(DEFAUTL_NSLEEP);	
			LCD_SCL_HIGH
			ndelay(DEFAUTL_NSLEEP);					
		}

			LCD_CSX_HIGH
			ndelay(DEFAUTL_NSLEEP);	
	}
	}

    if(table->wait)
	msleep(table->wait);
}

static void spi_init(void)
{
	/* Setting the Default GPIO's */
	spi_sclk = *(lcdc_s6d04d1_pdata->gpio_num);
	spi_cs   = *(lcdc_s6d04d1_pdata->gpio_num + 1);
	spi_sdi  = *(lcdc_s6d04d1_pdata->gpio_num + 2);
	lcd_en   = *(lcdc_s6d04d1_pdata->gpio_num + 3);
	lcd_reset= *(lcdc_s6d04d1_pdata->gpio_num + 4);
//	spi_sdo  = *(lcdc_s6d04d1_pdata->gpio_num + 3);

	/* Set the output so that we dont disturb the slave device */
	gpio_set_value(spi_sclk, 0);
	gpio_set_value(spi_sdi, 0);

	/* Set the Chip Select De-asserted */
	gpio_set_value(spi_cs, 0);
}

static void s6d04d1_disp_powerup(void)
{
	int i; // quattro_jiny46kim [[
	DPRINT("start %s\n", __func__);	

	if (!s6d04d1_state.disp_powered_up && !s6d04d1_state.display_on) 
	{
		/* Reset the hardware first */

		if (hw_version < 5 ||hw_version == 90) 
		{
		   msleep(10);
			gpio_set_value(lcd_reset, 0);
			udelay(10);
			gpio_set_value(lcd_reset, 1);
			msleep(5);
		}
		else
		{
			gpio_set_value(lcd_reset, 1);
			msleep(10);
			gpio_set_value(lcd_reset, 0);
			udelay(50);
			gpio_set_value(lcd_reset, 1);
			msleep(10);
		}

		/* Include DAC power up implementation here */		
		s6d04d1_state.disp_powered_up = TRUE;
	}
}

static void s6d04d1_disp_powerdown(void)
{
	int i ; // quattro_jiny46kim [[

	DPRINT("start %s\n", __func__);	


// fmt shin moon young, May 2nd, 2011

    /* Reset Assert */
 gpio_set_value(lcd_reset, 0); 
  LCD_CSX_LOW

// fmt shin moon young, May 2nd, 2011

  LCD_SCL_LOW
  LCD_SDI_LOW
	s6d04d1_state.disp_powered_up = FALSE;
}

static void s6d04d1_disp_on(void)
{
	int i;

	DPRINT("start %s\n", __func__);	

	if (s6d04d1_state.disp_powered_up && !s6d04d1_state.display_on) {
	
		/* s6d04d1 setting */

    if ((hw_version<5)||(hw_version==90)) {
      for (i = 0; i < POWER_ON_SETTINGS; i++)
        setting_table_write(&power_on_setting_table[i]);
    }
    else {
      for (i = 0; i < POWER_ON_SETTINGS; i++)
        setting_table_write(&power_on_setting_table_rev05[i]);
    }

		s6d04d1_state.display_on = TRUE;
	}
}

static int lcdc_s6d04d1_panel_on(struct platform_device *pdev)
{
  static int first_kernel_enter_flag = 0;
  int i;

  DPRINT("start %s\n", __func__);	

  if(first_kernel_enter_flag == 0)
  {
    lcdc_s6d04d1_pdata->panel_config_gpio(1);

    /* Control LCD_D(23) for sleep current */
    gpio_tlmm_config(GPIO_CFG(100, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

    spi_init();	/* LCD needs SPI */	

    s6d04d1_state.disp_initialized = TRUE;
    s6d04d1_state.display_on = TRUE;
    s6d04d1_state.disp_powered_up = TRUE;

    first_kernel_enter_flag = 1;
  }
  else
  {
    if (!s6d04d1_state.disp_initialized) 
    {
      lcdc_s6d04d1_pdata->panel_config_gpio(1);

      /* Control LCD_D(23) for sleep current */
      gpio_tlmm_config(GPIO_CFG(100, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
      spi_init();	/* LCD needs SPI */
      s6d04d1_disp_powerup();
      s6d04d1_disp_on();

#ifdef FEATURE_LCD_ESD_DET
      if (s6d04d1_state.irq_disabled && USED_ESD_DETET) 
      {
        enable_irq(MSM_GPIO_TO_INT(GPIO_ESD_DET));
        s6d04d1_state.irq_disabled = FALSE;
      }
#endif

      s6d04d1_state.disp_initialized = TRUE;
    }
  }

  return 0;
}

static int lcdc_s6d04d1_panel_off(struct platform_device *pdev)
{
	int i;

	DPRINT("start %s\n", __func__);	

	// TEMP CODE for BLU
	gpio_set_value(GPIO_BL_CTRL, 0);
	lcd_brightness = 0;

	if (s6d04d1_state.disp_powered_up && s6d04d1_state.display_on) 
	{

		if (USED_ESD_DETET) 
		{
		    if( !s6d04d1_state.irq_disabled && !phone_power_off)
		    {  
		    	disable_irq(MSM_GPIO_TO_INT(GPIO_ESD_DET));
		    	s6d04d1_state.irq_disabled = TRUE;
		    }
		}

		if (hw_version<5 || hw_version == 90) {
			for (i = 0; i < POWER_OFF_SETTINGS; i++)
				setting_table_write(&power_off_setting_table[i]); 
		}
		else {
			for (i = 0; i < POWER_OFF_SETTINGS; i++)
          {   
#ifdef FEATURE_LCD_ESD_DET
				if( phone_power_off == 1 )
				{
					setting_table_write(&power_off_setting_table_rev05_temp[i]); 
				}
				else
#endif
				{
					setting_table_write(&power_off_setting_table_rev05[i]); 
				}
           }
		}


		s6d04d1_disp_powerdown();
		
		lcdc_s6d04d1_pdata->panel_config_gpio(0);
		s6d04d1_state.display_on = FALSE;
		s6d04d1_state.disp_initialized = FALSE;
		
		/* Control LCD_D(23) for sleep current */
		gpio_tlmm_config(GPIO_CFG(100, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

#ifdef FEATURE_LCD_ESD_DET
    disable_irq(MSM_GPIO_TO_INT(GPIO_ESD_DET));
    s6d04d1_state.irq_disabled = TRUE;
#endif	

  }
  return 0;
}

#if 1
#define MAX_BRIGHTNESS_IN_BLU	32
#define PULSE_CAL(x) (((x)==0) ? MAX_BRIGHTNESS_IN_BLU:(MAX_BRIGHTNESS_IN_BLU -((x)*2) + 1))

/*controlling back light from both platform and gp2a (light sensor) */
static void control_backlight(int bl_level )
{
    int pulse,pulse2;
    unsigned long flag;

    /* lock necessary in order control lcd backlight from gp2a light sensor and platform code */
//    spin_lock(&lcd_lock);

    DPRINT("%s bl_level = %d , lcd_brightness = %d\n", __func__, bl_level, lcd_brightness); 
    if(!bl_level || lcd_brightness <= 0)            // reduce log msg
      DPRINT("start %s:%d\n", __func__, bl_level);

    if (bl_level <= 0)
    {
      /* keep back light OFF */
      
      gpio_set_value(GPIO_BL_CTRL, 0);
      mdelay(5);              //guaranteed shutdown
    }
    else
    {    
      if(lcd_brightness != bl_level)
      {
        if(bl_level > 255)
          bl_level = 255;

        pulse = (MAX_BRIGHTNESS_IN_BLU - (bl_level/8)) %(MAX_BRIGHTNESS_IN_BLU+1);
        // hw 
        if( pulse <=3 ) pulse = 3;

        if(!lcd_brightness)
        {
            gpio_set_value(GPIO_BL_CTRL, 0);
            udelay(1);
        }
        spin_lock_irqsave(&lcd_lock,flag); // minimize the spin lock time
        for( ; pulse >0 ; pulse--)
        {
              gpio_set_value(GPIO_BL_CTRL, 0);
              udelay(1);
              gpio_set_value(GPIO_BL_CTRL, 1);
              udelay(1);
        }
        spin_unlock_irqrestore(&lcd_lock,flag);        
        //udelay(500);

     }
    }
    lcd_brightness = bl_level;

    /* unlock the baklight control */
//    spin_unlock(&lcd_lock);    
}


static void lcdc_s6d04d1_set_backlight(struct msm_fb_data_type *mfd)
{
		int bl_level = mfd->bl_level;
		/* Value necessary for controlling backlight in case light sensor gp2a is off */
		app_bl_level = bl_level;

		int pulse;
		
		/* To control back light from both platform and gp2a (light sensor) */
		control_backlight(bl_level);

/* Below  code is moved to separate routine(control_backlight) since input parameters does not match 
so that it can be controlled by both platform and gp2a light sensor */
#if 0

		DPRINT("lcdc_s6d04d1_set_backlight!!!\n");	
		DPRINT("bl_level = %d , lcd_brightness = %d\n", bl_level, lcd_brightness);	
   
		if(!bl_level || lcd_brightness <= 0)		// reduce log msg
			DPRINT("start %s:%d\n", __func__, bl_level);	

		if (bl_level <= 0) 
		{
			/* keep back light OFF */
			gpio_set_value(GPIO_BL_CTRL, 0);
			mdelay(3);		//guaranteed shutdown
		}
		else 
		{
		  #if 0
			pulse = MAX_BRIGHTNESS_IN_BLU - bl_level;
			
			gpio_set_value(GPIO_BL_CTRL, 0);
			mdelay(2);

			gpio_set_value(GPIO_BL_CTRL, 1);
			udelay(1);

			for(; pulse>0; pulse--)
			{
				gpio_set_value(GPIO_BL_CTRL, 0);
				udelay(1);
				gpio_set_value(GPIO_BL_CTRL, 1);
				udelay(1);
			}
			#else

        if(lcd_brightness > bl_level) //light down
        {
          pulse = PULSE_CAL(bl_level) - PULSE_CAL(lcd_brightness) ;
        }
        else if(lcd_brightness < bl_level) //light up
        {
          pulse = MAX_BRIGHTNESS_IN_BLU - PULSE_CAL(lcd_brightness) + PULSE_CAL(bl_level);
        }
        else
        { 
          pulse = MAX_BRIGHTNESS_IN_BLU;
        }

        DPRINT("bl_level = %d , lcd_brightness = %d, pulse=%d\n", bl_level, lcd_brightness,pulse);	
        
        for(;pulse>0;pulse--)
        {
          gpio_set_value(GPIO_BL_CTRL, 0);
          udelay(5);
          gpio_set_value(GPIO_BL_CTRL, 1);
          udelay(5);
        }
			#endif
		}
		lcd_brightness = bl_level;
#endif
}

/* Auto brightness to be controlled by gp2a driver in case Auto brightess in enabled */
void lcdc_s6d04d1_set_backlight_autobrightness(int bl_level)
{
	DPRINT("%s\n", __func__);	

	/* Control backlight routine */             
	control_backlight(bl_level);
}
#endif

static int __init s6d04d1_probe(struct platform_device *pdev)
{
	int err;
	DPRINT("start %s\n", __func__);	

#ifdef FEATURE_LCD_ESD_DET
	if( hw_version > 2 )
	{
		gpio_tlmm_config(GPIO_CFG(GPIO_ESD_DET,  0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
    init_timer(&g_lcd_esd_timer);
    g_lcd_esd_timer.function = s6d0d1_esd_timer_handler;

    INIT_DELAYED_WORK(&lcd_esd_work, s6d04d1_esd);	
	err = request_irq(MSM_GPIO_TO_INT(GPIO_ESD_DET), s6d04d1_esd_irq_handler, IRQF_TRIGGER_RISING,
		"LCD_ESD_DET", (void*)pdev->dev.platform_data);

	if (err) {
		DPRINT("%s, request_irq failed %d(ESD_DET), ret= %d\n", __func__, GPIO_ESD_DET, err);    
	}  
#endif

	if (pdev->id == 0) {
		lcdc_s6d04d1_pdata = pdev->dev.platform_data;
		/*  LCD SPI GPIO early init. */
		spi_sclk = *(lcdc_s6d04d1_pdata->gpio_num);
		spi_cs   = *(lcdc_s6d04d1_pdata->gpio_num + 1);
		spi_sdi  = *(lcdc_s6d04d1_pdata->gpio_num + 2);
		lcd_en   = *(lcdc_s6d04d1_pdata->gpio_num + 3);
		lcd_reset= *(lcdc_s6d04d1_pdata->gpio_num + 4);
		return 0;
	}
	msm_fb_add_device(pdev);
	return 0;
}


static void s6d04d1_shutdown(struct platform_device *pdev)
{
	DPRINT("start %s\n", __func__);	

#ifdef FEATURE_LCD_ESD_DET
  phone_power_off = 1;
  s6d04d1_esd_irq_handler_cancel();
  if( pdev )
  {
	free_irq(MSM_GPIO_TO_INT(GPIO_ESD_DET),(void*)pdev->dev.platform_data);
  }
#endif

	lcdc_s6d04d1_panel_off(pdev);

}

static struct platform_driver this_driver = {
	.probe  = s6d04d1_probe,
	.shutdown	= s6d04d1_shutdown,
	.driver = {
		.name   = "lcdc_s6d04d1_wqvga",
	},
};

static struct msm_fb_panel_data s6d04d1_panel_data = {
	.on = lcdc_s6d04d1_panel_on,
	.off = lcdc_s6d04d1_panel_off,
	.set_backlight = lcdc_s6d04d1_set_backlight,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel",
	.id	= 1,
	.dev	= {
		.platform_data = &s6d04d1_panel_data,
	}
};

#define LCDC_PIXELCLK    40 \
					* (LCDC_HFP+LCDC_HPW+LCDC_HBP+LCDC_FB_XRES) \
					* (LCDC_VFP+LCDC_VPW+LCDC_VBP+LCDC_FB_YRES)

#define LCDC_FB_XRES     320
#define LCDC_FB_YRES    480

#define LCDC_HBP		32
#define LCDC_HPW	16
#define LCDC_HFP		12

#define LCDC_VBP	7
#define LCDC_VPW	4
#define LCDC_VFP		8 

static int __init lcdc_s6d04d1_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

	s6d04d1_state.panel_initialized = TRUE;
#ifdef CONFIG_FB_MSM_TRY_MDDI_CATCH_LCDC_PRISM
	if (msm_fb_detect_client("lcdc_s6d04d1_wqvga"))
	{
		printk(KERN_ERR "%s: msm_fb_detect_client failed!\n", __func__); 
		return 0;
	}
#endif
	DPRINT("start %s\n", __func__);	


	/* used for controlling control backilght code from gp2a light sensor and platform code */
	spin_lock_init(&lcd_lock);


	ret = platform_driver_register(&this_driver);
	if (ret)
	{
		printk(KERN_ERR "%s: platform_driver_register failed! ret=%d\n", __func__, ret); 
		return ret;
	}

	pinfo = &s6d04d1_panel_data.panel_info;
	pinfo->xres = LCDC_FB_XRES;
	pinfo->yres = LCDC_FB_YRES;
	pinfo->height = 69; //in mm for DPI calibration on platform
	pinfo->width = 42; //in mm for DPI calibration on platform
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 24;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 15124000 ; //24576000; //8192000; //LCDC_PIXELCLK ;//7500000;//8192000;
	pinfo->bl_max = 255;//29; //Max brightness of LCD
	pinfo->bl_min = 1;
	pinfo->lcdc.h_back_porch = LCDC_HBP;
	pinfo->lcdc.h_front_porch = LCDC_HFP;
	pinfo->lcdc.h_pulse_width = LCDC_HPW;
	pinfo->lcdc.v_back_porch = LCDC_VBP;
	pinfo->lcdc.v_front_porch = LCDC_VFP;
	pinfo->lcdc.v_pulse_width = LCDC_VPW;
	pinfo->lcdc.border_clr = 0;     /* blk */
	pinfo->lcdc.underflow_clr = 0xff;       /* blue */
	pinfo->lcdc.hsync_skew = 0;

	printk("%s\n", __func__);

	ret = platform_device_register(&this_device);
	if (ret)
	{
		printk(KERN_ERR "%s: platform_device_register failed! ret=%d\n", __func__, ret); 
		platform_driver_unregister(&this_driver);
	}

	s6d04d1_state.panel_initialized = FALSE;
	return ret;
}

#ifdef FEATURE_LCD_ESD_DET
extern void tsp_detect_esd_wrok(void);
void s6d04d1_esd(void)
{
	DPRINT("start %s, panel_initialized: %d,disp_initialized: %d\n", 
      __func__, s6d04d1_state.panel_initialized, s6d04d1_state.disp_initialized);
	if (!s6d04d1_state.panel_initialized ) {
      if(  gpio_get_value(GPIO_ESD_DET) == 1 )
      {
        char backup_brightness = lcd_brightness; 

        lcdc_s6d04d1_panel_off(NULL);
        mdelay(20);
        lcdc_s6d04d1_panel_on(NULL);
        mdelay(3);
        control_backlight(backup_brightness);

        tsp_detect_esd_wrok();
       }

      esd_repeat_count = 0;
      del_timer(&g_lcd_esd_timer);
      g_lcd_esd_timer.expires = get_jiffies_64() + msecs_to_jiffies(0); // 800 -> 0  sleep issue cause by rapidly lcd on/off. h/w request
      add_timer(&g_lcd_esd_timer);
	}
}


static irqreturn_t s6d04d1_esd_irq_handler(int irq, void *handle)
{
  DPRINT("start %s (display_on=%d,disp_initialized=%d)\n", __func__, 
      s6d04d1_state.display_on, s6d04d1_state.disp_initialized);

  if( USED_ESD_DETET  /*&& s6d04d1_state.disp_initialized*/ && !s6d04d1_state.irq_disabled)
  {
    //schedule_work(&lcd_esd_work);  
    schedule_delayed_work(&lcd_esd_work, 0);// 100 -> 0  sleep issue cause by rapidly lcd on/off. h/w request
  }

  return IRQ_HANDLED;
}

static void s6d04d1_esd_irq_handler_cancel(void)
{
  cancel_work_sync(&lcd_esd_work);
}


static void s6d0d1_esd_timer_handler(unsigned long data)
{
  char backup_brightness = lcd_brightness; 

  // reset lcd 
  if( gpio_get_value(GPIO_ESD_DET) == 1 )
  {
    schedule_delayed_work(&lcd_esd_work, 0); // 100 -> 0  sleep issue cause by rapidly lcd on/off . h/w request
  }
  else
  {
    esd_repeat_count += 1;
    if( esd_repeat_count > ESD_REPEAT_MAX)
    {
      esd_repeat_count = 0;
    }
    else
    {
      g_lcd_esd_timer.expires = get_jiffies_64() + msecs_to_jiffies(0); // 800 -> 0  sleep issue cause by rapidly lcd on/off. h/w request
      add_timer(&g_lcd_esd_timer);
    }
  }
}
#endif

module_init(lcdc_s6d04d1_panel_init);
