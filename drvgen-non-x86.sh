#!/bin/sh
qemu-i386 tools/dct/DrvGen drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/codegen.dws out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc gpio_boot_h

qemu-i386 tools/dct/DrvGen drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/codegen.dws out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc gpio_usage_h

qemu-i386 tools/dct/DrvGen drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/codegen.dws out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc kpd_h

qemu-i386 tools/dct/DrvGen drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/codegen.dws out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc eint_h

qemu-i386 tools/dct/DrvGen drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/codegen.dws out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc adc_h

qemu-i386 tools/dct/DrvGen drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/codegen.dws out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc pmic_h

qemu-i386 tools/dct/DrvGen drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/codegen.dws out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc pmic_c

qemu-i386 tools/dct/DrvGen drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/codegen.dws out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc md1_eint_h

qemu-i386 tools/dct/DrvGen drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/codegen.dws out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc eint_dtsi

qemu-i386 tools/dct/DrvGen drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/codegen.dws out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc clk_buf_h

qemu-i386 tools/dct/DrvGen drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/codegen.dws out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc i2c_h

qemu-i386 tools/dct/DrvGen drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/codegen.dws out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/inc i2c_dtsi

cp -r out/drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/* drivers/misc/mediatek/mach/mt6580/aw733/dct/dct/
