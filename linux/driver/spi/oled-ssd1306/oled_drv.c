//https://blog.csdn.net/qq_36413982/article/details/123783530
#include <linux/init.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/spi/spi.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>

/* oled设备 */
struct oled_device {
   int dc_gpio;                /* 命令/数据引脚 */
   int rst_gpio;               /* 重置引脚*/
   struct class *oled_class;   /* 设备class */
   struct device *device; 	/* 设备 */
   struct device_node *nd; /* 设备节点 */
   dev_t devid;                /* 设备编号 */
   struct cdev spi_cdev;      /* oled字符设备 */
   struct spi_device *spi;     /* spi 从设备 */

   uint8_t databuf[10];

};

struct oled_device * xboard_oled_dev;

////////////////////////////////////////////////////////////////////////////////////////////////
int oled_spi_write(char *buf, uint16_t len)
{
    int status;
#if 0
    struct spi_message msg;
	struct spi_transfer xfer = {
		.len = len,
		.tx_buf = buf,
	};

	spi_message_init(&msg);                       /* 初始化spi_message */
	spi_message_add_tail(&xfer, &msg);             /* 添加到传输队列 */
    status = spi_sync(xboard_oled_dev->spi, &msg);          /* 同步发送 */
#else
    status = spi_write(xboard_oled_dev->spi, buf, len);   
#endif
    return status;
}

 int oled_write_cmd(uint8_t cmd)
{
    int ret = 0;

   // printk("xboard_oled_dev->dc_gpio:%d\n",xboard_oled_dev->dc_gpio);
   // mdelay(200);
    gpio_set_value(xboard_oled_dev->dc_gpio, 0); /* 拉低，表示写入的是指令 */
    ret = oled_spi_write(&cmd, 1);
    return ret;
}

 int oled_write_data(uint8_t data)
{
    int ret = 0;

    gpio_set_value(xboard_oled_dev->dc_gpio, 1); /* 拉高，表示写入数据 */
    mdelay(5);
    ret = oled_spi_write(&data, 1);
    return ret;
}

 int oled_write_datas(uint8_t datas[],uint16_t len)
{
    int ret = 0;

    gpio_set_value(xboard_oled_dev->dc_gpio, 1); /* 拉高，表示写入数据 */
     mdelay(5);
    ret = oled_spi_write(datas, len);
    return ret;
}



