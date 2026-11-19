# HOTLCD 树莓派 4/5 DSI 显示模组驱动

适用于 Raspberry Pi 4/5 与 HOTLCD 配套 DSI 显示模组，支持显示、触摸和背光控制。请在断电状态下按配套接线说明连接屏幕。

**H070A23 CM4 用户请使用 [CM4 使用说明](README-H070A23-RPICM4.md)，执行 `bash install.sh` 安装。** 以下步骤用于树莓派 4/5。

## 1. 准备

将配套驱动目录复制到树莓派用户目录，进入该目录。驱动文件必须与树莓派平台及显示模组匹配；树莓派 4 需使用配套的 2 Lane 驱动版本。

当前源码的 H070A23 为 4 Lane 配置，树莓派 4 用户需另用配套的 2 Lane 版本，不能只更换设备树。

安装编译工具及当前运行内核的头文件：

```bash
sudo apt update
sudo apt install build-essential device-tree-compiler "linux-headers-$(uname -r)"
```

离线使用时，请提前安装上述依赖。

## 2. 编译与安装

按平台选择设备树，以下两条命令只执行对应的一条：

```bash
# 树莓派 4
cp vc4-kms-dsi-hotlcd-panel-pi4-overlay.dts vc4-kms-dsi-hotlcd-panel-overlay.dts
```

```bash
# 树莓派 5
cp vc4-kms-dsi-hotlcd-panel-pi5-overlay.dts vc4-kms-dsi-hotlcd-panel-overlay.dts
```

然后执行：

```bash
BOOT_DIR=/boot
if mountpoint -q /boot/firmware; then BOOT_DIR=/boot/firmware; fi

make clean &&
make &&
dtc -@ -I dts -O dtb \
    -o vc4-kms-dsi-hotlcd-panel.dtbo \
    vc4-kms-dsi-hotlcd-panel-overlay.dts &&
sudo install -m 644 panel-hotlcd-dsi.ko "/lib/modules/$(uname -r)/panel-hotlcd-dsi.ko" &&
sudo depmod -a "$(uname -r)" &&
sudo install -m 644 vc4-kms-dsi-hotlcd-panel.dtbo "$BOOT_DIR/overlays/"
```

任一步报错时请停止后续操作，并保留完整报错。编译安装成功后，若系统已有当前内核的启动内存盘，再执行：

```bash
if [ -f "/boot/initrd.img-$(uname -r)" ]; then
    sudo update-initramfs -u -k "$(uname -r)"
fi
```

## 3. 启动配置

编辑启动分区的 `config.txt`：新版系统通常为 `/boot/firmware/config.txt`，旧版为 `/boot/config.txt`。

在生效的 `[all]` 段配置以下内容。示例型号为 `h080a11`，请按实际模组替换：

```ini
[all]
display_auto_detect=0
dtoverlay=vc4-kms-v3d
dtoverlay=vc4-kms-dsi-hotlcd-panel,h080a11
```

已有配置请修改原项，避免重复；停用其他 DSI 屏幕配置。

| 显示模组 | 型号参数 |
|---|---|
| H042A01 | `h042a01` |
| H043A8 | `h043a8` |
| H070A23 | `h070a23` |
| H080A11 | `h080a11` |

以下参数可追加到同一条 `dtoverlay` 行，使用逗号分隔：

| 参数 | 作用 |
|---|---|
| `disable_touch` | 禁用触摸 |
| `invx` / `invy` | 反向触摸 X / Y 轴 |
| `swapxy` | 交换触摸 X/Y 轴 |
| `dsi0` | 选择 DSI0，默认使用 DSI1 |

例如，交换触摸 X/Y 轴并反向 Y 轴：

```ini
dtoverlay=vc4-kms-dsi-hotlcd-panel,h080a11,swapxy,invy
```

安装与配置完成后重启：

```bash
sudo reboot
```

## 4. 背光调节

树莓派 5 支持 PWM 调光，范围 0～100，0 关闭背光。可在系统屏幕设置中右键 DSI 显示器，选择 Brightness（亮度）；也可执行：

```bash
echo 30 | sudo tee /sys/class/backlight/backlight_pwm/brightness
```

树莓派 4 配套版本支持背光开关，0 关闭、1 开启：

```bash
echo 1 | sudo tee /sys/class/backlight/backlight_gpio/brightness
```

## 5. 更新与问题反馈

更新源码后重新执行第 2 节并重启。升级系统内核后，请先启动新内核，安装匹配的头文件，再重新编译安装驱动。

反馈问题时，请提供树莓派型号、显示模组型号、`config.txt` 中的显示配置，以及以下输出；安装失败时附完整报错：

```bash
uname -r
sudo dmesg | grep -iE 'hotlcd|dsi|panel|goodix|pwm'
```

技术支持：lcd@hotlcd.com
