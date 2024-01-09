# simple_device_driver
The code is a Linux kernel module named "devTranslator.c" that functions as a character device driver with translation capabilities, including Pig Latin and Caesar Cipher encryption/decryption.

## How to build:
- In the Module directories run make to create device driver files. 
- Then run the ./installIt.sh script to install the kernel module that was just created.
- Now in the Test directory run make run to start the test program.
- To uninstall the kernel module run the ./removeIt.sh script.
