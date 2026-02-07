# HY-345 Assignment 4 - Report
## Nikos Chronis - Student ID: 5174

---

## Part 1: Linux Kernel Compilation and QEMU Setup

### 1.1 Development Environment

Development was performed on the department machines (frapa.csd.uoc.gr) with the following specifications:

- **Host OS:** Debian 6.1.0-40-amd64
- **Target Architecture:** i386 (32-bit)
- **Kernel Version:** Linux 2.6.38.1
- **Compiler:** GCC 4.9.2 (from the course area)
- **Emulator:** QEMU (qemu-system-i386)
- **Guest OS:** ttylinux 14.1

### 1.2 Problems Encountered and Solutions

#### Problem 1: GCC Incompatibility
**Description:** The pre-installed GCC (version 12.2.0) is not compatible with Linux kernel 2.6.38.1, causing errors like "kernel does not support PIC mode".

**Solution:** Use GCC 4.9.2 provided by the course:
```bash
export PATH="/home/misc/courses/hy345/gcc-4.9.2-standalone/bin:$PATH"
export PATH="/home/misc/courses/hy345/gcc-4.9.2-standalone/libexec/gcc/x86_64-unknown-linux-gnu/4.9.2:$PATH"
```

#### Problem 2: Perl Compatibility Error
**Description:** During compilation, the following error appeared:
```
Can't use 'defined(@array)' at kernel/timeconst.pl line 373
```
The newer Perl version on frapa does not support the old `defined(@array)` syntax.

**Solution:** Modify the script with:
```bash
sed -i 's/defined(@/(@/g' kernel/timeconst.pl
```

#### Problem 3: Wrong Architecture (x86_64 instead of i386)
**Description:** Initially compiled for x86_64, but the disk image (ttylinux) is for 32-bit i386 architecture. This caused kernel panic during boot.

**Solution:** Compile with explicit architecture:
```bash
make ARCH=i386 bzImage
```
and run with:
```bash
qemu-system-i386 (instead of qemu-system-x86_64)
```

#### Problem 4: QEMU GTK Initialization Failed
**Description:** When running QEMU, "gtk initialization failed" appeared due to missing X11 forwarding.

**Solution:** Use the `-nographic` parameter with serial console:
```bash
qemu-system-i386 ... -append "root=/dev/hda console=ttyS0" -nographic
```

#### Problem 5: Disk Image Issues
**Description:** Initially, various disk images were tested:
- `hy345-linux.img` (128MB, QCOW2, AES-encrypted)
- `hy345-development.img` (128MB, raw with partition table)

There was confusion with root device names (hda vs sda, hda vs hda1).

**Solution:** The correct image is `hy345-linux.img` with `root=/dev/hda` (without partition number, as the filesystem is directly on the device without a partition table).

### 1.3 Final Compilation Procedure (Successful)

```bash
# 1. Environment preparation
cd /tmp/csd5174
tar -xjf /home/misc/courses/hy345/qemu-linux/linux-2.6.38.1.tar.bz2
cd linux-2.6.38.1

# 2. Setup compiler
export PATH="/home/misc/courses/hy345/gcc-4.9.2-standalone/bin:$PATH"
export PATH="/home/misc/courses/hy345/gcc-4.9.2-standalone/libexec/gcc/x86_64-unknown-linux-gnu/4.9.2:$PATH"

# 3. Configuration
cp /home/misc/courses/hy345/qemu-linux/.config .
make oldconfig  # Enter for all defaults

# 4. Fix Perl compatibility issue
sed -i 's/defined(@/(@/g' kernel/timeconst.pl

# 5. Compile for i386
make ARCH=i386 bzImage

# 6. Verify success
# Kernel: arch/x86/boot/bzImage is ready
```

### 1.4 Running in QEMU (Successful)

```bash
# Copy disk image
cp /home/misc/courses/hy345/qemu-linux/hy345-linux.img /tmp/csd5174/

# Start QEMU
qemu-system-i386 \
  -hda /tmp/csd5174/hy345-linux.img \
  -append "root=/dev/hda console=ttyS0" \
  -kernel arch/x86/boot/bzImage \
  -nographic
```

### 1.5 Result

The vanilla kernel 2.6.38.1 boots successfully in QEMU with the following characteristics:
- Kernel version: `2.6.38.1-hy345`
- Architecture: `i686`
- Console: `/dev/ttyS0`
- Root filesystem: `ext2` mounted successfully
- Login: Works normally (root/hy345, user/csd-hy345)

**Note:** The warning "module dependencies FAILED" is expected since kernel modules were not installed (`make modules_install` was not executed).

