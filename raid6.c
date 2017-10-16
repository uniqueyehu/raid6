/*
 * 1、1 Byte为单位来计算raid6校验
 * 2、坏两个盘时数据恢复
 * 3、模拟磁盘数据使用动态申请的内存
 * raid6_v1.1
 */
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#define DISKS 5
#define MAXBYTES 16

// 这里使用的是unsigned类型，需要注意非unsigned类型的情况
typedef unsigned char U8X;
typedef unsigned int U32X;
typedef unsigned char * PCHARX;

//U8X data[DISKS][MAXBYTES];

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

//输出磁盘数据
void output_data(U32X v_BufNum, U32X v_uBytes, PCHARX *v_ppBuf)
{
    U32X i, j;
    for(i = 0; i < v_BufNum; i++)
    {
        printf("Disk : %d\n", i);
        if(i == 0 || i == 1)
        {
            for(j = 0; j < v_uBytes - 1; j++)
                printf("%3d ", v_ppBuf[i][j]);
            printf("%3d\n\n", v_ppBuf[i][j]);
        }
        else
        {
            for(j = 0; j < v_uBytes - 1; j++)
                printf("%c ", v_ppBuf[i][j]);
            printf("%c\n\n", v_ppBuf[i][j]);
        }
    }
}

/**
 * @brief gen_syndrome
 * @param v_BufNum
 * @param v_uBytes
 * @param v_ppBuf
 *
 * 1、生成校验，将P校验放在data[0]中，Q校验放在data[1]中
 * 2、以1 Byte为单位来进行计算
 */
void raid6_gen_syndrome(U32X v_BufNum, U32X v_uBytes, PCHARX *v_ppBuf)
{
    U32X i, j;
    U8X p = 0, q = 0;

    printf("***********************\n");
    printf("In raid6_gen_syndrome : \n");

    for(i = 0; i < v_uBytes; i++)
    {
        p = 0, q = 0;
        for(j = 2; j < v_BufNum; j++)
        {
            p ^= v_ppBuf[j][i];
            q ^= gf_mul[gf_exp[j]][v_ppBuf[j][i]];
        }
        v_ppBuf[0][i] = p;
        v_ppBuf[1][i] = q;
    }

    printf("After syndrome :\n");
    output_data(v_BufNum, v_uBytes, v_ppBuf);
}

/**
 * @brief raid6_gen_pxor
 * @param v_BufNum
 * @param v_uBytes
 * @param v_ppBuf
 *
 * 1、raid6单独计算P校验
 * 2、计算结果保存在data[0]中
 */
void raid6_gen_p(U32X v_BufNum, U32X v_uBytes, PCHARX *v_ppBuf)
{
    U32X i, j;
    U8X p = 0;

    printf("***********************\n");
    printf("In raid6_gen_p : \n");

    for(i = 0; i < v_uBytes; i++)
    {
        p = 0;
        for(j = 2; j < v_BufNum; j++)
        {
            p ^= v_ppBuf[j][i];
        }
        v_ppBuf[0][i] = p;
    }

    printf("After syndrome :\n");
    output_data(v_BufNum, v_uBytes, v_ppBuf);
}

/**
 * @brief raid6_gen_q
 * @param v_BufNum
 * @param v_uBytes
 * @param v_ppBuf
 *
 * 1、单独计算Q校验
 * 2、计算结果放在data[1]中
 */
void raid6_gen_q(U32X v_BufNum, U32X v_uBytes, PCHARX *v_ppBuf)
{
    U32X i, j;
    U8X q = 0;

    printf("***********************\n");
    printf("In raid6_gen_q : \n");

    for(i = 0; i < v_uBytes; i++)
    {
        q = 0;
        for(j = 2; j < v_BufNum; j++)
        {
            q ^= gf_mul[gf_exp[j]][v_ppBuf[j][i]];
        }
        v_ppBuf[1][i] = q;
    }

    printf("After syndrome :\n");
    output_data(v_BufNum, v_uBytes, v_ppBuf);
}

/**
 * @brief XOR_Execute
 * @param v_uBufNum
 * @param v_uBytes
 * @param v_ppBuf
 * @param pXor
 *
 * 1、raid5中计算异或校验函数
 * 2、将P校验放在data[0]中
 */
