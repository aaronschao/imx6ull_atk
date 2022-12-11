/*!
 * @file timer.c
 * @brief 使用内核定时器来定时驱动beep
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
#include <linux/timer.h>
#include <linux/of_gpio.h>

#define OPEN_CMD            (_IO(0XEF, 0x1))              /* 打开定时器 */
#define CLOSE_CMD           (_IO(0XEF, 0x2))              /* 关闭定时器 */
#define SETPERIOD_CMD       (_IO(0XEF, 0x3))              /* 设置定时器周期命令 */

typedef struct {
    dev_t devid;                    /* 设备号 */
    struct cdev cdev;               /* cdev */
    struct class *class;            /* 类 */
    struct device *device;          /* 设备 */
    int major;                      /* 主设备号 */
    int minor;                      /* 次设备号 */
    struct device_node *nd;         /* 设备节点 */
    int beep_gpio;                  /* beep所使用的GPIO编号 */
    int timerperiod;                 /* 定时周期 */
    struct timer_list timer;        /* 定时器 */
    spinlock_t lock;                /* 自旋锁 */
}timer_dev_t;

static timer_dev_t timer_dev = {0};

/*!
 * @brief 初始化关于beep的GPIO
 * @return int
 */
static int __beep_init(void)
{
    int iret = 0;

    timer_dev.nd = of_find_node_by_path("/beep");
    if(NULL == timer_dev.nd)
    {
        printk(KERN_ERR"beep node not find\n");
        return -EINVAL;
    }
    else
    {
        printk(KERN_INFO"beep node find\n");
    }

    timer_dev.beep_gpio = of_get_named_gpio(timer_dev.nd, "gpio", 0);
    if(timer_dev.beep_gpio < 0)
    {
        printk(KERN_ERR"get beep gpio fail\n");
        return -EINVAL;
    }
    printk(KERN_INFO"get beep gpio success, gpio:%d\n", timer_dev.beep_gpio);

    iret = gpio_direction_output(timer_dev.beep_gpio, 1);
    if(iret < 0)
    {
        printk(KERN_ERR"set beep gpio output fail\n");
        return -EINVAL;
    }
    return 0;
}


/*!
 * @brief   打开设备
 * @param inode[in] 传递给驱动的inode
 * @param filp[in] 设备文件，file结构体有个叫做private_data的成员变量. 一般在open的时候将private_data指向设备结构体。
 * @return int  0 成功;其他 失败
 */
static int timer_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &timer_dev;    /* 设置私有数据 */

    timer_dev.timerperiod = 50;

    return __beep_init();
}

/*!
 * @brief   关闭/释放设备
 * @param inode[in]
 * @param filp[in]  要关闭的设备文件(文件描述符)
 * @return int  0 成功;其他 失败
 */
static int timer_release(struct inode *inode, struct file *filp)
{
    timer_dev_t *dev = filp->private_data;

    del_timer_sync(&timer_dev.timer);
    gpio_set_value(dev->beep_gpio, 1);
    return 0;
}

/*
 * @description : ioctl 函数，
 * @param – filp : 要打开的设备文件(文件描述符)
 * @param - cmd : 应用程序发送过来的命令
 * @param - arg : 参数
 * @return : 0 成功;其他 失败
 */
static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    timer_dev_t *dev = (timer_dev_t *)filp->private_data;
    int timerperiod = 0;
    unsigned long flag = 0;

    switch(cmd)
    {
        case OPEN_CMD:
            spin_lock_irqsave(&dev->lock, flag);
            timerperiod = dev->timerperiod;
            spin_unlock_irqrestore(&dev->lock, flag);
            mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timerperiod));
        break;
        case CLOSE_CMD:
            del_timer_sync(&dev->timer);
        break;
        case SETPERIOD_CMD:
            spin_lock_irqsave(&dev->lock, flag);
            dev->timerperiod = arg;
            spin_unlock_irqrestore(&dev->lock, flag);
            mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timerperiod));
        break;
        default:
            printk(KERN_ERR"cmd is error\n");
            break;
    }

    return 0;
}

static void timer_function(struct timer_list *arg)
{
    timer_dev_t *dev = container_of(arg, timer_dev_t, timer);
    static int status = 1;

    status = !status;

    gpio_set_value(timer_dev.beep_gpio, status);
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timerperiod));

    return ;
}

/* 设备操作函数 */
static struct file_operations timer_fops = {
    .owner = THIS_MODULE,
    .open = timer_open,
    .release = timer_release,
    .unlocked_ioctl = timer_unlocked_ioctl,
};

static int __init timer_init(void)
{
    spin_lock_init(&timer_dev.lock);

    if(timer_dev.major)
    {
        timer_dev.devid = MKDEV(timer_dev.major, 0);
        register_chrdev_region(timer_dev.devid, 1, "timer");
    }
    else
    {
        alloc_chrdev_region(&timer_dev.devid, 0, 1, "timer");
        timer_dev.major = MAJOR(timer_dev.devid);
        timer_dev.minor = MINOR(timer_dev.devid);
    }

    timer_dev.cdev.owner = THIS_MODULE;
    cdev_init(&timer_dev.cdev, &timer_fops);

    cdev_add(&timer_dev.cdev, timer_dev.devid, 1);

    timer_dev.class = class_create(THIS_MODULE, "timer");
    if(IS_ERR(timer_dev.class))
    {
        return PTR_ERR(timer_dev.class);
    }

    timer_dev.device = device_create(timer_dev.class, NULL, timer_dev.devid, NULL, "timer");
    if(IS_ERR(timer_dev.device))
    {
        return PTR_ERR(timer_dev.device);
    }

    timer_setup(&timer_dev.timer, timer_function, 0);

    return 0;
}

static void __exit timer_exit(void)
{
    del_timer_sync(&timer_dev.timer);
    gpio_set_value(timer_dev.beep_gpio, 1);

    cdev_del(&timer_dev.cdev);
    unregister_chrdev_region(timer_dev.devid, 1);
    device_destroy(timer_dev.class, timer_dev.devid);
    class_destroy(timer_dev.class);

    return ;
}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("AaronsChao");