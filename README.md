# H070A23 CM4 显示模组使用说明

版本：`v1.0.0`。源码分支：[h070a23-rpicm4](https://github.com/HotDisplay/rpi-hotlcd-panel/tree/h070a23-rpicm4)。

适用于配套 H070A23 CM4 显示模组和 Raspberry Pi OS，使用 DSI1、4 Lane、51MHz 配置。以下命令在 CM4 终端中执行。

## 1. 安装与更新

将完整驱动目录复制到 CM4 用户目录，进入该目录后执行：

```bash
bash install.sh
```

按提示输入管理员密码。缺少依赖时需要联网；离线使用时，请提前安装 `build-essential`、`device-tree-compiler`、`kmod` 和与当前运行内核匹配的头文件。

看到“安装完成”后重启：

```bash
sudo reboot
```

更新驱动时重新执行上述步骤。升级系统内核后，请先启动新内核，再执行安装脚本并重启。安装前文件备份在启动分区的 `hotlcd-backups/` 目录。

## 2. 设备树配置

安装脚本会自动配置启动分区的 `config.txt`：新版系统通常为 `/boot/firmware/config.txt`，旧版为 `/boot/config.txt`。

以下配置由脚本写入，仅供核对，无需重复添加：

```ini
[all]
display_auto_detect=0
dtparam=audio=off
dtoverlay=vc4-kms-v3d
dtoverlay=vc4-kms-dsi-hotlcd-panel-cm4-h070a23
```

默认使用 H070A23、DSI1，包含背光与配套触摸配置。若脚本提示配置冲突，请按报错停用冲突的 overlay 后重试。

以下参数可追加到本模组现有的 `dtoverlay` 行，使用逗号分隔：

| 参数 | 作用 |
|---|---|
| `disable_touch` | 禁用触摸 |
| `invx` / `invy` | 反向触摸 X / Y 轴 |
| `swapxy` | 交换触摸 X/Y 轴 |

例如，交换触摸 X/Y 轴并反向 Y 轴：

```ini
dtoverlay=vc4-kms-dsi-hotlcd-panel-cm4-h070a23,swapxy,invy
```

触摸方向参数需在触摸正常识别后使用；修改配置后重启生效。

## 3. 亮度调节

桌面入口：**控制中心 → Screens（屏幕）→ 右键 DSI-1 显示器方框 → Brightness（亮度）**。

也可在终端设置。亮度范围 0～100，默认 80；0 关闭背光：

```bash
echo 30 | sudo tee /sys/class/backlight/backlight_pwm/brightness
```

## 4. 问题反馈

请提供本模组的设备树配置行及以下输出；安装失败时附完整报错：

```bash
uname -r
sudo dmesg | grep -iE 'hotlcd|dsi|panel|goodix|pwm'
```
