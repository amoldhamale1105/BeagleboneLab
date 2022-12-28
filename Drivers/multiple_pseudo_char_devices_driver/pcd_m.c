#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

#define NO_OF_DEVICES 4

#define PCD1_MEM_SIZE 1024
#define PCD2_MEM_SIZE 512
#define PCD3_MEM_SIZE 1024
#define PCD4_MEM_SIZE 512

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__

// pseudo device memory
char pcd1_device_buffer[PCD1_MEM_SIZE];
char pcd2_device_buffer[PCD2_MEM_SIZE];
char pcd3_device_buffer[PCD3_MEM_SIZE];
char pcd4_device_buffer[PCD4_MEM_SIZE];

//Device private data
struct pcdev_private_data
{
	char* buffer;
	unsigned size;
	const char* serial_number;
	int perm;
	struct cdev cdev;
    //struct spinlock_t pcdev_lock;
    struct mutex pcdev_lock;
};

//Driver private data structure
struct pcdrv_private_data
{
	int total_devices;
	dev_t device_number;
	struct class *class_pcd;
	struct device *device_pcd;
	struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR 0x11

struct pcdrv_private_data pcdrv_data =
{
	.total_devices = NO_OF_DEVICES,
	.pcdev_data = {
		{
			.buffer = pcd1_device_buffer,
			.size = PCD1_MEM_SIZE,
			.serial_number = "PCDEV1XYZ123",
			.perm = RDONLY
		},
		{
			.buffer = pcd2_device_buffer,
			.size = PCD2_MEM_SIZE,
			.serial_number = "PCDEV2XYZ123",
			.perm = WRONLY
		},
		{
			.buffer = pcd3_device_buffer,
			.size = PCD3_MEM_SIZE,
			.serial_number = "PCDEV3XYZ123",
			.perm = RDWR
		},
		{
			.buffer = pcd4_device_buffer,
			.size = PCD4_MEM_SIZE,
			.serial_number = "PCDEV4XYZ123",
			.perm = RDWR
		}
	}
};

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size = pcdev_data->size;
	pr_info("lseek requested\n");
	pr_info("current file position = %lld\n",filp->f_pos);
	if(offset > max_size || offset < 0)
		return -EINVAL;
	
	switch(whence)
	{
		case SEEK_SET:
		filp->f_pos = offset;
		break;
		
		case SEEK_CUR:
		if(filp->f_pos+offset > max_size || filp->f_pos+offset < 0)
			return -EINVAL; 
		filp->f_pos += offset;
		break;

		case SEEK_END:
		if(max_size+offset > max_size || max_size+offset < 0)
			return -EINVAL; 
		filp->f_pos = max_size + offset;
		break;
		
		default:
		return -EINVAL;
	}

	pr_info("new value of file position = %lld\n",filp->f_pos);
	return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size = pcdev_data->size;
	
    pr_info("read requested for %zu bytes\n", count);
	pr_info("Current file position = %lld\n", *f_pos);
	
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;
	
	if(copy_to_user(buff, &pcdev_data->buffer[*f_pos], count))
		return -EFAULT; 

	*f_pos += count;
	pr_info("Number of bytes successfully read = %zu\n", count);
	pr_info("Updated file position = %lld\n", *f_pos);

	//Return the number of bytes successfully read
	return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size = pcdev_data->size;
	ssize_t ret;

    mutex_lock(&pcdev_data->pcdev_lock);
	pr_info("write requested for %zu bytes\n", count);
	pr_info("Current file position = %lld\n", *f_pos);
	
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;
	
	if(!count){
		ret = -ENOMEM;
		goto out;
	}
	
	if(copy_from_user(&pcdev_data->buffer[*f_pos], buff, count)){
		ret = -EFAULT;
		goto out;
	}

	*f_pos += count;
	ret = count;
	pr_info("Number of bytes successfully written = %zu\n", count);
	pr_info("Updated file position = %lld\n", *f_pos);

out:
    mutex_unlock(&pcdev_data->pcdev_lock);
	return ret;
}

static int check_permission(int dev_perm, int access_mode)
{
	if(dev_perm == RDWR)
		return 0;

	else if(dev_perm == RDONLY && (access_mode & FMODE_READ) && !(access_mode & FMODE_WRITE))
		return 0;
	
	else if(dev_perm == WRONLY && !(access_mode & FMODE_READ) && (access_mode & FMODE_WRITE))
		return 0;

	return -EPERM;
}

int pcd_open(struct inode *inode, struct file *filp)
{
	int ret, minor_n;
	struct pcdev_private_data *pcdev_data;

	minor_n = MINOR(inode->i_rdev);
	pr_info("minor access = %d\n",minor_n);
	
	//Extract dev private data ptr from its member cdev
	pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);
	
	//Save ptr of dev private data for other file operation methods
	filp->private_data = pcdev_data;

	ret = check_permission(pcdev_data->perm, filp->f_mode);

	!ret ? pr_info("Open successful\n") : pr_info("Open unsuccessful\n");

	return ret;
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


static int __init pcd_driver_init(void)
{
	int ret, i;
    ret = alloc_chrdev_region(&pcdrv_data.device_number, 0, NO_OF_DEVICES, "pcd_devices");
	if(ret < 0)
		goto out;
	
	//create device class directory under /sys/class
	pcdrv_data.class_pcd = class_create(THIS_MODULE, "pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_err("Class creation failed!");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		goto unreg_chrdev;
	}
	
	for(i=0; i<NO_OF_DEVICES;i++)
	{
		pr_info("Device number <major>:<minor> = %d:%d\n", MAJOR(pcdrv_data.device_number+i), MINOR(pcdrv_data.device_number+i));
        
        //Initialize spinlock or mutex
        //spin_lock_init(&pcdrv_data.pcdev_data[i].pcdev_lock);
        mutex_init(&pcdrv_data.pcdev_data[i].pcdev_lock);

		cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);
	 
		//Register device with VFS
		pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
		ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, pcdrv_data.device_number+i, 1);
		if(ret < 0)
			goto class_del;

		//populate sysfs with dev information
		pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, pcdrv_data.device_number+i, NULL, "pcdev-%d",i);
		if(IS_ERR(pcdrv_data.device_pcd))
		{
			pr_err("Device creation failed!");
			ret = PTR_ERR(pcdrv_data.device_pcd);
			goto cdev_del;
		}
	}
	
	pr_info("Module init was successful\n");
    return 0;

cdev_del:
class_del:
	for(; i>=0; i--)
	{
		device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);
unreg_chrdev:
	unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
out:
	pr_info("Module insertion failed!\n");
	return ret;
}

static void __exit pcd_driver_cleanup(void)
{
	int i;
	for(i=0; i<NO_OF_DEVICES; i++)
	{
		device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);
	unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
	pr_info("module unloaded\n");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amol Dhamale");
MODULE_DESCRIPTION("A multiple device pseudo char driver kernel module");
MODULE_INFO(board, "BBB Rev A5");
