/*
 * gpio_en.c
 *
 * Kernel module to enable Bluetooth core on WL1271
 *
 * Copyright(c) 2010 Texas Instruments. All rights reserved.      
 * All rights reserved.                                                  
 *                                                                       
 * Redistribution and use in source and binary forms, with or without    
 * modification, are permitted provided that the following conditions    
 * are met:                                                              
 *                                                                       
 *  * Redistributions of source code must retain the above copyright     
 *    notice, this list of conditions and the following disclaimer.      
 *  * Redistributions in binary form must reproduce the above copyright  
 *    notice, this list of conditions and the following disclaimer in    
 *    the documentation and/or other materials provided with the         
 *    distribution.                                                      
 *  * Neither the name Texas Instruments nor the names of its            
 *    contributors may be used to endorse or promote products derived    
 *    from this software without specific prior written permission.      
 *                                                                       
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT      
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT   
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/wl12xx.h>

#define AM33XX_CTRL_REGADDR(reg)					\
		AM33XX_L4_WK_IO_ADDRESS(AM33XX_SCM_BASE + (reg))

/* bit 3: 0 - enable, 1 - disable for pull enable */
#define AM33XX_PULL_DISA		(1 << 3)
#define AM33XX_PULL_ENBL		(0 << 3)

/* Convert GPIO signal to GPIO pin number */
#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

/* Bluetooth Enable PAD for EVM Rev 1.1 and up */
#define AM33XX_CONTROL_PADCONF_MCASP0_AHCLKX_OFFSET		0x09AC

/* Bluetooth Enable PAD for EVM Rev 1.0 */
#define AM33XX_CONTROL_PADCONF_GPMC_CSN2_OFFSET			0x0884

const struct wl12xx_platform_data *wl12xx_get_platform_data(void);

unsigned char bt_enable_pin;
int selected_pad;
int pad_mux_value;

static int __init gpio_test_init(void)
{
	const struct wl12xx_platform_data *wlan_data;
	wlan_data = wl12xx_get_platform_data();

	bt_enable_pin = wlan_data->bt_enable_gpio;

	/* Default to alpha EVM in case the element is not initialized */
	if (! bt_enable_pin)
	{
		pr_info("BT Enable pin is not initialized, defaulting to EVM Rev 1.0A.\n");
		bt_enable_pin = GPIO_TO_PIN(1, 31); 
	}

	/* Select pad conf register based on EVM board rev */
	if ( bt_enable_pin == GPIO_TO_PIN(3, 21) )	
		selected_pad = AM33XX_CONTROL_PADCONF_MCASP0_AHCLKX_OFFSET;
	else
		selected_pad = AM33XX_CONTROL_PADCONF_GPMC_CSN2_OFFSET;
 
	printk("Gpio value is :%d\n", bt_enable_pin);
	gpio_direction_output(bt_enable_pin, 0);
	msleep(1);
	printk("WL1271: BT Enable\n");
	gpio_direction_output(bt_enable_pin, 1);

	/* Enable pullup on the enable pin for keeping BT active during suspend */
	pad_mux_value = readl(AM33XX_CTRL_REGADDR(selected_pad));
	pad_mux_value &= (~AM33XX_PULL_DISA);
	writel(pad_mux_value, AM33XX_CTRL_REGADDR(selected_pad));

	return 0;
}

static void __exit gpio_test_exit(void)
{
	printk("WL1271: BT Disable\n");
        gpio_direction_output(bt_enable_pin, 0);

	/* Disable pullup on the enable pin to allow BT shut down during suspend */
	pad_mux_value = readl(AM33XX_CTRL_REGADDR(selected_pad));
	pad_mux_value |= AM33XX_PULL_DISA;
	writel(pad_mux_value, AM33XX_CTRL_REGADDR(selected_pad));
}



module_init(gpio_test_init);
module_exit(gpio_test_exit);
MODULE_LICENSE("GPL");
