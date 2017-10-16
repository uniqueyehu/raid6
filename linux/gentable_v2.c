/**
* generate Galois table module
* write the tables into a file
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

typedef unsigned char U8X;
typedef unsigned int U32X;
typedef unsigned char * PCHARX;

//Q校验计算所需的4个表
U8X gf_mul[256][256];
U8X gf_exp[256];
U8X gf_inv[256];
U8X gf_exi[256];

//伽罗华域的乘法
static U8X gfmul(U8X a, U8X b)
{
    U8X v = 0;

    while (b) {
        if (b & 1)
            v ^= a;
        a = (a << 1) ^ (a & 0x80 ? 0x1d : 0);
        b >>= 1;
    }

    return v;
}

static U8X gfpow(U8X a, int b)
{
    U8X v = 1;

    b %= 255;
    if (b < 0)
        b += 255;

    while (b) {
        if (b & 1)
            v = gfmul(v, a);
        a = gfmul(a, a);
        b >>= 1;
    }

    return v;
}

//4个表的生成函数
void gen_gftable()
{
    int i, j, k;
    U8X v;
    U8X exptbl[256], invtbl[256];

    /* Compute multiplication table */
    for (i = 0; i < 256; i++) {
        for (j = 0; j < 256; j += 8) {
            for (k = 0; k < 8; k++) {
                gf_mul[i][j + k] = gfmul(i, j + k);
            }
        }
    }

    /* Compute power-of-2 table (exponent) */
    v = 1;
    for (i = 0; i < 256; i += 8) {
        for (j = 0; j < 8; j++) {
            gf_exp[i + j] = exptbl[i + j] = v;
            v = gfmul(v, 2);
            if (v == 1)
                v = 0;	/* For entry 255, not a real entry */
        }
    }

    /* Compute inverse table x^-1 == x^254 */
    for (i = 0; i < 256; i += 8) {
        for (j = 0; j < 8; j++) {
            gf_inv[i + j] = invtbl[i + j] = v = gfpow(i + j, 254);
        }
    }

    /* Compute inv(2^x + 1) (exponent-xor-inverse) table */
    for (i = 0; i < 256; i += 8) {
        for (j = 0; j < 8; j++){
            gf_exi[i + j] = v = invtbl[exptbl[i + j] ^ 1];
        }

    }
}

int gentalbe_init(void)
{
    struct file *fp = 0;
    mm_segment_t fs = get_fs();
    int ret = 0;
    long err = 0;
    int i, j;

    printk("********************\n");
    printk("Begin to gen tables!\n");
    printk("********************\n");

    gen_gftable();

    set_fs(KERNEL_DS);
    fp = filp_open("/home/gftable/gf_table", O_RDWR|O_CREAT, 0);
    if((err = IS_ERR(fp)))
    {
        printk("open file error %ld\n", err);
        return -1;
    }

    //************************************************************
    fp->f_pos = 0;
    ret = vfs_write(fp, gf_mul, sizeof(gf_mul), &fp->f_pos);
    if(ret < 0)
    {
        printk("vfs_write gf_mul ret = %d\n", ret);
        return -1;
    }

    ret = vfs_write(fp, gf_exp, sizeof(gf_exp), &fp->f_pos);
    if(ret < 0)
    {
        printk("vfs_write gf_exp ret = %d\n", ret);
        return -1;
    }

    ret = vfs_write(fp, gf_inv, sizeof(gf_inv), &fp->f_pos);
    if(ret < 0)
    {
        printk("vfs_write gf_inv ret = %d\n", ret);
        return -1;
    }

    ret = vfs_write(fp, gf_exi, sizeof(gf_exi), &fp->f_pos);
    if(ret < 0)
    {
        printk("vfs_write gf_exi ret = %d\n", ret);
        return -1;
    }
    //************************************************************

    filp_close(fp, 0);
    set_fs(fs);

    printk("********************\n");
    printk("Success to gen tables!\n");
    printk("********************\n");

    return 0;
}

void gentable_exit(void)
{
    printk("********************\n");
    printk("Bye World!\n");
    printk("********************\n");
}

module_init(gentalbe_init);
module_exit(gentable_exit);

MODULE_LICENSE("GPL");
MODULE_LICENSE("yehu");