void XOR_Execute(U32X v_uBufNum, U32X v_uBytes, PCHARX *v_ppBuf, int pXor)
{

}

/**
 * @brief raid6_2data_recov
 * @param disks
 * @param faila
 * @param failb
 * @param v_ppBuf
 *
 * 1、坏两个数据盘时的恢复过程
 * 2、以1 Byte为单位
 * 3、首先要定位坏盘，定好位之后，直接结算即可
 */
void raid6_2data_recov(int disks, int faila, int failb, PCHARX *v_ppBuf)
{
    int i;
    U8X *pbmul, *qmul;
    U8X *p, *q, *dp, *dq;
    U8X px, qx, db;
    U8X zero[MAXBYTES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};//只要一个就行

    printf("***********************\n");
    printf("data + data failed!\n");
    printf("Before syndrome :\n");
    output_data(disks, MAXBYTES, v_ppBuf);

    p = v_ppBuf[0];
    q = v_ppBuf[1];

    dp = v_ppBuf[faila];
    v_ppBuf[faila] = zero;
    v_ppBuf[0] = dp;//v_ppBuf[0]现在指向坏盘a，用来存Pxy

    dq = v_ppBuf[failb];
    v_ppBuf[failb] = zero;
    v_ppBuf[1] = dq;//v_ppBuf[0]现在指向坏盘b，用来存Qxy

    raid6_gen_syndrome(disks, MAXBYTES, v_ppBuf);

    v_ppBuf[faila] = dp;
    v_ppBuf[failb] = dq;
    v_ppBuf[0] = p;
    v_ppBuf[1] = q;

    pbmul = gf_mul[gf_exi[failb - faila]];
    qmul = gf_mul[gf_inv[gf_exp[faila] ^ gf_exp[failb]]];

    for(i = 0; i < MAXBYTES; i++)
    {
        px = *p ^ *dp;
        qx = qmul[*q ^ *dq];
        *dq++ = db = pbmul[px] ^ qx;
        *dp++ = db ^ px;
        p++; q++;
    }
}

/**
 * @brief raid6_1data_recov_usep
 * @param disks
 * @param faila
 * @param v_ppBuf
 *
 * 1、坏一块数据盘
 * 2、使用p校验，类似RAID5的方式恢复数据
 */