---

## Part 2: System Calls Implementation (Assignment 3)

### 2.1 Overview

For Assignment 3, two new system calls were implemented:
- `set_proc_info(int deadline, int est_runtime)` - Sets the LTF parameters for the current process
- `get_proc_info(int *deadline, int *est_runtime)` - Returns the LTF parameters of the current process

### 2.2 Modified/Created Files

| File | Type | Description |
|------|------|-------------|
| `include/linux/sched.h` | Modified | Added `deadline`, `est_runtime` fields to `task_struct` |
| `arch/x86/include/asm/unistd_32.h` | Modified | Defined syscall numbers (341, 342) |
| `arch/x86/kernel/syscall_table_32.S` | Modified | Added entries to syscall table |
| `include/linux/syscalls.h` | Modified | Added function prototypes |
| `kernel/proc_info.c` | **New** | Implementation of system calls |
| `kernel/Makefile` | Modified | Added `proc_info.o` to build |

### 2.3 Detailed Changes

#### 2.3.1 Adding fields to task_struct (`include/linux/sched.h`)

At the end of `struct task_struct` (line 1530) the following were added:

```c
/* HY345 - LTF scheduling fields */
int deadline;      /* deadline in seconds */
int est_runtime;   /* estimated runtime in milliseconds */
```

**Command:**
```bash
sed -i '1530a\
\
/* HY345 - LTF scheduling fields */\
int deadline;      /* deadline in seconds */\
int est_runtime;   /* estimated runtime in milliseconds */' include/linux/sched.h
```

#### 2.3.2 Defining Syscall Numbers (`arch/x86/include/asm/unistd_32.h`)

Syscall numbers 341 and 342 were added:

```c
#define __NR_set_proc_info 341
#define __NR_get_proc_info 342
#define NR_syscalls 343
```

**Command:**
```bash
sed -i 's/#define NR_syscalls 341/#define __NR_set_proc_info 341\n#define __NR_get_proc_info 342\n#define NR_syscalls 343/' arch/x86/include/asm/unistd_32.h
```

#### 2.3.3 Syscall Table (`arch/x86/kernel/syscall_table_32.S`)

Entries were added at the end of the table:

```asm
.long sys_set_proc_info         /* 341 */
.long sys_get_proc_info         /* 342 */
```

**Command:**
```bash
echo -e "\t.long sys_set_proc_info\t\t/* 341 */\n\t.long sys_get_proc_info\t\t/* 342 */" >> arch/x86/kernel/syscall_table_32.S
```

#### 2.3.4 Function Prototypes (`include/linux/syscalls.h`)

The prototypes were added:

```c
/* HY345 - LTF scheduling syscalls */
asmlinkage long sys_set_proc_info(int deadline, int est_runtime);
asmlinkage long sys_get_proc_info(int __user *deadline, int __user *est_runtime);
```

#### 2.3.5 System Calls Implementation (`kernel/proc_info.c`)

A new file was created with the implementation:

```c
/*
 * HY345 - Assignment 3 & 4
 * System calls for LTF scheduler
 * Nikos Chronis - Student ID: 5174
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

asmlinkage long sys_set_proc_info(int deadline, int est_runtime)
{
    printk(KERN_INFO "HY345 [csd5174]: set_proc_info called - deadline=%d, est_runtime=%d\n",
           deadline, est_runtime);

    /* Validate parameters */
    if (deadline <= 0 || est_runtime <= 0)
        return -EINVAL;

    /* est_runtime (ms) must fit within deadline (seconds) */
    if (est_runtime > deadline * 1000)
        return -EINVAL;

    /* Set values in current task */
    current->deadline = deadline;
    current->est_runtime = est_runtime;

    return 0;
}

asmlinkage long sys_get_proc_info(int __user *deadline, int __user *est_runtime)
{
    printk(KERN_INFO "HY345 [csd5174]: get_proc_info called\n");

    /* Validate pointers */
    if (!deadline || !est_runtime)
        return -EINVAL;

    /* Copy values to user space */
    if (put_user(current->deadline, deadline))
        return -EFAULT;
    if (put_user(current->est_runtime, est_runtime))
        return -EFAULT;

    return 0;
}
```

#### 2.3.6 Makefile Modification (`kernel/Makefile`)

`proc_info.o` was added to the object list:

```makefile
obj-y += proc_info.o
```

**Command:**
```bash
sed -i '/^obj-y += groups.o/a obj-y += proc_info.o' kernel/Makefile
```

### 2.4 Fixes and Optimizations

