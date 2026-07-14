#!/bin/sh

sudo cp ./panel-hotlcd-dsi.ko /lib/modules/$(uname -r)
sudo depmod
sudo modprobe panel-hotlcd-dsi