/////////////////////////////////////////////////////////////////////////////////////////
int ssd1306_set_pos(uint16_t x, uint16_t y)
{
    int ret = 0;

    ret = oled_write_cmd(0xb0 + y);
    ret = oled_write_cmd(x & 0x0f);
    ret = oled_write_cmd(((x & 0xf0) >> 4) | 0x10);
    return ret;
}


 void ssd1306_reset(void)
{
    gpio_set_value(xboard_oled_dev->rst_gpio, 0);
    mdelay(100);
    gpio_set_value(xboard_oled_dev->rst_gpio, 1);
    mdelay(50);
}

 void ssd1306_disp_on_off(uint8_t on_off)
{
    if (on_off)
    {
            //oled_write_cmd(0x8d); /* set dispkay on */
            //oled_write_cmd(0x14); /* set dispkay on */
            oled_write_cmd(0xaf); /* set dispkay on */

    }
    else
    {
           // oled_write_cmd(0x8d); /* set dispkay off */
           // oled_write_cmd(0x10); /* set dispkay off */
            oled_write_cmd(0xae); /* set dispkay off */

    }
        
}



 void ssd1306_disp_clear(void)
{
    uint8_t x, y;

    for (y = 0; y < 8; y++)
    {
        ssd1306_set_pos(0, y);
        for (x = 0; x < 128; x++)
            oled_write_data(0); /* 清零 */
    }
}

 void oled_ssd1306_disp_test(void)
{
    uint8_t x, y;

    printk("oled_ssd1306_disp_test\r\n");
    for (y = 0; y < 8; y++)
    {
        ssd1306_set_pos(0, y);
        for (x = 0; x < 128; x++)
        {
            if (x % 2 == 0)
                oled_write_data(0);
            else
                oled_write_data(1);
        }
    }

    msleep(1000);
    ssd1306_disp_clear();
}

 int ssd1306_init(void)
{
    int ret = 0;
     printk("ssd1306_init!\r\n");
    ssd1306_reset();

    ret = oled_write_cmd(0xae); //关闭显示

    ret = oled_write_cmd(0x00); //设置 lower column address
    ret = oled_write_cmd(0x10); //设置 higher column address

    ret = oled_write_cmd(0x40); //设置 display start line

    ret = oled_write_cmd(0xB0); //设置page address

    ret = oled_write_cmd(0x81); // contract control
    ret = oled_write_cmd(0x66); // 128

    ret = oled_write_cmd(0xa1); //设置 segment remap

    ret = oled_write_cmd(0xa6); // normal /reverse

    ret = oled_write_cmd(0xa8); // multiple ratio
    ret = oled_write_cmd(0x3f); // duty = 1/64

    ret = oled_write_cmd(0xc8); // com scan direction

    ret = oled_write_cmd(0xd3); // set displat offset
    ret = oled_write_cmd(0x00); //

    ret = oled_write_cmd(0xd5); // set osc division
    ret = oled_write_cmd(0x80); //

    ret = oled_write_cmd(0xd9); // ser pre-charge period
    ret = oled_write_cmd(0x1f); //

    ret = oled_write_cmd(0xda); // set com pins
    ret = oled_write_cmd(0x12); //

    ret = oled_write_cmd(0xdb); // set vcomh
    ret = oled_write_cmd(0x30); //

    ret = oled_write_cmd(0x8d); // set charge pump disable
    ret = oled_write_cmd(0x14); //

    ret = oled_write_cmd(0xaf); // set dispkay on
ret = oled_write_cmd(0xaf); // set dispkay on
ret = oled_write_cmd(0xaf); // set dispkay on
    ssd1306_disp_clear();
    ssd1306_set_pos(0, 0);

    return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////////


/***************************************************************************************************
 *
 * 打开设备
 *
 **************************************************************************************************/
static int ssd1306_drv_open(struct inode *inode, struct file *filp)
{
    
    //printk(KERN_INFO "oled open\n");
    //filp->private_data = xboard_oled_dev; /* 打开设备 */

    return 0;
}

//static ssize_t _drv_read(struct file *filp, char __user *buf, size_t size, loff_t *offset)
//{
//    int ret = 0;

//    oled_disp_test();

//    printk("%s %s\r\n", __FUNCTION__, DEV_NAME);

//    ret = size;
//    return ret;
//}


static ssize_t ssd1306_drv_write(struct file *filp, const char __user *buf,size_t cnt, loff_t *offt)
{
	int retval;
	unsigned char databuf[2];
	uint8_t  data, type=0;
  	

	retval = copy_from_user(databuf, buf, cnt);
    if(retval < 0)
	{
  	    printk("kernel write failed!\r\n");
  	    return -EFAULT;
	}
    //printk("cnt:%d,ret=%d,type=%d,data=0x%x\n",cnt,retval,databuf[1],databuf[0]);
	data = databuf[0];
	type = databuf[1];

    if(type)
    {
        oled_write_data(data);
    }
    else
    {
        oled_write_cmd(data);
    }
    

    return 0;
}

#define OLED_CMD_SET_XY                   0x01       /* 显示开关*/
#define OLED_CMD_WRITE_DATAS              0x02
#define OLED_CMD_SET_XY_WRITE_DATAS       0x03
#define OLED_CMD_DISP_ON_OFF              0x04

static long ssd1306_drv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    uint8_t buf[3];
    uint16_t size=0;
    const void __user *userspace = (const void __user *)arg;

    switch (cmd & 0x0f)                /* 最低字节存放命令字段 */
    {
    case OLED_CMD_DISP_ON_OFF:
        ret = copy_from_user(&buf[0], userspace, 1);
        ssd1306_disp_on_off(buf[0]);
        break;
    case OLED_CMD_SET_XY:
        ret = copy_from_user(&buf, userspace, 2);
        if (ret > 0) {
            ret = -EFAULT;
            goto exit;
        }
        // printk("x %d, y %d\r", buf[0], buf[1]);
        ssd1306_set_pos(buf[0], buf[1]);
        break;
    case OLED_CMD_WRITE_DATAS:
        size = (uint16_t)(cmd & 0xffffff00);        /* 前三字节存放数据大小 */
        size >>= 8;
        // printk("size %d\r", size);
        ret = copy_from_user(xboard_oled_dev->databuf, userspace, size);
        if (ret > 0) {
            ret = -EFAULT;
            goto exit;
        }
        oled_write_datas(xboard_oled_dev->databuf,size);
    case OLED_CMD_SET_XY_WRITE_DATAS:
        printk("OLED_CMD_SET_XY_WRITE_DATAS\n");
        ret = copy_from_user(buf, userspace, size);
        if (ret > 0) {
            ret = -EFAULT;
            goto exit;
        }
        //oled_write_datas(xboard_oled_dev->databuf);
        break;
    }

exit:
    return ret;
}

static int ssd1306_drv_release(struct inode *node, struct file *filp)
{
    return 0;
}

static struct file_operations oled_drv_ops = {
    .owner = THIS_MODULE,
    .open = ssd1306_drv_open,
//    .read = _drv_read,
    .write = ssd1306_drv_write,
    .unlocked_ioctl = ssd1306_drv_ioctl,
    .release = ssd1306_drv_release,
};


