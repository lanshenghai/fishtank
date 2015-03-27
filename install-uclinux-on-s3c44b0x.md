


> Written with [StackEdit](https://stackedit.io/).

下载相关软件包
==============
将下载的所有东西，放到文件夹m68k-elf-tools_tools-20030314中  

下载下面这个目录里面所有的文件：
 http://www.uclinux.org/pub/uClinux/arm-elf-tools/tools-20030314/

下载linux kernel的patch文件：
 http://www.uclinux.org/pub/uClinux/uClinux-2.4.x/uClinux-2.4.32-uc0.diff.gz
     
下载linux kernel：
到网上任意找2.4.32 kernel

     
解压
====
1. 解压linux kernel

 > tar -jxvf linux-2.4.32.tar.bz2

1. 解压kernel patch

 >gunzip uClinux-2.4.32-uc0.diff.gz

1. patch:

 >cd linux-2.4.32
 >patch -p1 < ../uClinux-2.4.32-uc0.diff
 >cd ..

1. 解压uClibc-20030314.tar.gz

 >tar -zxvf uClibc-20030314.tar.gz

编译 
====
第一次：

 > sudo ./build-uclinux-tools.sh build

出错以后继续：

 > sudo ./build-uclinux-tools.sh continue

用于定位问题（编译debug版本）：

 > sudo CFLAGS=-g ./build-uclinux-tools.sh build
 > 
 > sudo CFLAGS=-g ./build-uclinux-tools.sh continue

修复编译错误：

1. gcc-2.95.3/gcc/config/arm/arm.c:531
    注释掉:

    > //arm_prog_mode = TARGET_APCS_32 ? PROG_MODE_PROG32 : PROG_MODE_PROG26;

1. gcc-2.95.3/gcc/collect2.c:1172
    添加一个参数

     > redir_handle = open (redir, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU);

1. uClibc/libc/sysdeps/linux/arm/ioperm.c:50
    添加头文件
    
     > \#include "linux/input.h"
     
修复代码错误：

1. gcc-2.95.3/texinfo/makeinfo/makeinfo.c:1686

     from 
 
      >    if (!cr_or_whitespace (string[x]))
            {
               strcpy (string, string + x);
               break;
            }
     
     to
     
      >  if (!cr_or_whitespace (string[x]))
            {
               memmove (string, string + x, len - x + 1);
               break;
            }
