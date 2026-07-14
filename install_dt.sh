#!/bin/sh

sudo dtc -I dts -O dtb -o vc4-kms-dsi-hotlcd-panel.dtbo vc4-kms-dsi-hotlcd-panel-overlay.dts
sudo cp vc4-kms-dsi-hotlcd-panel.dtbo /boot/overlays/