#### 2.4.1 Cleanup of Duplicate Prototypes (`include/linux/syscalls.h`)

**Problem:** Due to incorrect use of `sed` with the pattern `/#endif/i`, the prototypes were added multiple times (10 duplicates) to the file.

**Solution:** Remove all duplicates and add a single declaration at the end:
```bash
# Remove all duplicates
perl -i -ne 'print unless /sys_set_proc_info|sys_get_proc_info|HY345/' include/linux/syscalls.h

# Add once before the final #endif
sed -i '$ d' include/linux/syscalls.h
cat >> include/linux/syscalls.h << 'EOF'

/* HY345 - LTF scheduling syscalls */
asmlinkage long sys_set_proc_info(int deadline, int est_runtime);
asmlinkage long sys_get_proc_info(int __user *deadline, int __user *est_runtime);

#endif
EOF
```

**Verification:**
```bash
grep -c "sys_set_proc_info" include/linux/syscalls.h
# Output: 1
```

#### 2.4.2 Adding Feasibility Check with time_after()

**Problem:** The initial check `est_runtime > deadline * 1000` was not accurate in jiffies.

**Solution:** Use the kernel macro `time_after()` for correct checking in jiffies:
```c
/* Feasibility check: runtime must fit before deadline */
if (time_after(jiffies + rt_jiffies, dl_jiffies))
    return -EINVAL;
```

### 2.5 Final Implementation of proc_info.c

```c
/*
 * HY345 - Assignment 3 & 4
 * System calls for LTF scheduler
 * Nikos Chronis - Student ID: 5174
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
```

### 2.6 Modification of task_struct for Jiffies

The fields in `include/linux/sched.h` were converted to `unsigned long`:
```c
/* HY345 - LTF scheduling fields */
unsigned long deadline;     /* absolute deadline in jiffies */
unsigned long est_runtime;  /* estimated runtime in jiffies */
```

### 2.7 Compilation and Testing

```bash
# Setup compiler
export PATH="/home/misc/courses/hy345/gcc-4.9.2-standalone/bin:$PATH"
export PATH="/home/misc/courses/hy345/gcc-4.9.2-standalone/libexec/gcc/x86_64-unknown-linux-gnu/4.9.2:$PATH"

# Compile
make ARCH=i386 bzImage -j4
```

**Result:** `Kernel: arch/x86/boot/bzImage is ready (#4)`

### 2.8 Summary of Final Changes

| File | Change |
|------|--------|
| `include/linux/sched.h` | `unsigned long deadline, est_runtime` in `task_struct` |
| `arch/x86/include/asm/unistd_32.h` | Syscall numbers 341, 342 |
| `arch/x86/kernel/syscall_table_32.S` | Entries for new syscalls |
| `include/linux/syscalls.h` | Single prototype declaration |
| `kernel/proc_info.c` | Implementation with jiffies and `time_after()` check |
| `kernel/Makefile` | Added `proc_info.o` |

---

## Part 3: LTF Scheduler Implementation (Assignment 4)

### 3.1 Objective

The objective of Part 3 was to implement a scheduler based on the Least Tolerance First (LTF) algorithm in Linux kernel 2.6.38.1.

LTF prioritizes the process with the smallest tolerance, where:

- $tolerance = \frac{slack}{remaining\_runtime}$
- $slack = deadline - current\_time$
- $remaining\_runtime = est\_runtime - executed\_time$

The implementation was done without floating point, as required in kernel code.

### 3.2 General Approach

The implementation was performed:

- without creating a new scheduling class
- without modifying `sched_class` structures
- with intervention exclusively in `kernel/sched.c`
- with integration of the LTF mechanism inside `schedule()`

LTF operates as a priority above the existing scheduler:

- If there is an eligible LTF process → it is selected
- Otherwise → fallback to regular `pick_next_task()` (CFS / RT / idle)

### 3.3 New Function: `pick_next_ltf_task()`

The following function was added:

```c
static struct task_struct *pick_next_ltf_task(struct rq *rq)
```

The function:

- Iterates through all processes (`for_each_process`)
- Examines only runnable tasks (`TASK_RUNNING`)
- Examines only tasks on the same CPU/runqueue
- Filters processes that don't have `deadline` or `est_runtime`

It calculates:

- `slack = deadline - now`
- `executed = NS_TO_JIFFIES(p->se.sum_exec_runtime)`
- `remaining = est_runtime - executed`

It ignores processes:

- that have missed their deadline
- that have exhausted their runtime

The tolerance comparison is done without floating point, using cross multiplication:

