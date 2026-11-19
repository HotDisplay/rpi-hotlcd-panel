#!/usr/bin/env bash
# Build and install the H070A23 CM4 driver on the target Raspberry Pi.
set -Eeuo pipefail
export PATH="$PATH:/usr/sbin:/sbin"

fail() {
    printf '错误：%s\n' "$*" >&2
    exit 1
}

if (( $# )); then
    if [[ $# == 1 && ( $1 == --help || $1 == -h ) ]]; then
        printf 'H070A23 CM4 安装：bash install.sh\n在 CM4 本机执行；缺少依赖时需要联网安装。安装后请手动重启。\n'
        exit 0
    fi
    fail '不接受安装参数；使用 bash install.sh。'
fi

stage='检查环境'
build_dir=''
backup_dir=''
sudo_cmd=()
transaction_started=0
install_done=0
initramfs_attempted=0
targets=()
backup_names=()
existed=()
staged_files=()

atomic_restore() {
    local temporary="$2.hotlcd-restore.$$"
    staged_files+=("$temporary")
    "${sudo_cmd[@]}" install -D -m 644 "$1" "$temporary" || return
    "${sudo_cmd[@]}" mv -f -- "$temporary" "$2"
}

finish() {
    local result=$? i file restored=1
    trap - EXIT ERR INT TERM
    set +e
    if (( transaction_started && !install_done )); then
        printf '安装未完成，正在恢复原驱动、设备树和启动配置……\n' >&2
        for i in "${!targets[@]}"; do
            if [[ ${existed[i]} == 1 ]]; then
                atomic_restore "$backup_dir/${backup_names[i]}" "${targets[i]}" || restored=0
            else
                "${sudo_cmd[@]}" rm -f -- "${targets[i]}" || restored=0
            fi
        done
        "${sudo_cmd[@]}" depmod -a "$kernel" || restored=0
        if (( initramfs_attempted )); then
            "${sudo_cmd[@]}" update-initramfs -u -k "$kernel" || restored=0
        fi
        if (( restored )); then
            printf '已恢复原文件；请排除错误后重新安装。\n' >&2
        else
            printf '恢复未全部完成，请根据报错处理；备份：%s\n' "$backup_dir" >&2
        fi
    fi
    for file in "${staged_files[@]}"; do
        [[ ! -e $file ]] || "${sudo_cmd[@]}" rm -f -- "$file"
    done
    [[ -z $build_dir ]] || rm -rf -- "$build_dir"
    exit "$result"
}

trap finish EXIT
trap 'printf "错误：%s失败（第 %s 行），安装未完成。\n" "$stage" "$LINENO" >&2; [[ -z "$backup_dir" ]] || printf "已有文件备份：%s\n" "$backup_dir" >&2' ERR
trap 'exit 130' INT
trap 'exit 143' TERM

[[ $(uname -s) == Linux ]] || fail '请将驱动目录复制到 CM4，在 CM4 的终端执行安装。'
[[ -r /proc/device-tree/model ]] || fail '无法读取树莓派型号。'
model=$(tr -d '\0' < /proc/device-tree/model)
[[ $model == 'Raspberry Pi Compute Module 4 '* ]] || fail "本安装入口仅适用于 CM4；当前设备：$model"

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
overlay='vc4-kms-dsi-hotlcd-panel-cm4-h070a23'
overlay_source="$overlay-overlay.dts"
module='panel-hotlcd-dsi'
for file in Makefile "$module.c" "$overlay_source"; do
    [[ -f $script_dir/$file ]] || fail "驱动包缺少 $file，请复制完整驱动目录。"
done

command -v findmnt > /dev/null || fail '缺少 findmnt，请先安装 util-linux。'
boot_dir=''
for candidate in /boot/firmware /boot; do
    if [[ -f $candidate/config.txt && -d $candidate/overlays ]] &&
       [[ $(findmnt -rn -M "$candidate" -o FSTYPE) == vfat ]]; then
        boot_dir=$candidate
        break
    fi
done
[[ -n $boot_dir ]] || fail '未找到已挂载的 FAT 启动分区（/boot/firmware 或 /boot）。'
mount_options=$(findmnt -rn -M "$boot_dir" -o OPTIONS)
[[ ,$mount_options, != *,ro,* ]] || fail "启动分区 $boot_dir 为只读，请先恢复为可写。"
config="$boot_dir/config.txt"
kernel=$(uname -r)
module_dir="/lib/modules/$kernel"
kdir="$module_dir/build"
build_dir=$(mktemp -d /tmp/hotlcd-h070a23.XXXXXX)

stage='检查启动配置'
cp -- "$config" "$build_dir/config.before"
# Keep user settings, migrate the old H070A23 entry, and preserve touch options.
# Unknown includes and conflicting overlays require manual resolution.
awk -v overlay="$overlay" '
function reject(message) {
    print "错误：config.txt 第 " NR " 行：" message > "/dev/stderr"
    failed = 1
    exit 1
}
BEGIN {
    begin = "# BEGIN HOTLCD H070A23 CM4"
    end = "# END HOTLCD H070A23 CM4"
    model_matches = 1
}
{
    raw = $0
    sub(/\r$/, "", raw)
    if (raw == begin) {
        if (managed || block_position) reject("安装配置段重复。")
        block_position = ++total
        lines[total] = ""
        managed = 1
        next
    }
    if (raw == end) {
        if (!managed) reject("安装配置段缺少开始标记。")
        managed = 0
        next
    }
    value = raw
    sub(/#.*/, "", value)
    include_line = (value ~ /^[ \t]*include([ \t]|$)/)
    gsub(/[ \t]/, "", value)
    if (value == "[all]") {
        model_matches = 1
        conditional = 0
    } else if (value == "[pi4]" || value == "[cm4]") {
        model_matches = 1
    } else if (value ~ /^\[(pi[0-9]+|pi0w|pi02|pi3\+|pi400|pi500|pi500\+|cm[0-9]+s?|none)\]$/) {
        model_matches = 0
    } else if (value ~ /^\[.*\]$/) {
        conditional = 1
    }
    if (!model_matches && !managed) {
        lines[++total] = raw
        next
    }
    if (include_line)
        reject("存在 include 引用，请先合并或检查引用文件中的显示配置。")
    if ((value ~ /^overlay_prefix=/ && value != "overlay_prefix=overlays/") ||
        (value ~ /^os_prefix=/ && value != "os_prefix="))
        reject("使用了自定义启动路径，请先恢复默认启动路径。")
    if (value ~ /^dtparam=.*(invx|invy|swapxy|disable_touch)/)
        reject("请将触摸参数写在本模组的 dtoverlay 同一行。")
    consume = 0
    if (value ~ /^dtoverlay=/) {
        count = split(substr(value, 11), args, ",")
        name = args[1]
        if (name == overlay || name == "vc4-kms-dsi-hotlcd-panel") {
            if (conditional) reject("请将本模组配置移到 [all]、[pi4] 或 [cm4] 段。")
            selected = "dtoverlay=" overlay
            legacy_h070a23 = 0
            for (i = 2; i <= count; i++) {
                if (name != overlay && args[i] == "h070a23") {
                    legacy_h070a23 = 1
                    continue
                }
                if (args[i] !~ /^(disable_touch|invx|invy|swapxy)(=.*)?$/)
                    reject("不支持自动迁移参数 " args[i] "，请检查屏幕型号和接口配置。")
                selected = selected "," args[i]
            }
            if (name != overlay && !legacy_h070a23)
                reject("旧 HOTLCD 配置未选择 h070a23，请先确认屏幕型号。")
            if (panel != "" && panel != selected)
                reject("存在不同的 H070A23 触摸参数，请保留一组配置。")
            panel = selected
            consume = 1
        } else if (name == "vc4-kms-v3d") {
            if (conditional) reject("请将 vc4-kms-v3d 配置移到 [all]、[pi4] 或 [cm4] 段。")
            if (kms != "" && kms != value)
                reject("存在不同的 vc4-kms-v3d 参数，请保留一组配置。")
            kms = value
            if (!managed && ++kms_outside > 1)
                reject("存在重复的 vc4-kms-v3d 配置，请保留一组配置。")
            consume = managed
        } else if (name ~ /dsi/ || name ~ /^vc4-fkms/ ||
                   name ~ /^(i2c5|pwm|goodix)(-|$)/) {
            reject("请先停用冲突的 overlay：" name)
        }
    }
    if (managed) {
        if (value != "" && value != "[all]" && !consume &&
            value != "display_auto_detect=0" && value != "dtparam=audio=off")
            reject("安装配置段中有其他设置，请将其移到该段之外。")
        next
    }
    if (consume) raw = "# HOTLCD installer disabled: " raw
    lines[++total] = raw
}
function print_block() {
    print begin
    print "[all]\ndisplay_auto_detect=0\ndtparam=audio=off"
    if (!kms_outside) print (kms == "" ? "dtoverlay=vc4-kms-v3d" : kms)
    print (panel == "" ? "dtoverlay=" overlay : panel)
    print end
}
END {
    if (failed) exit 1
    if (managed) {
        print "错误：config.txt 安装配置段缺少结束标记。" > "/dev/stderr"
        exit 1
    }
    while (total > 0 && lines[total] ~ /^[ \t]*$/ && total != block_position) total--
    for (i = 1; i <= total; i++) {
        if (i == block_position) print_block()
        else print lines[i]
    }
    if (!block_position) {
        if (total) print ""
        print_block()
    }
}' "$build_dir/config.before" > "$build_dir/config.txt"

sudo_cmd=()
if (( EUID != 0 )); then
    command -v sudo > /dev/null || fail '需要管理员权限，请使用 root 执行安装。'
    sudo_cmd=(sudo)
fi

stage='准备编译依赖'
packages=()
for tool in make gcc ld; do
    if ! command -v "$tool" > /dev/null; then
        packages+=(build-essential)
        break
    fi
done
command -v dtc > /dev/null || packages+=(device-tree-compiler)
if ! command -v modinfo > /dev/null || ! command -v depmod > /dev/null; then
    packages+=(kmod)
fi
if (( ${#packages[@]} )) || [[ ! -f $kdir/Makefile ]]; then
    command -v apt-get > /dev/null || fail '请先安装编译工具、dtc、kmod 和匹配当前内核的头文件。'
    printf '正在获取依赖；此步骤需要联网，当前内核：%s\n' "$kernel"
    "${sudo_cmd[@]}" apt-get update || fail '更新软件源失败。离线安装请提前准备依赖和对应内核头文件。'
    if [[ ! -f $kdir/Makefile ]]; then
        if apt-cache show "linux-headers-$kernel" > /dev/null 2>&1; then
            packages+=("linux-headers-$kernel")
        elif apt-cache show raspberrypi-kernel-headers > /dev/null 2>&1; then
            packages+=(raspberrypi-kernel-headers)
        else
            fail "软件源中没有内核 $kernel 的头文件，请安装匹配的头文件后重试。"
        fi
    fi
    "${sudo_cmd[@]}" apt-get install -y --no-install-recommends "${packages[@]}"
fi
for tool in make gcc ld dtc modinfo depmod; do
    command -v "$tool" > /dev/null || fail "缺少 $tool，请检查依赖是否安装成功。"
done
[[ -f $kdir/Makefile ]] || fail "仍缺少内核 $kernel 的头文件。请准备匹配版本，或更新内核并重启后重试。"
if [[ -f /boot/initrd.img-$kernel ]]; then
    command -v update-initramfs > /dev/null || fail '已有 initramfs，但缺少更新工具；请先安装 initramfs-tools。'
fi
header_kernel=$(make -s --no-print-directory -C "$kdir" kernelrelease)
[[ $header_kernel == "$kernel" ]] || fail "头文件版本 $header_kernel 与当前内核 $kernel 不匹配。"

stage='编译驱动和设备树'
printf 'H070A23 CM4：内核 %s，启动分区 %s\n' "$kernel" "$boot_dir"
cp -- "$script_dir/Makefile" "$script_dir/$module.c" "$build_dir/"
make -C "$kdir" M="$build_dir" -j"$(nproc)" modules
dtc -@ -I dts -O dtb -o "$build_dir/$overlay.dtbo" "$script_dir/$overlay_source"
[[ -s $build_dir/$module.ko && -s $build_dir/$overlay.dtbo ]] || fail '未生成有效的驱动或设备树文件。'
vermagic=$(modinfo -F vermagic "$build_dir/$module.ko")
[[ ${vermagic%% *} == "$kernel" ]] || fail '编译产物的内核版本不匹配，停止安装。'

stage='备份和安装'
backup_dir="$boot_dir/hotlcd-backups/$(date +%Y%m%d-%H%M%S)-$$"
module_path="$module_dir/updates/$module.ko"
legacy_path="$module_dir/$module.ko"
dtbo_path="$boot_dir/overlays/$overlay.dtbo"
targets=("$config" "$module_path" "$legacy_path" "$dtbo_path")
backup_names=(config.txt "$module.ko" "$module-legacy.ko" "$overlay.dtbo")
"${sudo_cmd[@]}" install -d "$backup_dir"
for i in "${!targets[@]}"; do
    if [[ -f ${targets[i]} ]]; then
        existed+=(1)
        "${sudo_cmd[@]}" cp -- "${targets[i]}" "$backup_dir/${backup_names[i]}"
    else
        existed+=(0)
    fi
done

# Stage every replacement in its destination filesystem before changing files.
staged_files=("$module_path.hotlcd.$$" "$dtbo_path.hotlcd.$$" "$config.hotlcd.$$")
sources=("$build_dir/$module.ko" "$build_dir/$overlay.dtbo" "$build_dir/config.txt")
for i in "${!sources[@]}"; do
    "${sudo_cmd[@]}" install -D -m 644 "${sources[i]}" "${staged_files[i]}"
done
cmp -s "$config" "$build_dir/config.before" || fail '启动配置在编译期间发生变化，未替换任何文件，请重新运行安装。'

transaction_started=1
"${sudo_cmd[@]}" mv -f -- "${staged_files[0]}" "$module_path"
if [[ -f $legacy_path ]]; then
    "${sudo_cmd[@]}" rm -- "$legacy_path"
fi
"${sudo_cmd[@]}" depmod -a "$kernel"
loaded_path=$(modinfo -k "$kernel" -n "$module")
[[ $(readlink -f -- "$loaded_path") == "$(readlink -f -- "$module_path")" ]] ||
    fail "模块索引仍指向 $loaded_path，请检查同名旧模块。"
"${sudo_cmd[@]}" mv -f -- "${staged_files[1]}" "$dtbo_path"
if [[ -f /boot/initrd.img-$kernel ]]; then
    stage='更新 initramfs'
    initramfs_attempted=1
    "${sudo_cmd[@]}" update-initramfs -u -k "$kernel"
fi
stage='更新启动配置'
if ! cmp -s "$config" "$build_dir/config.txt"; then
    "${sudo_cmd[@]}" mv -f -- "${staged_files[2]}" "$config"
fi
install_done=1
printf '\n安装完成。\n配置文件：%s\n已有文件备份：%s\n请执行 sudo reboot，重启后使用新驱动。\n' "$config" "$backup_dir"
