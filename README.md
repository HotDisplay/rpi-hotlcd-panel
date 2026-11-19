# HOTLCD 树莓派 5 DSI 显示模组驱动

源码分支：[main](https://github.com/HotDisplay/rpi-hotlcd-panel/tree/main)。

适用于 Raspberry Pi 5 与 HOTLCD 配套 DSI 显示模组，支持显示、触摸和 PWM 背光。请在断电状态下按配套接线说明连接模组。

## 1. 安装与更新

将完整驱动目录复制到树莓派 5 用户目录，进入目录后按显示模组型号安装。例如 H070A23：

```bash
bash install.sh h070a23
```

| 显示模组 | 安装参数 |
|---|---|
| H042A01 | `h042a01` |
| H043A8 | `h043a8` |
| H070A23 | `h070a23` |
| H080A11 | `h080a11` |

按提示输入管理员密码。脚本自动编译并安装驱动和设备树。缺少依赖时需要联网；离线使用时，请提前安装 `build-essential`、`device-tree-compiler`、`kmod` 和匹配当前运行内核的头文件。

看到“安装完成”后重启：

```bash
sudo reboot
```

更新源码时重新执行安装命令并重启。升级系统内核后，请先启动新内核，再重新安装驱动。安装前文件备份在启动分区的 `hotlcd-backups/` 目录。

## 2. 设备树配置

安装脚本会自动配置启动分区的 `config.txt`：新版系统通常为 `/boot/firmware/config.txt`，旧版为 `/boot/config.txt`。

以下以 H070A23 为例，仅供核对，无需重复添加：

```ini
[all]
display_auto_detect=0
dtoverlay=vc4-kms-v3d
dtoverlay=vc4-kms-dsi-hotlcd-panel,h070a23
```

默认使用 DSI1。H070A23 使用 4 Lane、51MHz 配置。

以下参数可追加到本模组现有的 `dtoverlay` 行，使用逗号分隔：

| 参数 | 作用 |
|---|---|
| `disable_touch` | 禁用触摸 |
| `invx` / `invy` | 反向触摸 X / Y 轴 |
| `swapxy` | 交换触摸 X/Y 轴 |
| `dsi0` | 改用 DSI0 接口 |
| `brightness=80` | 默认背光亮度，范围 0～100 |

例如，交换触摸 X/Y 轴并反向 Y 轴：

```ini
dtoverlay=vc4-kms-dsi-hotlcd-panel,h070a23,swapxy,invy
```

触摸方向参数需在触摸正常识别后使用；修改配置后重启生效。若脚本提示配置冲突，请按报错检查并处理。

## 3. 亮度调节

桌面入口：**控制中心 → Screens（屏幕）→ 右键所用 DSI 显示器 → Brightness（亮度）**。

也可在终端设置。亮度范围 0～100，默认 80；0 关闭背光：

```bash
echo 30 | sudo tee /sys/class/backlight/backlight_pwm/brightness
```

## 4. 问题反馈

请提供显示模组型号、设备树配置行及以下输出；安装失败时附完整报错：

```bash
uname -r
sudo dmesg | grep -iE 'hotlcd|dsi|panel|goodix|pwm'
```

技术支持：lcd@hotlcd.com