static int ssd1306_driver_probe(struct spi_device *spi)
{
    int err = 0;
    struct device *_dev;
    struct device_node *_dts_node;
    // struct device_node *xboard_oled_dev_node;
    xboard_oled_dev = (struct oled_device *)kzalloc(sizeof(struct oled_device), GFP_KERNEL);
    if (!xboard_oled_dev)
    {
        printk("can't kzalloc moledpu6050 dev\n");
        return -ENOMEM;
    }

    _dts_node = spi->dev.of_node;
    if (!_dts_node)
    {
        printk("oled espi can not found!\r\n");
        err = -EINVAL;
        goto exit_free_dev;
    }

    xboard_oled_dev->dc_gpio = of_get_named_gpio(_dts_node, "dc-gpio", 0); /* 获取dc_gpio */
    if (!gpio_is_valid(xboard_oled_dev->dc_gpio))
    {
        printk("don't get oled %s!!!\n", "dc-gpio");
        err = -EINVAL;
        goto exit_free_dev;
    }
    printk("oled dc-gpio %d\n", xboard_oled_dev->dc_gpio);
    err=gpio_request( xboard_oled_dev->dc_gpio, "dc_gpio" );
    if( err < 0 )
    {
      printk("ERROR: Reset GPIO %d request,error %d\n", xboard_oled_dev->dc_gpio,err);

    }
    gpio_direction_output(xboard_oled_dev->dc_gpio, 1); /* 设置gpio为输入 */
    
    xboard_oled_dev->rst_gpio = of_get_named_gpio(_dts_node, "rst-gpio", 0); /* 获取rst_gpio */
    if (!gpio_is_valid(xboard_oled_dev->rst_gpio))
    {
        printk("don't get oled %s!!!\n", "rst-gpio");
        err = -EINVAL;
        goto exit_free_dev;
    }
    printk("oled rst-gpio %d\n", xboard_oled_dev->rst_gpio);
    err=gpio_request( xboard_oled_dev->rst_gpio, "rst_gpio" );
    if(  err< 0 )
    {
      printk("ERROR: Reset GPIO %d request,error %d\n", xboard_oled_dev->rst_gpio,err);

    }
    gpio_direction_output(xboard_oled_dev->rst_gpio, 1); /* 设置gpio为输入 */

    /* 内核自动分配设备号 */
    err = alloc_chrdev_region(&xboard_oled_dev->devid, 0, 1, "oledspi");
    if (err < 0)
    {
        pr_err("Error: failed to register oled, err: %d\n", err);
        goto exit_free_dev;
    }

    cdev_init(&xboard_oled_dev->spi_cdev, &oled_drv_ops);
    err = cdev_add(&xboard_oled_dev->spi_cdev, xboard_oled_dev->devid, 1);
    if (err)
    {
        printk("cdev add failed\r\n");
        goto exit_unregister;
    }
    

    xboard_oled_dev->oled_class = class_create(THIS_MODULE, "oledspi");
    if (IS_ERR(xboard_oled_dev->oled_class))
    {
        err = PTR_ERR(xboard_oled_dev->oled_class);
        goto exit_cdev_del;
    }

    /* 创建设备节点 */
    _dev = device_create(xboard_oled_dev->oled_class, NULL, xboard_oled_dev->devid, NULL, "oledspi");
    if (IS_ERR(_dev))
    { /* 判断指针是否合法 */
        err = PTR_ERR(_dev);
        goto exit_class_del;
    }

    xboard_oled_dev->spi = spi;

    //mutex_init(&xboard_oled_dev->m_lock); /* 初始化互斥锁  */
     ssd1306_init();
     //oled_ssd1306_disp_test();
    printk("%s probe success\r\n", "oledspi");

    goto exit;

exit_class_del:
    class_destroy(xboard_oled_dev->oled_class);
exit_cdev_del:
    cdev_del(&xboard_oled_dev->spi_cdev);
exit_unregister:
    unregister_chrdev_region(xboard_oled_dev->devid, 1); /* 注销设备 */
exit_free_dev:
    kfree(xboard_oled_dev);
    xboard_oled_dev = NULL;
exit:
    return err;
}

static int ssd1306_driver_remove(struct spi_device *spi)
{
    int ret = 0;

    device_destroy(xboard_oled_dev->oled_class, xboard_oled_dev->devid);
    class_destroy(xboard_oled_dev->oled_class);
    cdev_del(&xboard_oled_dev->spi_cdev);
    unregister_chrdev_region(xboard_oled_dev->devid, 1); /* 注销设备 */
    kfree(xboard_oled_dev);

    printk(KERN_INFO "%s remove success\n", "oledspi");

    return ret;
}

/* 设备树的匹配列表 */
static struct of_device_id dts_match_table[] = {
    {.compatible = "solomon,ssd1306-spi"}, /* 通过设备树来匹配 */
    {},
};

/* 传统匹配方式 ID 列表 */
static const struct spi_device_id spi_dev_id[] = {
    {.name = "ssd1306oled", 0},
    {}};

/* SPI 驱动结构体 */
static struct spi_driver oled_driver = {
    .probe = ssd1306_driver_probe,
    .remove = ssd1306_driver_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ssd1306oled",
        .of_match_table = dts_match_table,
    },
    .id_table = spi_dev_id,
};

module_spi_driver(oled_driver);

MODULE_AUTHOR("Ares");
MODULE_LICENSE("GPL");







