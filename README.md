driver build:
	make
	./install_drv.sh

device build:
	./install_dt.sh

config.txt config load:
	example:
		dtoverlay=vc4-kms-dsi-hotlcd-panel,h080a11
	all:
		h042a01,
		h043a8,
		h070a23,
		h080a11,
		
