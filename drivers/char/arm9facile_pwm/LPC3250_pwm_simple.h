/* templates for file operations */
int LPC3250_pwm_simple_open(struct inode *inode, struct file *file);
int LPC3250_pwm_simple_release(struct inode *inode, struct file *file);
ssize_t LPC3250_pwm_simple_read(struct file *file, char * buf, size_t count, loff_t *ppos);
ssize_t LPC3250_pwm_simple_write(struct file *file, const char __user * buf, size_t count, loff_t *ppos);
static loff_t LPC3250_pwm_simple_llseek(struct file *file, loff_t offset, int orig);
long LPC3250_pwm_simple_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

