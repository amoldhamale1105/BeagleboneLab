#include "pcd_platform_driver_dt_sysfs.h"
#include "pcd_syscalls.h"

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
    struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
    int max_size = pcdev_data->pdata.size;
    pr_info("lseek requested\n");
    pr_info("current file position = %lld\n", filp->f_pos);
    if (offset > max_size || offset < 0)
        return -EINVAL;

    switch (whence)
    {
    case SEEK_SET:
        filp->f_pos = offset;
        break;

    case SEEK_CUR:
        if (filp->f_pos + offset > max_size || filp->f_pos + offset < 0)
            return -EINVAL;
        filp->f_pos += offset;
        break;

    case SEEK_END:
        if (max_size + offset > max_size || max_size + offset < 0)
            return -EINVAL;
        filp->f_pos = max_size + offset;
        break;

    default:
        return -EINVAL;
    }

    pr_info("new value of file position = %lld\n", filp->f_pos);
    return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
    int max_size = pcdev_data->pdata.size;

    pr_info("read requested for %zu bytes\n", count);
    pr_info("Current file position = %lld\n", *f_pos);

    if ((*f_pos + count) > max_size)
        count = max_size - *f_pos;

    if (copy_to_user(buff, &pcdev_data->buffer[*f_pos], count))
        return -EFAULT;

    *f_pos += count;
    pr_info("Number of bytes successfully read = %zu\n", count);
    pr_info("Updated file position = %lld\n", *f_pos);

    // Return the number of bytes successfully read
    return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
    int max_size = pcdev_data->pdata.size;
    ssize_t ret;

    mutex_lock(&pcdev_data->pcd_lock);
    pr_info("write requested for %zu bytes\n", count);
    pr_info("Current file position = %lld\n", *f_pos);

    if ((*f_pos + count) > max_size)
        count = max_size - *f_pos;

    if (!count){
        ret = -ENOMEM;
        goto out;
    }

    if (copy_from_user(&pcdev_data->buffer[*f_pos], buff, count)){
        ret = -EFAULT;
        goto out;
    }

    *f_pos += count;
    ret = count;
    pr_info("Number of bytes successfully written = %zu\n", count);
    pr_info("Updated file position = %lld\n", *f_pos);

out:
    mutex_unlock(&pcdev_data->pcd_lock);
    return ret;
}

static int check_permission(int dev_perm, int access_mode)
{
    if (dev_perm == RDWR)
        return 0;

    else if (dev_perm == RDONLY && (access_mode & FMODE_READ) && !(access_mode & FMODE_WRITE))
        return 0;

    else if (dev_perm == WRONLY && !(access_mode & FMODE_READ) && (access_mode & FMODE_WRITE))
        return 0;

    return -EPERM;
}

int pcd_open(struct inode *inode, struct file *filp)
{
    int ret, minor_n;
    struct pcdev_private_data *pcdev_data;

    minor_n = MINOR(inode->i_rdev);
    pr_info("minor access = %d\n", minor_n);

    // Extract dev private data ptr from its member cdev
    pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);

    // Save ptr of dev private data for other file operation methods
    filp->private_data = pcdev_data;

    ret = check_permission(pcdev_data->pdata.perm, filp->f_mode);

    !ret ? pr_info("Open successful\n") : pr_info("Open unsuccessful\n");

    return ret;
}

int pcd_release(struct inode *inode, struct file *filp)
{
	pr_info("release successful\n");
	return 0;
}
