# HOTLCD 树莓派4/5 DSI 显示屏驱动

本驱动用于在 **树莓派4/5** 上点亮 HOTLCD（鑫洪泰）MIPI DSI 显示屏及配套转接板，支持电容触摸（Goodix GT9271）与背光控制。

- 驱动文件：`panel-hotlcd-dsi.c`（基于 Broadcom `panel-raspberrypi-touchscreen` 修改）
- 设备树：提供树莓派4/5两个版本（`vc4-kms-dsi-hotlcd-panel-pi4-overlay.dts` / `vc4-kms-dsi-hotlcd-panel-pi5-overlay.dts`），编译前按版本重命名为 `vc4-kms-dsi-hotlcd-panel-overlay.dts`
- 适用平台：Raspberry Pi 4 / 5（Bookworm 及以上系统）
- 树莓派5：DSI 排线 22pin，支持 DSI 4 Lane，背光 PWM 可调光
- 树莓派4：DSI 排线 15pin，仅支持 MIPI 2 通道，使用时需将驱动中显示屏通道数参数改为 2 Lane 并重新编译；背光为 GPIO 开关控制（不可调亮度）。若出现花屏扭曲/DSI LP 报错，需降低驱动中对应屏的 clock（见 FAQ Q7）

---

## 一、支持的屏幕型号

请根据您购买的屏幕，在后续配置中选择对应型号参数。

| 型号参数 | 尺寸 | 分辨率 | 显示方向 | DSI Lane | 像素格式 | 默认背光 |
|----------|------|--------|----------|----------|----------|----------|
| `h042a01` | 4.2" | 1280×960 | 横屏 | 4 | RGB888 | 80 |
| `h043a8`  | 4.3" | 480×800  | 竖屏 | 2 | RGB888 | 80 |
| `h070a23` | 7.0" | 1024×600 | 横屏 | 2 | RGB888 | 80 |
| `h080a11` | 8.0" | 800×1280 | 竖屏 | 4 | RGB888 | 80 |

> ⚠️ **务必根据自己的屏幕型号选择参数**，设备树默认按 `h080a11`（8寸）配置，不指定型号可能导致点屏失败。
>
> ℹ️ `h070a23` 的 DSI Lane 数可在驱动寄存器中配置（默认 2 Lane），如需改为其他 Lane 数需修改源码并重新编译驱动（见安装步骤）。

---

## 二、硬件连接

通过 HOTLCD 配套转接板将屏幕接到树莓派4/5，共两根线：

1. **DSI 排线**：转接板同时提供 22pin（树莓派5）与 15pin（树莓派4）两种 DSI 排线接口，按所用树莓派版本选择对应接口连接；传输 DSI 显示信号，同时承载触摸 I2C（i2c_csi_dsi，Goodix GT9271 @ 0x5d）；
2. **8pin 供电/控制线**：一头插转接板，另一头插树莓派4/5 GPIO 排针，用于 5V 供电以及显示屏复位、背光控制、触摸中断/复位等信号（不含 I2C）。

8pin 排线对应的 GPIO 分配如下：

| 功能 | 引脚 | 说明 |
|------|------|------|
| 5V 供电 | 5V pin | 转接板与屏供电 |
| 屏幕复位 | GPIO23 | 高电平复位 |
| 触摸中断 | GPIO18 | 下降沿触发 |
| 触摸复位 | GPIO15 | |
| 背光控制 | GPIO14 | 树莓派5=PWM0可调光；树莓派4=GPIO开关不可调光 |

> 触摸 I2C 已由 DSI 排线引出，8pin 线上不含 I2C 信号，无需额外接 I2C。
> 接线时按转接板丝印对应插入，8pin 线插反可能导致供电异常，请确认方向后再上电。

---

## 三、安装步骤

以下操作均在树莓派4/5本机上执行，需联网。官方树莓派镜像默认已包含编译所需依赖，可直接开始。

### 1. 拉取驱动源码

在树莓派4/5上执行：

```bash
cd ~
git clone https://github.com/Nil-lxl/rpi-hotlcd-panel.git
cd rpi-hotlcd-panel
```

> 若暂无 git，也可将驱动目录手动拷贝到树莓派4/5，例如 `~/hotlcd-panel/`。

