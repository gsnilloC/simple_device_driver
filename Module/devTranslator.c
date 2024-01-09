/**************************************************************
 * Class:  CSC-415-01 Fall 2023
 * Name: Collins Gichohi
 * Student ID: 922440815
 * GitHub UserID: gsnilloC
 * Project: Assignment 6 – Device Driver
 *
 * File: devTranslator.c
 * This Linux kernel module, "devTranslator.c," 
 * functions as a character device driver with translation 
 * capabilities, including Pig Latin and Caesar Cipher 
 * encryption/decryption. The module uses a structure named 
 * myds to store translation data and implements key functions 
 * such as myWrite for translation on write operations and 
 * myRead for reading the translated text. 
 **************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/random.h>

// Define constants for device registration
#define MY_MAJOR 415
#define MY_MINOR 0
#define DEVICE_NAME "devTranslator"
#define CUSTOM_IOCTL_COMMAND 3

// Define a structure to hold translator data
struct myds
{
    int flag;          // Counter for the number of translations
    char *translation; // Buffer to store translated text
};

// Global variables
int major, minor;
char *kernel_buffer;

// Define a character device structure
struct cdev my_cdev;
int actual_rx_size = 0;

// Module information
MODULE_AUTHOR("Collins Gichohi");
MODULE_DESCRIPTION("A pig latin translator");
MODULE_LICENSE("GPL");

// Function prototypes
static int myOpen(struct inode *inode, struct file *fs);
static int myClose(struct inode *inode, struct file *fs);
static ssize_t myWrite(struct file *fs, const char __user *buf, size_t size, loff_t *off);
static ssize_t myRead(struct file *fs, char __user *buf, size_t size, loff_t *off);
static long myIoCTL(struct file *fs, unsigned int command, unsigned long data);
int pigLatin(const char *input, char *output, size_t output_size);
int caesarCipher(const char *input, char *output, size_t output_size);
int decodeCipher(const char *input, char *output, size_t output_size);
int isAlpha(char c);
int isVowel(char c);
int isLower(char c);
char toLower(char c);

// File operations structure
struct file_operations fops = {
    .open = myOpen,
    .release = myClose,
    .write = myWrite,
    .read = myRead,
    .unlocked_ioctl = myIoCTL,
};

int init_module(void)
{
    int result;
    dev_t devno;

    // Create device number using major and minor numbers
    devno = MKDEV(MY_MAJOR, MY_MINOR);

    // Register character device region
    result = register_chrdev_region(devno, 1, DEVICE_NAME);
    printk(KERN_INFO "Register chardev succeeded 1: %d\n", result);

    // Initialize the character device structure with file operations
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    // Add the character device to the system
    result = cdev_add(&my_cdev, devno, 1);
    printk(KERN_INFO "Register chardev succeeded 2: %d\n", result);
    printk(KERN_INFO "Welcome to my Translator Device\n");

    // Check for registration failure
    if (result < 0)
    {
        printk(KERN_ERR "Register chardev failed: %d\n", result);
        return result;
    }

    return 0;
}

void cleanup_module(void)
{
    dev_t devno;

    // Get the device number
    devno = MKDEV(MY_MAJOR, MY_MINOR);

    // Unregister the character device region
    unregister_chrdev_region(devno, 1);

    // Delete the character device from the system
    cdev_del(&my_cdev);

    printk(KERN_INFO "Goodbye from Pig Latin Translator!\n");
}

// write to the character device
static ssize_t myWrite(struct file *fs, const char __user *buf, size_t size, loff_t *off)
{
    struct myds *ds;
    char *translated_buffer = vmalloc(size * 2);

    // Check if vmalloc for translated buffer failed
    if (translated_buffer == NULL)
    {
        printk(KERN_ERR "vmalloc for translated buffer failed!\n");
        return -1;
    }

    // Get translator data structure from private_data
    ds = (struct myds *)fs->private_data;

    // Free existing translation buffer if it exists
    if (ds->translation != NULL)
    {
        vfree(ds->translation);
    }

    // Allocate memory for the translation buffer
    ds->translation = vmalloc(size * 2);
    if (ds->translation == NULL)
    {
        printk(KERN_ERR "vmalloc in myWrite failed!\n");
    }

    // Copy user data to the translation buffer
    if (copy_from_user(ds->translation, buf, size))
    {
        vfree(ds->translation);
        ds->translation = NULL;
        return -1;
    }

    if (ds->flag == 1)
    {
        // Perform translation
        if (pigLatin(ds->translation, translated_buffer, size * 2) != 0)
        {
            vfree(translated_buffer);
            return -1;
        }
    }
    else if (ds->flag == 2)
    {
        // Perform translation
        if (caesarCipher(ds->translation, translated_buffer, size * 2) != 0)
        {
            vfree(translated_buffer);
            return -1;
        }
    }
    else if (ds->flag == 3)
    {
        // Perform translation
        if (decodeCipher(ds->translation, translated_buffer, size * 2) != 0)
        {
            vfree(translated_buffer);
            return -1;
        }
    }
    else
    {
        return -1;
    }

    // Copy translated buffer back to the translation buffer
    strcpy(ds->translation, translated_buffer);

    // Free translated buffer memory
    vfree(translated_buffer);

    // Return the size of the data written
    return size;
}

// Read data from the character device
static ssize_t myRead(struct file *fs, char __user *buf, size_t size, loff_t *off)
{
    struct myds *ds;
    size_t bytesToCopy;

    // Get translator data structure from private_data
    ds = (struct myds *)fs->private_data;

    // If the translation buffer is empty, nothing to read
    if (ds->translation == NULL)
    {
        return 0;
    }

    // Calculate the number of bytes to copy
    bytesToCopy = strlen(ds->translation);

    // Copy translated text to user space
    if (copy_to_user(buf, ds->translation, bytesToCopy))
    {
        printk(KERN_INFO "copy to user failed!\n");
        return -1; // Use -EFAULT for copy_to_user failure
    }

    // Return the number of bytes copied
    return bytesToCopy;
}

// Open the character device
static int myOpen(struct inode *inode, struct file *fs)
{
    struct myds *ds;

    // Allocate memory for the translator data structure
    ds = vmalloc(sizeof(struct myds));

    // Check if vmalloc failed
    if (ds == NULL)
    {
        printk(KERN_ERR "vmalloc Failed!\n");
        return -1;
    }

    // Initialize count and set it as private_data
    ds->flag = -1;
    fs->private_data = ds;

    // Return success
    return 0;
}

// Close the character device
static int myClose(struct inode *inode, struct file *fs)
{
    struct myds *ds;

    // Get translator data structure from private_data
    ds = (struct myds *)fs->private_data;

    // Set translation buffer to NULL and free memory
    ds->translation = NULL;
    vfree(ds);

    // Return success
    return 0;
}

// IOCTL function to handle custom commands
static long myIoCTL(struct file *fs, unsigned int command, unsigned long data)
{
    struct myds *ds;

    // Get translator data structure from private_data
    ds = (struct myds *)fs->private_data;

    // Check if the command is valid
    if (command == CUSTOM_IOCTL_COMMAND)
    {
        // Access data from user space
        if (copy_from_user(&ds->flag, (int __user *)data, sizeof(int)))
        {
            return -1; // handle copy_from_user failure
        }
    }

    // Return success
    return 0;
}

// Translate English to Pig Latin
int pigLatin(const char *input, char *output, size_t output_size)
{
    size_t i = 0;
    size_t j = 0;

    // Check if input, output, or output_size is invalid
    if (input == NULL || *input == '\0' || output == NULL)
        return -1;

    // Iterate through the input string
    while (input[i] && j < output_size - 1)
    {
        // Skip non-alphabetic characters
        while (input[i] && !isAlpha(input[i]))
        {
            // Convert non-alphabetic character to lowercase
            output[j++] = toLower(input[i++]);
        }

        // Check if the current word starts with a vowel
        if (isVowel(input[i]))
        {
            // Append "yay" to the word
            while (input[i] && isAlpha(input[i]))
            {
                // Convert alphabetic characters to lowercase
                output[j++] = toLower(input[i++]);
            }

            output[j++] = 'y';
            output[j++] = 'a';
            output[j++] = 'y';
        }
        else
        {
            char consonant = input[i++];

            // Move the initial consonants to the end and append "ay"
            while (input[i] && isAlpha(input[i]))
            {
                output[j++] = toLower(input[i++]);
            }

            // Convert the initial consonant to lowercase
            output[j++] = toLower(consonant);
            output[j++] = 'a';
            output[j++] = 'y';
        }
    }

    // Null-terminate the output string
    output[j] = '\0';

    // Return success
    return 0;
}

// Encode Caesar Cipher
int caesarCipher(const char *input, char *output, size_t output_size)
{
    size_t i = 0;
    size_t j = 0;

    int shift = 3;

    // Check if input, output, or output_size is invalid
    if (input == NULL || *input == '\0' || output == NULL)
        return -1;

    // Iterate through the input string
    while (input[i] && j < output_size - 1)
    {
        // Skip non-alphabetic characters
        while (input[i] && !isAlpha(input[i]))
        {
            // Copy non-alphabetic character as is
            output[j++] = input[i++];
        }

        // Shift alphabetic characters by the specified amount
        if (isAlpha(input[i]))
        {
            char base; // start at the beginning of the alphabet each time
            if (isLower(input[i]))
            {
                base = 'a';
            }
            else
            {
                base = 'A';
            }

            // shift all the characters by 3
            output[j++] = (input[i] - base + shift) % 26 + base;
            i++;
        }
    }

    // Null-terminate the output string
    output[j] = '\0';

    // Return success
    return 0;
}

// Decode Caesar Cipher
int decodeCipher(const char *input, char *output, size_t output_size)
{
    size_t i = 0;
    size_t j = 0;

    int shift = 3; // Should be the same shift value used for encryption

    // Check if input, output, or output_size is invalid
    if (input == NULL || *input == '\0' || output == NULL)
        return -1;

    // Iterate through the input string
    while (input[i] && j < output_size - 1)
    {
        // Skip non-alphabetic characters
        while (input[i] && !isAlpha(input[i]))
        {
            // Copy non-alphabetic character as is
            output[j++] = input[i++];
        }

        // Shift alphabetic characters by the specified amount in the opposite direction
        if (isAlpha(input[i]))
        {
            char base; // start at the beginning of the alphabet each time
            if (isLower(input[i]))
            {
                base = 'a';
            }
            else
            {
                base = 'A';
            }

            // Reverse the Caesar shift
            output[j++] = (input[i] - base - shift + 26) % 26 + base;
            i++;
        }
    }

    // Null-terminate the output string
    output[j] = '\0';

    // Return success
    return 0;
}

// Check if a character is a vowel
int isVowel(char c)
{
    return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');
}

// Check if a character is a lowercase letter
int isLower(char c)
{
    return ('a' <= c && c <= 'z');
}

// Check if a character is an alphabetic character
int isAlpha(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

// Convert a character to lowercase
char toLower(char c)
{
    return ('A' <= c && c <= 'Z') ? c + ('a' - 'A') : c;
}