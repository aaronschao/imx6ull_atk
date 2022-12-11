/*!
 * @file led.c
 * @brief led驱动程序
 * @par Copyright(c):    Revised BSD License,see section LICENSE
 * @author shichao ()
 * @date 2022-11-05
 * @version 1.0.0
 * @par History:
 * version | author | date | desc
 * --- | --- | --- | ---
 * V1.0.0 | shichao | 2022-11-05 | 创建第一版
 */
#include <linux/module.h>
#include <linux/ide.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_address.h>

/*! 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6ULL_CCM_CCGR1 = NULL;
static void __iomem *SW_MUX_GPIO1_IO03 = NULL;
static void __iomem *SW_PAD_GPIO1_IO03 = NULL;
static void __iomem *GPIO1_DR = NULL;
static void __iomem *GPIO1_GDIR = NULL;

typedef struct {
    dev_t devid;                        /*!< 设备号 */
    struct cdev cdev;                   /*!< cdev */
    struct class *class;                /*!< 类 */
    struct device *device;              /*!< 设备 */
    int major;                          /*!< 主设备号 */
    int minor;                          /*!< 次设备号 */
    struct device_node *nd;             /*!< 设备节点 */
}led_dev_t;

static led_dev_t led_dev = {0};

/*!
 * @brief   打开设备
 * @param inode[in] 传递给驱动的inode
 * @param filp[in] 设备文件，file结构体有个叫做private_data的成员变量. 一般在open的时候将private_data指向设备结构体。
 * @return int  0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &led_dev;    /* 设置私有数据 */
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
static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
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
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    u32 val = 0;
    int iret = 0;
    unsigned char databuf[1];

    iret = copy_from_user(databuf, buf, cnt);
    if(0 > iret)
    {
        printk(KERN_ERR"kernel write failed!\n");
        return -EFAULT;
    }
    printk("databuf[0]:%d\n", databuf[0]);
    if(1 == databuf[0])
    {
        val = readl(GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, GPIO1_DR);
    }
    else if(0 == databuf[0])
    {
        val = readl(GPIO1_DR);
        val |= (1 << 3);
        writel(val, GPIO1_DR);
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
static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/*!
 * @brief 关于led的GPIO初始化
 * @return int
 */
static int __led_init(void)
{
    u32 val = 0;
    int iret = 0;
    const char *str = NULL;
    struct property *proper = NULL;

    led_dev.nd = of_find_node_by_path("/led");
    if(NULL == led_dev.nd)
    {
        printk(KERN_ERR"led node not find\n");
        return -EINVAL;
    }
    else
    {
        printk(KERN_INFO"led node find\n");
    }

    proper = of_find_property(led_dev.nd, "compatible", NULL);
    if(NULL == proper)
    {
        printk(KERN_ERR"compatible not find\n");
        return -EINVAL;
    }
    else
    {
        printk(KERN_INFO"compatible find, value:%s\n", (char *)proper->value);
    }

    iret = of_property_read_string(led_dev.nd, "status", &str);
    if(iret < 0)
    {
        printk(KERN_ERR"get status fail\n");
        return -EINVAL;
    }
    else
    {
        printk(KERN_INFO"status:%s\n", str);
    }
    /* 寄存器地址映射 */
#if 0
    IMX6ULL_CCM_CCGR1 = ioremap(regdata[0], regdata[1]);
    SW_MUX_GPIO1_IO03 = ioremap(regdata[2], regdata[3]);
    SW_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]);
    GPIO1_DR =ioremap(regdata[6], regdata[7]);
    GPIO1_GDIR = ioremap(regdata[8], regdata[9]);
#else
    IMX6ULL_CCM_CCGR1 = of_iomap(led_dev.nd, 0);
    SW_MUX_GPIO1_IO03 = of_iomap(led_dev.nd, 1);
    SW_PAD_GPIO1_IO03 = of_iomap(led_dev.nd, 2);
    GPIO1_DR = of_iomap(led_dev.nd, 3);
    GPIO1_GDIR = of_iomap(led_dev.nd, 4);
