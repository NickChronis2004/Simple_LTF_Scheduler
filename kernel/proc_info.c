/*
 * HY345 - Assignment 3 & 4
 * System calls for LTF scheduler
 * Nikos Chronis - AM: 5174
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

asmlinkage long sys_set_proc_info(int deadline, int est_runtime)
{
    unsigned long dl_jiffies, rt_jiffies;

    printk(KERN_INFO "HY345 [csd5174]: set_proc_info - deadline=%ds, est_runtime=%dms\n",
           deadline, est_runtime);

    if (deadline <= 0 || est_runtime <= 0)
        return -EINVAL;

    /* Convert to jiffies */
    dl_jiffies = jiffies + (unsigned long)deadline * HZ;
    rt_jiffies = msecs_to_jiffies(est_runtime);

    /* Feasibility check: runtime must fit before deadline */
    if (time_after(jiffies + rt_jiffies, dl_jiffies))
        return -EINVAL;

    current->deadline = dl_jiffies;
    current->est_runtime = rt_jiffies;

    return 0;
}

asmlinkage long sys_get_proc_info(int __user *deadline, int __user *est_runtime)
{
    int dl_secs, rt_msecs;

    printk(KERN_INFO "HY345 [csd5174]: get_proc_info called\n");

    if (!deadline || !est_runtime)
        return -EINVAL;

    if (time_before(jiffies, current->deadline))
        dl_secs = (current->deadline - jiffies) / HZ;
    else
        dl_secs = 0;

    rt_msecs = jiffies_to_msecs(current->est_runtime);

    if (put_user(dl_secs, deadline))
        return -EFAULT;
    if (put_user(rt_msecs, est_runtime))
        return -EFAULT;

    return 0;
}