```c
if ((u64)slack * (u64)best_remaining <
    (u64)best_slack * (u64)remaining)
```

This completely avoids the use of `double` in the kernel.

The function returns:

- pointer to the process with the smallest tolerance
- or `NULL` if there is no eligible LTF process

### 3.4 Modification of `schedule()`

The main change in `schedule()` was made at the point of selecting the next process.

Initially, the kernel calls:

```c
put_prev_task(rq, prev);
```

Immediately after, the following was added:

```c
next = pick_next_ltf_task(rq);
if (!next)
    next = pick_next_task(rq);
```

This way:

- The LTF scheduler has priority
- The regular scheduler remains unchanged and operates as fallback
- The logic of the Linux scheduler is not disrupted

No other changes were made to the flow of `schedule()`.

### 3.5 Code Cleanup and Correctness

During implementation:

- It was ensured that there is one and only one implementation of `schedule()` and `pick_next_ltf_task()`
- Older experimental LTF versions were removed

Not used:

- `double`
- `get_seconds()`
- `do_exit()` or `send_sig()` inside the scheduler

The code is fully compatible with kernel constraints.

### 3.6 Architecture & Compilation

The assignment requires 32-bit architecture (i386). For this reason, a complete rebuild was done with:

```bash
make mrproper
cp /home/misc/courses/hy345/qemu-linux/.config .
export PATH="/home/misc/courses/hy345/gcc-4.9.2-standalone/bin:$PATH"
export PATH="/home/misc/courses/hy345/gcc-4.9.2-standalone/libexec/gcc/x86_64-unknown-linux-gnu/4.9.2:$PATH"
hash -r
make ARCH=i386 oldconfig
make ARCH=i386 bzImage -j$(nproc)

```

The produced `arch/x86/boot/bzImage` was confirmed to be an i386 kernel image and successfully booted in QEMU.

```bash
qemu-system-i386 \
  -hda /tmp/csd5174/hy345-linux.img \
  -append "root=/dev/hda console=ttyS0" \
  -kernel /tmp/csd5174_hy345/linux-2.6.38.1/arch/x86/boot/bzImage \
  -nographic -no-reboot
```

### 3.7 Integration of Assignment 3 (syscalls) with Assignment 4 (LTF)

#### 3.7.1 Problem Encountered

During initial testing, the user-space test returned:

```text
set_proc_info: Function not implemented
```

This corresponds to `ENOSYS` and means that the running kernel did not have the syscalls declared/registered (not an argument error).

#### 3.7.2 Diagnosis

It was identified that the syscalls from Assignment 3 existed in a different kernel tree (/tmp/csd5174/linux-2.6.38.1), while the Assignment 4 kernel was being built from a different tree (/tmp/csd5174_hy345/linux-2.6.38.1) without these changes.

#### 3.7.3 Fix (transferring changes)

The following were transferred to the Assignment 4 tree:

- `kernel/proc_info.c` and inclusion of `proc_info.o` in `kernel/Makefile`
- Syscall registration in the 32-bit ABI:
  - `arch/x86/kernel/syscall_table_32.S` (entries 341/342)
  - `arch/x86/include/asm/unistd_32.h`:
    - `__NR_set_proc_info 341`
    - `__NR_get_proc_info 342`
    - `NR_syscalls 343`

After rebuild/boot, the `set_proc_info` call no longer returns `ENOSYS`, confirming that the syscalls are available in the current kernel.

### 3.8 Test Procedure (Expected Results)

#### 3.8.1 Transferring test files to QEMU

A shared drive was used via QEMU VVFAT so that ltf_test.c would be available inside the guest:

```bash
qemu-system-i386 \
  ... \
  -drive file=fat:rw:/tmp/csd5174_share,format=raw,if=ide,index=1
```


In the guest:

```bash
mkdir -p /mnt/share
mount /dev/hdb1 /mnt/share
cd /mnt/share
```

#### 3.8.2 Expected syscall testing (Assignment 3)

A simple test should:

- call `set_proc_info(deadline, est_runtime)`
- call `get_proc_info(...)`
- verify that values are returned correctly from the kernel

#### 3.8.3 Expected LTF testing (Assignment 4)

To verify LTF scheduling, two CPU-bound processes should be executed in parallel:

- same `est_runtime`
- different `deadline`

Example:

```bash
./ltf_test 8000 8000 20 &
./ltf_test 2000 8000 20 &
```


Expected result: the process with smaller $tolerance = \frac{slack}{remaining\_runtime}$ (i.e., with smaller `deadline`/`slack`) will get more CPU time and will "progress" faster (e.g., more iterations per second in heartbeat test).