### 2. 编译并安装驱动与设备树

先按所用树莓派版本，把对应的设备树源文件重命名为编译用的固定文件名 `vc4-kms-dsi-hotlcd-panel-overlay.dts`：

```bash
# 树莓派5（PWM 调光）
cp vc4-kms-dsi-hotlcd-panel-pi5-overlay.dts vc4-kms-dsi-hotlcd-panel-overlay.dts

# 树莓派4（GPIO 背光开关，不可调光）
cp vc4-kms-dsi-hotlcd-panel-pi4-overlay.dts vc4-kms-dsi-hotlcd-panel-overlay.dts
```

> 根据您的树莓派版本，二选一执行上面的 `cp` 命令。

然后编译安装：

```bash
make
sudo ./install_drv.sh
sudo ./install_dt.sh
```

> 若 `sudo ./install_drv.sh` 执行报 `Permission denied` 或无反应，改用 `bash` 显式调用：
> ```bash
> make
> sudo bash install_drv.sh
> sudo bash install_dt.sh
> ```

- `install_drv.sh`：把 `panel-hotlcd-dsi.ko` 复制到 `/lib/modules/$(uname -r)/`，执行 `depmod` 并 `modprobe` 加载；
- `install_dt.sh`：用 `dtc` 把 `vc4-kms-dsi-hotlcd-panel-overlay.dts` 编译成 `vc4-kms-dsi-hotlcd-panel.dtbo`，复制到 `/boot/overlays/`。

### 3. 配置 /boot/firmware/config.txt

编辑配置文件（**Bookworm 系统为 `/boot/firmware/config.txt`，旧系统为 `/boot/config.txt`**）：

```bash
sudo nano /boot/firmware/config.txt
```

在 `[all]` 段或文件末尾添加一行，**根据您的屏幕型号替换 `<型号>`**：

```ini
dtoverlay=vc4-kms-dsi-hotlcd-panel,h080a11
```

例如 7 寸屏：

```ini
dtoverlay=vc4-kms-dsi-hotlcd-panel,h070a23
```

> 同时建议屏蔽可能冲突的默认显示配置，确保 `[all]` 下没有其他 `dtoverlay=vc4-kms-v3d` 之外的 DSI/DSI 屏配置。

### 4. 重启

```bash
sudo reboot
```

