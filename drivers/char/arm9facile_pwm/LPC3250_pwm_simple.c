/*
filename:	LPC3250_pwm_simple.c
version	:	1.0
date	:	2013-02-13
location:	torino
author	:	Antonio Galea
project	:	ZamarGui
object	:	a small driver to handle the 2 simple pwm of the LPC3250
notes	:	a little rough
*/

#undef DEBUG

/* for general driver writing */
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>

/* to provide the io_p2v() function call */
#include <asm/io.h>

/* for the copy_to/from_user */
#include <asm/uaccess.h>

/* for the LPC32XX_PWM1_BASE macro and in general for the address */
/* macro of the LPC3250 peripherals */
#include <mach/platform.h>
#include <mach/hardware.h>

/* number and size of memory banks requested by the driver */
#define LPC3250_pwm_simple_BANKS		2

/* device name */
#define DEVICE_NAME "LPC3250_pwm_simple"

#define WRITE_BUFFER_SIZE	16
#define READ_BUFFER_SIZE	16

#include "LPC3250_pwm_simple.h"

struct LPC3250_pwm_simple_dev
{

    /* internal value of the pwm outputs */
    unsigned char	PWM0_value;
    unsigned char	PWM1_value;
    
    /* internal struct holding the file access functions */
    struct cdev 	cdev;

} *LPC3250_pwm_simple_devp;

static struct file_operations LPC3250_pwm_simple_fops =
{
	.owner			=	THIS_MODULE,
    .open			=	LPC3250_pwm_simple_open,
    .release		=	LPC3250_pwm_simple_release,
    .read			=	LPC3250_pwm_simple_read,
    .write			=	LPC3250_pwm_simple_write,
    .llseek			=	LPC3250_pwm_simple_llseek,
    .compat_ioctl	=	LPC3250_pwm_simple_ioctl,
};

/* allotted device number */
static dev_t LPC3250_pwm_simple_dev_number;
/* tie with the device model */
struct class *LPC3250_pwm_simple_class;

/* utilities ---------------------------- */

/* atoi3(): reads at most 3 chars and returns the */
/* corresponding integer or -1 if no num found  */
int atoi3(char * buf)
{
	int value=-1;
    if( '0' <= *buf && *buf <= '9' )
    {
    	value = *buf-'0';
        buf++;
    	if( '0' <= *buf && *buf <= '9' )
        {
        	value = value * 10 + (*buf-'0');
	        buf++;
    		if( '0' <= *buf && *buf <= '9' )
            {
            	value = value * 10 + (*buf-'0');
	            buf++;
    	    }
        }
    }
    return(value);
}

static int __init LPC3250_pwm_simple_init(void)
{
	printk(DEVICE_NAME": init\n");
    
    /* request dynamic allocation of a device major number */
    if(
    	alloc_chrdev_region(
        					&LPC3250_pwm_simple_dev_number,
                            0,
                            LPC3250_pwm_simple_BANKS,
                            DEVICE_NAME
                           ) < 0
	  )
	{
    	printk("Cannot register device: "DEVICE_NAME"\n");
        return(-1);
    }
    LPC3250_pwm_simple_class = class_create(THIS_MODULE, DEVICE_NAME);

	LPC3250_pwm_simple_devp = kmalloc(sizeof(struct LPC3250_pwm_simple_dev), GFP_KERNEL);
    if( LPC3250_pwm_simple_devp == 0 )
    {
    	printk(DEVICE_NAME": bad kmalloc\n");
        return(1); 
    }  
    
    /* init device values */
    LPC3250_pwm_simple_devp -> PWM0_value = 255;
    
    /* connect the file operations with the cdev */
    cdev_init(&(LPC3250_pwm_simple_devp->cdev), &LPC3250_pwm_simple_fops);
    LPC3250_pwm_simple_devp->cdev.owner = THIS_MODULE;
    
    /* connect the major/minor number to the cdev */
    if( cdev_add( &(LPC3250_pwm_simple_devp->cdev), LPC3250_pwm_simple_dev_number, 1 ) != 0 )
    {
    	printk(DEVICE_NAME": bad cdev\n");
        return(1);
    } 
    
    iowrite32(0x13, LPC32XX_CLKPWR_PWM_CLK_CTRL);
    
    /* send uevent to let udev (if present) to create /dev nodes */
    device_create( LPC3250_pwm_simple_class, NULL, LPC3250_pwm_simple_dev_number, NULL, "LPC3250_pwm");

	/* setting pwm0 to max value */
    iowrite32(0x80000001, io_p2v(LPC32XX_PWM1_BASE));

	return(0);
}

static void __exit LPC3250_pwm_simple_exit(void)
{
	printk(DEVICE_NAME": exit\n");
}

/*
                   #                         #          
                   #                         #          
                   #                                    
 ####  # ###   ### #         ## #   ####    ##   # ###  
#    # ##   # #   ##         # # # #    #    #   ##   # 
#    # #    # #    #         # # #      #    #   #    # 
###### #    # #    #         # # #  #####    #   #    # 
#      #    # #    #         # # # #    #    #   #    # 
#    # #    # #   ##         # # # #    #    #   #    # 
 ####  #    #  ### #         #   #  #####  ##### #    # 

*/

