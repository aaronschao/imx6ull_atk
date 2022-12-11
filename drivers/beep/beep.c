/*!
 * @file beep.c
 * @brief beep驱动程序
 * @par Copyright(c):    Revised BSD License,see section LICENSE
 * @author shichao ()
 * @date 2022-12-11
 * @version 1.0.0
 * @par History:
 * version | author | date | desc
 * --- | --- | --- | ---
 * V1.0.0 | shichao | 2022-12-11 | 创建第一版
 */
#include <linux/module.h>
#include <linux/ide.h>
#include <linux/cdev.h>
#include <linux/of_gpio.h>

/*! 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6ULL_CCM_CCGR1 = NULL;
static void __iomem *SW_MUX_GPIO1_IO03 = NULL;
static void __iomem *SW_PAD_GPIO1_IO03 = NULL;
static void __iomem *GPIO1_DR = NULL;
static void __iomem *GPIO1_GDIR = NULL;

typedef struct {
    dev_t devid;                    /*!< 设备号 */
    struct cdev cdev;               /*!< cdev */
    struct class *class;            /*!< 类 */
    struct device *device;          /*!< 设备 */
    int major;                      /*!< 主设备号 */
    int minor;                      /*!< 次设备号 */
    struct device_node *nd;         /*!< 设备节点 */
    int gpio;                       /*!< beep 所使用的GPIO编号 */
}beep_dev_t;

static beep_dev_t beep_dev = {0};

/*!
 * @brief   打开设备
 * @param inode[in] 传递给驱动的inode
 * @param filp[in] 设备文件，file结构体有个叫做private_data的成员变量. 一般在open的时候将private_data指向设备结构体。
 * @return int  0 成功;其他 失败
 */
static int beep_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &beep_dev;    /* 设置私有数据 */
    return 0;
}

/*!
 * @brief 从设备读取数据
 * @param filp[in]  要打开的设备文件(文件描述符)
 * @param buf[out]  返回给用户空间的数据缓冲区
 * @param cnt[in]   要读取的数据长度
 * @param offt[in]  相对于文件首地址的偏移
 * @return ssize_t  读取的字节数，如果为负值，表示读取失败
 */
static ssize_t beep_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

/*!
 * @brief 向设备写数据
 * @param filp[in]  设备文件，表示打开的文件描述符
 * @param buf[in]   要写给设备写入的数据
 * @param cnt[in]   要写入的数据长度
 * @param offt[in]  相对于文件首地址的偏移
 * @return ssize_t  写入的字节数，如果为负值，表示写入失败
 */
static ssize_t beep_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int iret = 0;
    unsigned char databuf[1];
    beep_dev_t *beep_dev = filp->private_data;

    iret = copy_from_user(databuf, buf, cnt);
    if(0 > iret)
    {
        printk(KERN_ERR"kernel write faibeep!\n");
        return -EFAULT;
    }

    if(1 == databuf[0])
    {
        gpio_set_value(beep_dev->gpio, 0);
    }
    else if(0 == databuf[0])
    {
        gpio_set_value(beep_dev->gpio, 1);
    }
    else
    {
        printk(KERN_ERR"input error, %c", databuf[0]);
    }

    return 0;
}

/*!
 * @brief   关闭/释放设备
 * @param inode[in]
 * @param filp[in]  要关闭的设备文件(文件描述符)
 * @return int  0 成功;其他 失败
 */
static int beep_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/*!
 * @brief 初始化关于beep的GPIO
 * @return int
 */
static int __beep_init(void)
{
    int iret = 0;

    beep_dev.nd = of_find_node_by_path("/beep");
    if(NULL == beep_dev.nd)
    {
        printk(KERN_ERR"beep node not find\n");
        return -EINVAL;
    }
    else
    {
        printk(KERN_INFO"beep node find\n");
    }

    beep_dev.gpio = of_get_named_gpio(beep_dev.nd, "gpio", 0);
    if(beep_dev.gpio < 0)
    {
        printk(KERN_ERR"get beep gpio fail\n");
        return -EINVAL;
    }
    printk(KERN_INFO"get beep gpio success, gpio:%d\n", beep_dev.gpio);

    iret = gpio_direction_output(beep_dev.gpio, 1);
    if(iret < 0)
    {
        printk(KERN_ERR"set beep gpio output fail\n");
        return -EINVAL;
    }
    return 0;
}

/*! 设备操作函数 */
static struct file_operations beep_fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .read = beep_read,
    .write = beep_write,
    .release = beep_release,
};

/*!
 * @brief   驱动入口函数
 * @return int
 */
static int __init beep_init(void)
{
    /* 注册字符设备驱动 */
#if 0
    /* 手动创建设备节点 mknod /dev/beep c 200 0 */
    if(0 > register_chrdev(200, "beep", &beep_fops))
    {
        printk("register chrdev faibeep!\n");
        return -EIO;
    }
#else
    /* 自动创建设备节点 */
    if (beep_dev.major)
    {
        beep_dev.devid = MKDEV(beep_dev.major, 0);
        register_chrdev_region(beep_dev.devid, 1, "beep");
    }
    else
    {
        alloc_chrdev_region(&beep_dev.devid, 0, 1, "beep");    /* 申请设备号 */
        beep_dev.major = MAJOR(beep_dev.devid);    /* 获取分配号的主设备号 */
        beep_dev.minor = MINOR(beep_dev.devid);    /* 获取分配号的次设备号 */
    }
    printk("beep_dev major=%d,minor=%d\n",beep_dev.major, beep_dev.minor);

    /* 初始化cdev */
    beep_dev.cdev.owner = THIS_MODULE;
    cdev_init(&beep_dev.cdev, &beep_fops);

    /* 添加一个cdev */
    cdev_add(&beep_dev.cdev, beep_dev.devid, 1);

    /* 创建类 */
    beep_dev.class = class_create(THIS_MODULE, "beep");
    if (IS_ERR(beep_dev.class))
    {
        return PTR_ERR(beep_dev.class);
    }

    /* 创建设备 */
    beep_dev.device = device_create(beep_dev.class, NULL, beep_dev.devid, NULL, "beep");
    if (IS_ERR(beep_dev.device))
    {
        return PTR_ERR(beep_dev.device);
    }
#endif
    return __beep_init();
}

/*!
 * @brief   驱动出口函数
 *
 */
static void __exit beep_exit(void)
{
    /* 取消映射 */
    iounmap(IMX6ULL_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    /* 注销字符设备驱动 */
#if 0
    unregister_chrdev(200, "beep");
#else
    cdev_del(&beep_dev.cdev);                    /* 删除cdev */
    unregister_chrdev_region(beep_dev.devid, 1); /* 注销设备号 */

    device_destroy(beep_dev.class, beep_dev.devid);
    class_destroy(beep_dev.class);
#endif
}

module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("AaronsChao");