重启后屏幕应正常显示桌面。若黑屏或异常，先看 [常见问题](#六常见问题)。

---

## 四、config.txt 参数说明

在 `dtoverlay=vc4-kms-dsi-hotlcd-panel` 后以逗号分隔追加参数：

| 参数 | 取值 | 说明 |
|------|------|------|
| `h042a01` / `h043a8` / `h070a23` / `h080a11` | — | **选择屏幕型号**（必选其一，默认 h080a11） |
| `disable_touch` | — | 禁用触摸 |
| `invx` | — | 触摸 X 轴反向 |
| `invy` | — | 触摸 Y 轴反向 |
| `swapxy` | — | 交换触摸 X/Y 轴 |
| `dsi0` | — | 使用 DSI0 接口（默认 DSI1） |

> 背光亮度调节参数尚在完善中，暂不通过 config.txt 配置。

**示例：**

```ini
# 8寸屏
dtoverlay=vc4-kms-dsi-hotlcd-panel,h080a11

# 7寸屏，触摸 X/Y 互换并 Y 反向
dtoverlay=vc4-kms-dsi-hotlcd-panel,h070a23,swapxy,invy

# 仅用屏幕，不要触摸
dtoverlay=vc4-kms-dsi-hotlcd-panel,h043a8,disable_touch
```

---

## 五、内核升级后的处理

⚠️ **树莓派内核每次升级后，`.ko` 模块需要重新编译安装**，否则重启后驱动失效、屏幕黑屏。

升级后请重新执行：

```bash
cd ~/hotlcd-panel
make clean
make
sudo ./install_drv.sh
sudo reboot
```

> 若 `sudo ./install_drv.sh` 报错，改用 `sudo bash install_drv.sh`（见安装步骤说明）。

设备树 `dtbo` 与内核版本无关，无需重新编译（除非本驱动有更新）。

---

## 六、常见问题

**Q1：重启后屏幕黑屏？**
1. 确认 `config.txt` 里型号参数与实际屏幕一致；
2. 确认排线插到位、方向正确；
3. SSH 登录后检查：`dmesg | grep -iE "hotlcd|dsi|panel"`，看是否有 probe 失败信息；
4. 确认驱动已加载：`lsmod | grep panel_hotlcd`，若无则重新执行 `install_drv.sh`。

**Q2：屏幕亮了但触摸没反应/方向不对？**
- 触摸不工作：检查 `dmesg | grep -i goodix`，确认 I2C 识别到 GT9271；
- 触摸方向不对：用 `invx`、`invy`、`swapxy` 参数组合调整。

**Q3：背光如何调节？**
- 树莓派5（PWM 调光）：运行时 `echo 80 | sudo tee /sys/class/backlight/backlight_pwm/brightness`（取值 0–100）；
- 树莓派4（GPIO 开关）：背光仅开/关，不可调亮度，`echo 1 | sudo tee /sys/class/backlight/backlight_gpio/brightness` 点亮、`echo 0` 熄灭。

**Q4：`make` 报错找不到内核头文件？**
```bash
sudo apt install -y raspberrypi-kernel-headers
# 若仍报错，确认当前内核：
uname -r
# 重新安装匹配版本头文件后重试
```

**Q5：`install_dt.sh` 报错 `dtc: command not found`？**
```bash
sudo apt install -y device-tree-compiler
```

**Q6：开机画面正常，进桌面后花屏/撕裂？**
- 检查电源供电是否充足（树莓派5建议 5V/5A 官方电源）；
- 确认未启用与其他 DSI/KMS 配置冲突的 overlay。

**Q7：树莓派4上屏幕花屏扭曲、内核报 DSI LP 错误？**
树莓派4的 DSI 信号时序裕量较小，部分屏在默认像素时钟下可能出现花屏扭曲或内核日志报 DSI LP 错误。解决方法：降低驱动中对应屏的像素时钟（clock）约 30%。

编辑 `panel-hotlcd-dsi.c`，找到您所用屏幕对应的 `ht_panel_h<型号>_mode` 结构体，将 `.clock` 值降低约 30%。以 8 寸屏为例：

```c
static const struct drm_display_mode ht_panel_h080a11_mode = {
    .clock = 69 * 1000,   // 原值；降低约30%改为 48 * 1000
    ...
};
```

改完重新编译安装驱动：

```bash
make clean
make
sudo ./install_drv.sh
sudo reboot
```

> `.clock` 单位为 kHz（`69 * 1000` 即 69 MHz），降低 30% 约为原值的 0.7 倍。若 30% 仍异常可继续微调至画面稳定。

---

## 七、文件说明

| 文件 | 作用 |
|------|------|
| `panel-hotlcd-dsi.c` | DSI 屏驱动源码（含 4 款屏的初始化序列与显示参数） |
| `vc4-kms-dsi-hotlcd-panel-pi5-overlay.dts` | 设备树 overlay 源码（树莓派5版，PWM 背光调光） |
| `vc4-kms-dsi-hotlcd-panel-pi4-overlay.dts` | 设备树 overlay 源码（树莓派4版，GPIO 背光开关） |
| `Makefile` | 驱动编译规则 |
| `install_drv.sh` | 安装驱动 `.ko` 到内核模块目录并加载 |
| `install_dt.sh` | 编译设备树并安装到 `/boot/overlays/` |

---

## 八、技术支持

- 厂商：HOTDISPLAY Technology Co., Ltd（鑫洪泰）
- 邮箱：lcd@hotlcd.com
- 驱动版本：基于 kernel `panel-raspberrypi-touchscreen` 适配
- 适用：Raspberry Pi 4 / 5 + Raspbian/Bookworm

如点屏遇到问题，请提供以下信息以便快速定位：
1. 屏幕型号（如 h080a11）；
2. `uname -a` 与 `cat /etc/os-release` 输出；
3. `dmesg | grep -iE "hotlcd|dsi|panel|goodix"` 完整输出；
4. `config.txt` 中与本驱动相关的配置行。