#endif

    /* 使能GPIO1时钟 */
    val = readl(IMX6ULL_CCM_CCGR1);
    val &= ~(0x03 << 26);
    val |= (0x03 << 26);
    writel(val, IMX6ULL_CCM_CCGR1);

    /* 设置GPIO1_IO03的复用功能，将其复用为GPIO1_IO03，最后设置IO属性 */
    writel(0x05, SW_MUX_GPIO1_IO03);        /* ALT5 */
    /****************************************************************************************************************************
    *    寄存器SW_PAD_GPIO1_IO06设置IO属性
    *    31-17(Reserved) 16(HYS) 15–14(PUS)  13(PUE) 12(PKE) 11(ODE) 10-8(Reserved) 7-6(SPEED) 5-3(DSE) 2-1(Reserved) 0(SRE)
    *    0               0       00          0       1       0       0              01         101       0               0
    *    保留位          关闭滞留 100k下拉   kepper  使能PUE  关闭开路输出   保留     100M速度    R0/5驱动能力  保留     低转换率
    *****************************************************************************************************************************/
    writel(0x1034, SW_PAD_GPIO1_IO03);

    /* 设置GPIO1_IO03为输出功能 */
    val = readl(GPIO1_GDIR);
    val &= ~(0x01 << 3);
    val |= (0x01 << 3);
    writel(val, GPIO1_GDIR);

    /* 默认关闭LED */
    val = readl(GPIO1_DR);
    val |= (0x01 << 3);
    writel(val, GPIO1_DR);

    return 0;
}

/*! 设备操作函数 */
static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};

/*!
 * @brief   驱动入口函数
 * @return int
 */
static int __init led_init(void)
{
    /* 注册字符设备驱动 */
#if 0
    /* 手动创建设备节点 mknod /dev/led c 200 0 */
    if(0 > register_chrdev(200, "led", &led_fops))
    {
        printk("register chrdev failed!\n");
        return -EIO;
    }
#else
    /* 自动创建设备节点 */
    if (led_dev.major)
    {
        led_dev.devid = MKDEV(led_dev.major, 0);
        register_chrdev_region(led_dev.devid, 1, "led");
    }
    else
    {
        alloc_chrdev_region(&led_dev.devid, 0, 1, "led");    /* 申请设备号 */
        led_dev.major = MAJOR(led_dev.devid);    /* 获取分配号的主设备号 */
        led_dev.minor = MINOR(led_dev.devid);    /* 获取分配号的次设备号 */
    }
    printk("led_dev major=%d,minor=%d\n",led_dev.major, led_dev.minor);

    /* 初始化cdev */
    led_dev.cdev.owner = THIS_MODULE;
    cdev_init(&led_dev.cdev, &led_fops);

    /* 添加一个cdev */
    cdev_add(&led_dev.cdev, led_dev.devid, 1);

    /* 创建类 */
    led_dev.class = class_create(THIS_MODULE, "led");
    if (IS_ERR(led_dev.class))
    {
        return PTR_ERR(led_dev.class);
    }

    /* 创建设备 */
    led_dev.device = device_create(led_dev.class, NULL, led_dev.devid, NULL, "led");
    if (IS_ERR(led_dev.device))
    {
        return PTR_ERR(led_dev.device);
    }
#endif
    return __led_init();
}

/*!
 * @brief   驱动出口函数
 *
 */
static void __exit led_exit(void)
{
    /* 取消映射 */
    iounmap(IMX6ULL_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    /* 注销字符设备驱动 */
#if 0
    unregister_chrdev(200, "led");
#else
    cdev_del(&led_dev.cdev);                    /* 删除cdev */
    unregister_chrdev_region(led_dev.devid, 1); /* 注销设备号 */

    device_destroy(led_dev.class, led_dev.devid);
    class_destroy(led_dev.class);
#endif
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("AaronsChao");
