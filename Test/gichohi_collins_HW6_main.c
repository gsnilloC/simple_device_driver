/**************************************************************
 * Class:  CSC-415-01 Fall 2023
 * Name: Collins Gichohi
 * Student ID: 922440815
 * GitHub UserID: gsnilloC
 * Project: Assignment 6 – Device Driver
 *
 * File: gichohi_collins_HW6_main.c
 * Test program for device driver.
 *
 **************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h> // Add this line
#include <sys/stat.h>  // Add this line

// Define the path to the device file
#define PATH "/dev/devTranslator"

// Custom IOCTL command value
#define CUSTOM_IOCTL_COMMAND 3

int main(int argc, char const *argv[])
{
    // Open the device file for reading and writing
    int fd = open(PATH, O_RDWR);
    int flag;

    // Check if the file descriptor is valid
    if (fd < 0)
    {
        perror("Error opening device");
        return -1;
    }
    else
    {
        // Display welcome message if the device is opened successfully
        printf("\n--------------------------------\n");
        printf("Welcome to THE Translator! \n");
        printf("Type 'q' to exit\n\n\n");
    }

    // Initialize buffers and quit command
    char buffer[1024];
    char readIntoBuffer[1024];
    const char *quitCommand = "q";

    printf("Which mode would you like to Enter?:\n1: PigLatin\n");
    printf("2: Encode\n");
    printf("3: Decode\n\n");

    // Use scanf to get an integer input
    if (scanf("%d", &flag) != 1)
    {
        printf("\nPlease choose a mode.\n");
        return -1;
    }

    if (flag > 3 || flag < 0){
        printf("Please enter a valid mode!\n");
        return -1;
    }

    printf("\n");
    // Flush the standard input buffer
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    // Call the ioctl command to retrieve the translation count
    if (ioctl(fd, CUSTOM_IOCTL_COMMAND, &flag) == -1)
    {
        perror("Error calling ioctl");
        close(fd);
        return -1;
    }

    // Main translation loop
    while (1)
    {
        // Prompt the user to enter text for translation
        printf("Enter text to translate:\n");
        fgets(buffer, sizeof(buffer), stdin);
        printf("\n");

        // Remove the newline character from the input buffer
        buffer[strcspn(buffer, "\n")] = '\0';

        // Check if the user wants to quit
        if (strcmp(buffer, quitCommand) == 0)
        {
            printf("Exiting translator!\n");
            break;
        }

        // Write input words to the device
        ssize_t bytesWritten = write(fd, buffer, strlen(buffer));

        // Check for errors during write
        if (bytesWritten < 0)
        {
            perror("Error writing to the device");
            close(fd);
            return -1;
        }
        else if (bytesWritten != strlen(buffer))
        {
            fprintf(stderr, "Incomplete write to the device\n");
            close(fd);
            return -1;
        }

        // Read translated text from the device
        ssize_t bytesRead = read(fd, readIntoBuffer, sizeof(readIntoBuffer));

        // Check for errors during read
        if (bytesRead == -1)
        {
            perror("Error reading from the device");
            close(fd);
            return -1;
        }

        // Null-terminate the translated text
        readIntoBuffer[bytesRead] = '\0';

        // Print the translated text to the console
        printf("Translated text:\n%.*s\n\n", (int)bytesRead, readIntoBuffer);
    }

    // Close the device file when exiting the program
    close(fd);
    return 0;
}