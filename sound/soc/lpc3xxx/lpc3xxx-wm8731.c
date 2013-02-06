/*
 * sound/soc/lpc3xxx/lpc3xxx-wm8731.c
 *
 * Author: Antonio Galea <antonio.galea@gmail.com>
 *
 * Based on:
 *   sound/soc/atmel/sam9g20_wm8731.c
 *   sound/soc/lpc3xxx/lpc3xxx-uda1380.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/gpio.h>

#include "../codecs/wm8731.h"
#include "lpc3xxx-pcm.h"
#include "lpc3xxx-i2s.h"

#define SND_MODNAME "arm9facile_wm8731"

#define MCLK_RATE 12288000

static int arm9facile_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned int clk = 0;
	int ret = 0;

	switch (params_rate(params)) {
	case 8000:
	case 16000:
	case 48000:
	case 96000:
		clk = 12288000;
		break;
	case 11025:
	case 22050:
	case 44100:
		clk = 11289600;
		break;
	}

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	/* set the codec system clock for DAC and ADC */
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8731_SYSCLK_XTAL, clk,
		SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops arm9facile_wm8731_ops = {
	.hw_params = arm9facile_hw_params,
};

static const struct snd_soc_dapm_widget arm9facile_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
        SND_SOC_DAPM_MIC("Mic Jack", NULL),
};

static const struct snd_soc_dapm_route intercon[] = {
	/* headphone connected to LHPOUT, RHPOUT */
	{"Headphone Jack", NULL, "LHPOUT"},
	{"Headphone Jack", NULL, "RHPOUT"},
	{"MICIN", NULL, "Mic Jack"},
};

/*
 * Logic for a wm8731 as connected on a arm9facile board.
 */
static int arm9facile_wm8731_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int ret;

	printk(KERN_DEBUG
			"arm9facile_wm8731 "
			": arm9facile_wm8731_init() called\n");

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8731_SYSCLK_XTAL,
		MCLK_RATE, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		printk(KERN_ERR "Failed to set WM8731 SYSCLK: %d\n", ret);
		return ret;
	}

        snd_soc_dapm_nc_pin(dapm, "LLINEIN");
        snd_soc_dapm_nc_pin(dapm, "RLINEIN");
        snd_soc_dapm_nc_pin(dapm, "LOUT");
        snd_soc_dapm_nc_pin(dapm, "ROUT");
        snd_soc_dapm_enable_pin(dapm, "MICIN");

	/* Add specific widgets */
	snd_soc_dapm_new_controls(dapm, arm9facile_dapm_widgets,
				  ARRAY_SIZE(arm9facile_dapm_widgets));
	/* Set up specific audio path interconnects */
	snd_soc_dapm_add_routes(dapm, intercon, ARRAY_SIZE(intercon));

	snd_soc_dapm_sync(dapm);

	return 0;
}

static struct snd_soc_dai_link arm9facile_wm8731_dai[] = {
        {
		.name           = "WM8731",
		.stream_name    = "WM8731 PCM",
		.cpu_dai_name   = "lpc3xxx-i2s0",
		.codec_dai_name = "wm8731-hifi",
		.init           = arm9facile_wm8731_init,
		.platform_name	= "lpc3xxx-audio.0",
		.codec_name     = "wm8731-codec.0-001b",
		.ops            = &arm9facile_wm8731_ops,
        },
};

static struct snd_soc_card snd_soc_arm9facile = {
	.name      = "LPC32XX",
	.dai_link  = &arm9facile_wm8731_dai[0],
	.num_links = ARRAY_SIZE(arm9facile_wm8731_dai),
};

static struct platform_device *arm9facile_snd_device;

static int __init arm9facile_init(void)
{
	int ret = 0;

	arm9facile_snd_device = platform_device_alloc("soc-audio", -1);
	if (!arm9facile_snd_device) {
		return -ENOMEM;
	}

	platform_set_drvdata(arm9facile_snd_device,
			&snd_soc_arm9facile);

	ret = platform_device_add(arm9facile_snd_device);
	if (ret) {
		pr_warning("%s: platform_device_add failed (%d)\n",
			   SND_MODNAME, ret);
		goto err_device_add;
	}

	return 0;

err_device_add:
        if(arm9facile_snd_device != NULL) {
	        platform_device_put(arm9facile_snd_device);
        }
	return ret;
}

static void __exit arm9facile_exit(void)
{
	platform_device_unregister(arm9facile_snd_device);
	arm9facile_snd_device = NULL;
}

module_init(arm9facile_init);
module_exit(arm9facile_exit);

/* Module information */
MODULE_AUTHOR("Antonio Galea <antonio.galea@gmail.com>");
MODULE_DESCRIPTION("ALSA SoC LPC3xxx WM8731");
MODULE_LICENSE("GPL");
