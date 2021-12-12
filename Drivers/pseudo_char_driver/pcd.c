#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

#define DEV_MEM_SIZE 512

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__

// pseudo device memory
char device_buffer[DEV_MEM_SIZE];

dev_t device_number;

struct cdev pcd_cdev;

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
	pr_info("lseek requested\n");
	pr_info("current file position = %lld\n",filp->f_pos);
	if(offset > DEV_MEM_SIZE || offset < 0)
		return -EINVAL;
	
	switch(whence)
	{
		case SEEK_SET:
		filp->f_pos = offset;
		break;
		
		case SEEK_CUR:
		if(filp->f_pos+offset > DEV_MEM_SIZE || filp->f_pos+offset < 0)
			return -EINVAL; 
		filp->f_pos += offset;
		break;

		case SEEK_END:
		if(DEV_MEM_SIZE+offset > DEV_MEM_SIZE || DEV_MEM_SIZE+offset < 0)
			return -EINVAL; 
		filp->f_pos = DEV_MEM_SIZE + offset;
		break;
		
		default:
		return -EINVAL;
	}

	pr_info("new value of file position = %lld\n",filp->f_pos);
	return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("read requested for %zu bytes\n", count);
	pr_info("Current file position = %lld\n", *f_pos);
	
	if((*f_pos + count) > DEV_MEM_SIZE)
		count = DEV_MEM_SIZE - *f_pos;
	
	if(copy_to_user(buff, &device_buffer[*f_pos], count))
		return -EFAULT; 

	*f_pos += count;
	pr_info("Number of bytes successfully read = %zu\n", count);
	pr_info("Updated file position = %lld\n", *f_pos);

	//Return the number of bytes successfully read
	return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("write requested for %zu bytes\n", count);
	pr_info("Current file position = %lld\n", *f_pos);
	
	if((*f_pos + count) > DEV_MEM_SIZE)
		count = DEV_MEM_SIZE - *f_pos;
	
	if(!count)
		return -ENOMEM;
	
	if(copy_from_user(&device_buffer[*f_pos], buff, count))
		return -EFAULT; 

	*f_pos += count;
	pr_info("Number of bytes successfully written = %zu\n", count);
	pr_info("Updated file position = %lld\n", *f_pos);

	//Return the number of bytes successfully written
	return count;
}

int pcd_open(struct inode *inode, struct file *filp)
{
	pr_info("open successful\n");
	return 0;
}

int pcd_release(struct inode *inode, struct file *filp)
{
	pr_info("release successful\n");
	return 0;
}

struct file_operations pcd_fops=
{
	.open = pcd_open,
	.write = pcd_write,
	.read = pcd_read,
	.llseek = pcd_lseek,
	.release = pcd_release,
	.owner = THIS_MODULE
};

struct class *class_pcd;
struct device *device_pcd;

static int __init pcd_driver_init(void)
{
	int ret;
    ret = alloc_chrdev_region(&device_number, 0, 1, "pcd_devices");
	if(ret < 0)
		goto out;
	
	pr_info("Device number <major>:<minor> = %d:%d\n", MAJOR(device_number), MINOR(device_number));
	cdev_init(&pcd_cdev, &pcd_fops);
	 
	//Register device with VFS
	pcd_cdev.owner = THIS_MODULE;
	ret = cdev_add(&pcd_cdev, device_number, 1);
	if(ret < 0)
		goto unreg_chrdev;
	
	//create device class directory under /sys/class
	class_pcd = class_create(THIS_MODULE, "pcd_class");
	if(IS_ERR(class_pcd))
	{
		pr_err("Class creation failed!");
		ret = PTR_ERR(class_pcd);
		goto cdev_del;
	}

	//populate sysfs with dev information
	device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");
	if(IS_ERR(device_pcd))
	{
		pr_err("Device creation failed!");
		ret = PTR_ERR(device_pcd);
		goto class_del;
	}
	
	pr_info("Module init was successful\n");
    return 0;

class_del:
	class_destroy(class_pcd);
cdev_del:
	cdev_del(&pcd_cdev);
unreg_chrdev:
	unregister_chrdev_region(device_number, 1);
out:
	pr_info("Module insertion failed!\n");
	return ret;
}

static void __exit pcd_driver_cleanup(void)
{
	device_destroy(class_pcd, device_number);
	class_destroy(class_pcd);
	cdev_del(&pcd_cdev);
	unregister_chrdev_region(device_number, 1);
	pr_info("module unloaded\n");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amol");
MODULE_DESCRIPTION("A pseudo char driver kernel module");
MODULE_INFO(board, "BBB Rev A5");
