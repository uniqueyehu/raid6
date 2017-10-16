/**
* generate syndrome module
* use kmalloc to get memory
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>

#define DISKS 5
#define MAXBYTES 16

typedef unsigned char U8X;
typedef unsigned int U32X;
typedef unsigned char * PCHARX;

//Q校验计算所需的4个表
U8X gf_mul[256][256];
U8X gf_exp[256];
U8X gf_inv[256];
U8X gf_exi[256];

void raid6_gen_syndrome()
{
    unsigned char **a_pcBuf;

    U8X i, j;
    U8X p = 0, q = 0;
    int a, b;

    a_pcBuf = kmalloc(DISKS * sizeof(char *), 0);
    for(i = 0; i < DISKS; i++)
        a_pcBuf[i] = kmalloc(MAXBYTES, 0);
    memset(a_pcBuf[0], 0, MAXBYTES);
    memset(a_pcBuf[1], 0, MAXBYTES);
    for(i = 2; i < DISKS; i++)
        for(j = 0; j < MAXBYTES; j++)
            a_pcBuf[i][j] = 'a' + i + j;

    printk("\n*******************************\n");
    printk("Data are : \n");
    for(i = 0; i < DISKS; i++)
    {
        printk("Disk : %d\n", i);
        if(i == 0 || i == 1)
        {
            for(j = 0; j < MAXBYTES - 1; j++)
                printk("%3d ", a_pcBuf[i][j]);
            printk("%3d\n\n", a_pcBuf[i][j]);
        }
        else
        {
            for(j = 0; j < MAXBYTES - 1; j++)
                printk("%c ", a_pcBuf[i][j]);
            printk("%c\n\n", a_pcBuf[i][j]);
        }
    }
    printk("*******************************\n");

    printk("\n*******************************\n");
    printk("raid6_gen_syndrome : \n");
    for(i = 0; i < MAXBYTES; i++)
    {
        p = 0, q = 0;
        for(j = 2; j < DISKS; j++)
        {
            p ^= a_pcBuf[j][i];
            q ^= gf_mul[gf_exp[j]][a_pcBuf[j][i]];
        }
        a_pcBuf[0][i] = p;
        a_pcBuf[1][i] = q;
    }
    printk("*******************************\n");

    printk("\n*******************************\n");
    printk("After syndrome are : \n");
    for(i = 0; i < DISKS; i++)
    {
        printk("Disk : %d\n", i);
        if(i == 0 || i == 1)
        {
            for(j = 0; j < MAXBYTES - 1; j++)
                printk("%3d ", a_pcBuf[i][j]);
            printk("%3d\n\n", a_pcBuf[i][j]);
        }
        else
        {
            for(j = 0; j < MAXBYTES - 1; j++)
                printk("%c ", a_pcBuf[i][j]);
            printk("%c\n\n", a_pcBuf[i][j]);
        }
    }
    printk("*******************************\n");

    for(i = 0; i < DISKS; i++)
        kfree(a_pcBuf[i]);
    kfree(a_pcBuf);

}

int ldgftalbe_init(void)
{
    struct file *fp = 0;
    mm_segment_t fs = get_fs();
    int ret = 0;
    long err = 0;
    int i, j;

    printk("********************\n");
    printk("Begin to load tables!\n");
    printk("********************\n");

    set_fs(KERNEL_DS);
    fp = filp_open("/home/gftable/gf_table", O_RDONLY, 0);
    if((err = IS_ERR(fp)))
    {
        printk("open file error %ld\n", err);
        return -1;
    }

    //************************************************************
    fp->f_pos = 0;
    ret = vfs_read(fp, gf_mul, sizeof(gf_mul), &fp->f_pos);
    if(ret < 0)
    {
        printk("vfs_read gf_mul ret = %d\n", ret);
        return -1;
    }

    ret = vfs_read(fp, gf_exp, sizeof(gf_exp), &fp->f_pos);
    if(ret < 0)
    {
        printk("vfs_read gf_exp ret = %d\n", ret);
        return -1;
    }

    ret = vfs_read(fp, gf_inv, sizeof(gf_inv), &fp->f_pos);
    if(ret < 0)
    {
        printk("vfs_read gf_inv ret = %d\n", ret);
        return -1;
    }

    ret = vfs_read(fp, gf_exi, sizeof(gf_exi), &fp->f_pos);
    if(ret < 0)
    {
        printk("vfs_read gf_exi ret = %d\n", ret);
        return -1;
    }
    //************************************************************

    filp_close(fp, 0);
    set_fs(fs);

    raid6_gen_syndrome();

    return 0;
}

void ldgftable_exit(void)
{
    printk("********************\n");
    printk("Bye World!\n");
    printk("********************\n");
}

module_init(ldgftalbe_init);
module_exit(ldgftable_exit);

MODULE_LICENSE("GPL");
MODULE_LICENSE("yehu");
