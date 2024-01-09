sudo insmod devTranslator.ko
sudo mknod /dev/devTranslator c 415 0
sudo chmod 666 /dev/devTranslator