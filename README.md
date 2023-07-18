# WQ_KIWI_USB

# WQ_KIWI_USB

编译器：Visual Studio 2022



打开项目：WQ_KIWI_USB.sln（双击）

执行代码：Ctrl + F5



平台环境：

|    项目     |  配置   | 平台 | 字符集 |
| :---------: | :-----: | :--: | :----: |
| WQ_KIWI_USB | Release | x64  | 未设置 |



- 文件结构

  mfgtool

  --------------------------------------------------------------------------------------------------------------------------
  
  > >  create_dll/	编译出的dll文件存放到当前目录下
  >
  > >  Release/		存放x86运行需要的x86 dll文件和编译生成文件。
  >
  > >  usb driver	安装libusb驱动软件和使用指导（readme.txt）。
  >
  > >  WQ_KIWI_USB/	项目代码
  > >
  > > >  bin_list/	需要下载的fw_cdc_dnld.wpk文件，实际上zip文件压缩改的后缀名，解压后里面是各种bin文件
  > > >
  > > > > fw_cdc_dnld.wpk
  > > > >
  > > > > fw_usb_dnld.wpk
  > > > >
  > > > > > ​	fw_dtop_core0.bin（固定：不要自己编译，就用库里的）
  > > > > >
  > > > > > ​	iomap_config.bin（固定：不要自己编译，就用库里的）
  > > > > >
  > > > > > ​	sbr_cdc_core0.bin（固定：不要自己编译，就用库里的）
  > > > > >
  > > > > > ​	fw_wifi_mp_test_core1.bin（仓库：kiwi_soc， 分支：fpga_s10_dev， target：scons app=patch_fw_bt_1_1_mp_test -j32）
  > > > > >
  > > > > > ​	fw_bt_1_1_mp_test_core3.bin（仓库：kiwi_soc， 分支：rom_1_1_dev， target:scons app=fw_wifi_mp_test -j32
  > >
  > > > dll/
  > > >
  > > > > libusb/	链接的动态库和头文件。
  > > >
  > > > > zlib/		链接的动态库和头文件。
  > >
  > > > src/
  > > >
  > > > > app/	存放main.c文件。
  > > >
  > > > >hal/	存放自己编写的驱动.c和.h。
  > > >
  > > > > lib/	存放链接的静态库。
  >
  > > x64/Release/	存放x64运行需要的x64 dll文件和编译生成文件。