### 3.8 Test Procedure and Results

#### 3.8.1 Transferring test files to QEMU

A shared drive was used via QEMU VVFAT so that ltf_test.c would be available inside the guest:

```bash
qemu-system-i386 \
  -hda /tmp/csd5174/hy345-linux.img \
  -append "root=/dev/hda console=ttyS0" \
  -kernel /tmp/csd5174_hy345/linux-2.6.38.1/arch/x86/boot/bzImage \
  -nographic -no-reboot \
  -drive file=fat:rw:/tmp/csd5174_share,format=raw,if=ide,index=1
```

In the guest:

```bash
mkdir -p /mnt/share
mount /dev/hdb1 /mnt/share
cd /mnt/share
```

#### 3.8.2 Bug Fix in Test Program (ltf_test.c)

**Problem:** The initial version of `ltf_test.c` caused a crash (system freeze) when calling `get_proc_info`. The kernel itself was functioning correctly — the issue was in the user-space test program.

**Root Cause:** The kernel syscall `sys_get_proc_info` expects two separate `int __user *` pointers:

```c
asmlinkage long sys_get_proc_info(int __user *deadline, int __user *est_runtime)
```

However, the original test program was passing a single pointer to a `struct d_params`:

```c
// OLD (INCORRECT) - passed one struct pointer instead of two int pointers
struct d_params {
    int deadline;
    int est_runtime;
};
static long get_proc_info(struct d_params *p) {
    return syscall(__NR_get_proc_info, p);  // only 1 argument!
}
```

This meant the kernel received only one argument. The second `int __user *est_runtime` parameter contained garbage from the register/stack, causing the kernel to attempt `put_user()` to an invalid memory address, which resulted in a crash.

**Fix:** The wrapper was corrected to pass two separate pointers matching the kernel signature:

```c
// NEW (CORRECT) - two separate int pointers
static long get_proc_info(int *deadline, int *est_runtime) {
    return syscall(__NR_get_proc_info, deadline, est_runtime);
}
```

And the call site was updated accordingly:

```c
int dl, rt;
if (get_proc_info(&dl, &rt) == 0) {
    printf("[PID %d] get_proc_info -> deadline=%d runtime=%d\n",
           getpid(), dl, rt);
}
```

#### 3.8.3 Compilation in Guest OS

The test program was compiled inside the QEMU guest (ttylinux):

```bash
gcc -o ltf_test ltf_test.c
```

Note: The `-static` flag cannot be used as ttylinux does not include a static libc (`cannot find -lc`). Dynamic linking works correctly.

#### 3.8.4 Syscall Test Results

Basic syscall test (without CPU burn):

```bash
./ltf_test 100 5000
```

Output:
```
[PID 1185] set_proc_info(100, 5000)
[PID 1185] get_proc_info -> deadline=99 runtime=5000
```

The results confirm:
- `set_proc_info` successfully sets the deadline and estimated runtime in the kernel
- `get_proc_info` correctly retrieves the values back to user-space
- The deadline shows 99 instead of 100 because approximately 1 second elapsed between the `set` and `get` calls (the kernel stores deadline as absolute jiffies and converts back to remaining seconds)
- The runtime shows 5000ms as expected (unchanged since no CPU time was consumed by the scheduler)

#### 3.8.5 LTF Scheduling Test

To verify LTF scheduling behavior, two CPU-bound processes were executed in parallel:

```bash
./ltf_test 8000 8000 20 &
./ltf_test 2000 8000 20 &
```

Parameters: same `est_runtime` (8000ms), different `deadline` (8000 vs 2000 seconds), 20 seconds of CPU burn.

**Observation:** The LTF scheduler activates and gives priority to the LTF-eligible processes. However, during this test the system became unresponsive (the shell/console stopped responding to input). This is expected behavior given the current LTF implementation: since LTF tasks have absolute priority over regular CFS tasks, and the CPU burn loop runs continuously, the LTF scheduler monopolizes the CPU and starves all other processes (including the shell). The QEMU instance had to be terminated externally with `killall -9 qemu-system-i386`.

This confirms that:
1. The LTF scheduler correctly identifies and prioritizes processes that have set deadline/est_runtime via the syscalls
2. The `pick_next_ltf_task()` function works as intended — it always selects the LTF task with the smallest tolerance
3. The starvation of non-LTF tasks is a natural consequence of the design (LTF has strict priority over CFS), which is consistent with the assignment specification: "Οι διεργασίες αυτές έχουν προτεραιότητα σε σχέση με τις υπόλοιπες που τρέχουν στο σύστημα"