int LPC3250_pwm_simple_open(struct inode *inode, struct file *file)
{
	struct LPC3250_pwm_simple_dev *LPC3250_pwm_simple_devp;

#ifdef DEBUG    
    printk("LPC3250_pwm_simple_open\n");
#endif
    
    /* get the per-device structure that contains the cdev */
    LPC3250_pwm_simple_devp = container_of(inode->i_cdev, struct LPC3250_pwm_simple_dev, cdev);
    
    /* easy access to LPC3250_pwm_simple_devp from rest of the entry points */
    file->private_data = LPC3250_pwm_simple_devp;
    
    /* initialize some fields */
    /* EMPTY for this driver */
    
	return(0);
}

int LPC3250_pwm_simple_release(struct inode *inode, struct file *file)
{
	/*
	struct LPC3250_pwm_simple_dev *LPC3250_pwm_simple_devp = file->private_data;
    */

#ifdef DEBUG    
    printk("LPC3250_pwm_simple_release\n");
#endif
    
    /* should reset file pointer */
	/* EMPTY for this driver */
    
	return(0);
}

ssize_t LPC3250_pwm_simple_read(struct file *file, char __user * buf, size_t count, loff_t *ppos)
{
    char string[READ_BUFFER_SIZE];
    struct LPC3250_pwm_simple_dev *LPC3250_pwm_simple_devp = file->private_data;
    int value;
    
#ifdef DEBUG    
    printk("LPC3250_pwm_simple_read\n");
#endif

	value = LPC3250_pwm_simple_devp -> PWM0_value;
    /* limiting input value to maximum in 8 bit range */
    if( value > 255 ) value = 255;

	/* scaling value to have an ascending progression */
    /* from 0 (disabled), 1 (max) .. 255 (min)        */
    /* to   0 (disabled), 1 (min) .. 255 (max)        */
    
    value = (256 - value) & 0xFF;
    
    sprintf(string, "%3d", value);
    
    if(*ppos>2) return(0);
    
    if( copy_to_user(buf, (void *)string, 3) != 0)
    {
    	return(-EIO);
    }
    
    (*ppos)+=3;
	return(3);
}

ssize_t LPC3250_pwm_simple_write(struct file *file, const char __user * buf, size_t count, loff_t *ppos)
{
    char string[WRITE_BUFFER_SIZE];

#ifdef DEBUG
    char debug_message[256];
#endif

    int  value, numchars;
    unsigned long int pwm_register;
    struct LPC3250_pwm_simple_dev *LPC3250_pwm_simple_devp = file->private_data;

#ifdef DEBUG
    printk("LPC3250_pwm_simple_write\n");
#endif

	numchars = count;
	if( count > 3 ) numchars = 3;

    if( copy_from_user((void *)string, buf, numchars) != 0)
    {
    	return(-EFAULT);
    }
	
    /* zero terminating string, not needed here but useful */
    /* if someone will mess with the code and not know what*/
    /* should be done                                      */
    string[ numchars ] = 0;
    
    value = atoi3(string);

    if( value < 0 )
    {
    	return(-EIO);
    }
    
    /* limiting input value to maximum in 8 bit range */
    if( value > 255 ) value = 255;

	/* scaling value to have an ascending progression */
    /* from 0 (disabled), 1 (max) .. 255 (min)        */
    /* to   0 (disabled), 1 (min) .. 255 (max)        */
    
    value = (256 - value) & 0xFF;

	LPC3250_pwm_simple_devp->PWM0_value = value;

	pwm_register = 0x80000000 | value;

    iowrite32(pwm_register, io_p2v(LPC32XX_PWM1_BASE));

#ifdef DEBUG
	sprintf(debug_message,"LPC32XX_CLKPWR_PWM_CLK_CTRL=%X  LPC32XX_PWM1_BASE=%X\n",ioread32(LPC32XX_CLKPWR_PWM_CLK_CTRL),ioread32(io_p2v(LPC32XX_PWM1_BASE)));
	printk(debug_message);	
#endif

	/* return number of byte written, equal to count */
    /* so that the writes will always be successful  */
	return(count);
}

static loff_t LPC3250_pwm_simple_llseek(struct file *file, loff_t offset, int orig)
{

#ifdef DEBUG
    printk("LPC3250_pwm_simple_llseek\n");
#endif

	return(0);
}

long LPC3250_pwm_simple_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

#ifdef DEBUG
    printk("LPC3250_pwm_simple_ioctl\n");
#endif

	return(0);
}

module_init(LPC3250_pwm_simple_init);
module_exit(LPC3250_pwm_simple_exit);

MODULE_AUTHOR("Antonio Galea");
MODULE_DESCRIPTION("Simple pwm driver for the arm9facile.");
MODULE_LICENSE("GPL");