void raid6_1data_recov_usep(int disks, int faila, PCHARX *v_ppBuf)
{
    int i;
    U8X *p, *dp;
    U8X zero[MAXBYTES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    printf("***********************\n");
    printf("1 data failed!\n");
    printf("Before syndrome :\n");
    output_data(disks, MAXBYTES, v_ppBuf);

    p = v_ppBuf[0];

    dp = v_ppBuf[faila];
    v_ppBuf[faila] = zero;
    v_ppBuf[0] = dp;//v_ppBuf[0]现在指向坏盘a，用来存Pxy

    raid6_gen_p(disks, MAXBYTES, v_ppBuf);

    v_ppBuf[faila] = dp;
    v_ppBuf[0] = p;

    for(i = 0; i < MAXBYTES; i++)
    {
        *dp = *p ^ *dp;
        p++; dp++;
    }
}

/**
 * @brief raid6_1data_recov_useq
 * @param disks
 * @param faila
 * @param v_ppBuf
 *
 * 1、坏一块数据盘
 * 2、使用q校验来恢复数据
 */
void raid6_1data_recov_useq(int disks, int faila, PCHARX *v_ppBuf)
{
    int i;
    U8X *qmul;
    U8X *q, *dq;
    U8X zero[MAXBYTES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    printf("***********************\n");
    printf("1 data + q failed!\n");
    printf("Before syndrome :\n");
    output_data(disks, MAXBYTES, v_ppBuf);

    q = v_ppBuf[1];

    dq = v_ppBuf[faila];
    v_ppBuf[faila] = zero;
    v_ppBuf[1] = dq;//v_ppBuf[1]现在指向坏盘a，用来存Qxy

    raid6_gen_q(disks, MAXBYTES, v_ppBuf);

    v_ppBuf[faila] = dq;
    v_ppBuf[1] = q;

    qmul = gf_mul[gf_inv[gf_exp[faila]]];

    for(i = 0; i < MAXBYTES; i++)
    {
        *dq = qmul[*q ^ *dq];
        dq++; q++;
    }
}

int main(int argc, char *argv[])
{
    unsigned char **a_pcBuf;

    gen_gftable();

    U8X i, j;
    int a, b;

    //模拟生成盘中数据
    a_pcBuf = malloc(DISKS * sizeof(char *));
    for(i = 0; i < DISKS; i++)
    {
        a_pcBuf[i] = malloc(MAXBYTES);
    }
    memset(a_pcBuf[0], 0, MAXBYTES);
    memset(a_pcBuf[1], 0, MAXBYTES);
    for(i = 2; i < DISKS; i++)
    {
        for(j = 0; j < MAXBYTES; j++)
            a_pcBuf[i][j] = 'a' + i + j;
    }

    //输出数据
    printf("***********************\n");
    printf("Data are : \n");
    output_data(DISKS, MAXBYTES, a_pcBuf);

    raid6_gen_syndrome(DISKS, MAXBYTES, a_pcBuf);

    //************************************
    //1、坏两块数据盘时，恢复
    //需要引入新的数据恢复函数
    a = 2, b = 3;
    memset(a_pcBuf[a], '2', MAXBYTES);
    memset(a_pcBuf[b], '5', MAXBYTES);
    raid6_2data_recov(DISKS, a, b, a_pcBuf);
    printf("***********************\n");
    printf("Recov data are : \n");
    output_data(DISKS, MAXBYTES, a_pcBuf);
    //************************************

    //************************************
    //2、坏一块数据盘和一块P校验盘时，恢复
    //需要引入新的数据恢复函数
//    a = 2;
//    memset(a_pcBuf[a], '2', MAXBYTES);
//    memset(a_pcBuf[0], 0, MAXBYTES);
//    raid6_1data_recov_useq(DISKS, a, a_pcBuf);
//    raid6_gen_p(DISKS, MAXBYTES, a_pcBuf);
//    printf("***********************\n");
//    printf("Recov data are : \n");
//    output_data(DISKS, MAXBYTES, a_pcBuf);

    //************************************
    //3、坏一块数据盘和一块Q校验盘时，恢复
    //复用之前的函数即可
//    a = 2;
//    memset(a_pcBuf[a], '2', MAXBYTES);
//    memset(a_pcBuf[1], 0, MAXBYTES);
//    raid6_1data_recov_usep(DISKS, a, a_pcBuf);
//    raid6_gen_q(DISKS, MAXBYTES, a_pcBuf);
//    printf("***********************\n");
//    printf("Recov data are : \n");
//    output_data(DISKS, MAXBYTES, a_pcBuf);

    //************************************
    //4、坏一块P校验盘和一块Q校验盘时，恢复
    //直接重新计算P+Q校验
//    memset(a_pcBuf[0], 0, MAXBYTES);
//    memset(a_pcBuf[1], 0, MAXBYTES);
//    raid6_gen_syndrome(DISKS, MAXBYTES, a_pcBuf);
//    printf("***********************\n");
//    printf("Recov data are : \n");
//    output_data(DISKS, MAXBYTES, a_pcBuf);

    //************************************
    //5、坏一块数据盘时，恢复
    //需要引入新的数据恢复函数
//    a = 2;
//    memset(a_pcBuf[a], '2', MAXBYTES);
//    raid6_1data_recov_usep(DISKS, a, a_pcBuf);
//    printf("***********************\n");
//    printf("Recov data are : \n");
//    output_data(DISKS, MAXBYTES, a_pcBuf);

    //************************************
    //6、坏一块P校验盘时，恢复
    //直接重新计算P校验
//    memset(a_pcBuf[0], 0, MAXBYTES);
//    raid6_gen_p(DISKS, MAXBYTES, a_pcBuf);
//    printf("***********************\n");
//    printf("Recov data are : \n");
//    output_data(DISKS, MAXBYTES, a_pcBuf);

    //************************************
    //7、坏一块Q校验盘时，恢复
    //直接重新计算Q校验
//    memset(a_pcBuf[1], 0, MAXBYTES);
//    raid6_gen_q(DISKS, MAXBYTES, a_pcBuf);
//    printf("***********************\n");
//    printf("Recov data are : \n");
//    output_data(DISKS, MAXBYTES, a_pcBuf);

    //************************************
    //释放空间
    for(i = 0; i < DISKS; i++)
        free(a_pcBuf[i]);
    free(a_pcBuf);

    return 0;
}
