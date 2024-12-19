#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024
#define ENCRYPTION_OFFSET 100
#define DECRYPTION_OFFSET -100

void close_files(int input_fd, int output_fd);
void encrypt_decrypt_buffer(char *buffer, int size, int offset);
int open_file(const char *filename, int flags);
int create_output_filename(const char *input_filename, char *output_filename, const char *suffix);

int process_file(const char *flag, const char *input_filename)
{
    int input_fd = open_file(input_filename, O_RDONLY);
    if (input_fd < 0)
    {
        perror("Error opening input file");
        return -1;
    }

    char output_filename[128];
    if (create_output_filename(input_filename, output_filename, (strcmp(flag, "-e") == 0) ? "_enc" : "_dec") < 0)
    {
        close(input_fd);
        perror("Error creating output filename");
        return -1;
    }

    int output_fd = open_file(output_filename, O_WRONLY | O_CREAT);
    if (output_fd < 0)
    {
        close(input_fd);
        perror("Error opening output file");
        return -1;
    }

    // Variables for time measurement
    struct timeval start, end;
    gettimeofday(&start, NULL);  // Start timer

    int read_count = 0, write_count = 0;

    // Read input file in chunks
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int offset = (strcmp(flag, "-e") == 0) ? ENCRYPTION_OFFSET : DECRYPTION_OFFSET;

    while ((bytes_read = read(input_fd, buffer, BUFFER_SIZE)) > 0)
    {
        read_count++;

        encrypt_decrypt_buffer(buffer, bytes_read, offset);

        ssize_t bytes_written = write(output_fd, buffer, bytes_read);
        if (bytes_written < 0)
        {
            close_files(input_fd, output_fd);
            perror("Error writing to output file");
            return -1;
        }
        write_count++;  // Increment write call counter
    }

    if (bytes_read < 0)
    {
        perror("Error reading from input file");
        return -1;
    }

    close_files(input_fd, output_fd);

    // End timer and calculate elapsed time
    gettimeofday(&end, NULL);
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed = seconds + microseconds * 1e-6;

    return (bytes_read < 0) ? -1 : 0;
}

// Encrypt or decrypt a buffer using the given offset
void encrypt_decrypt_buffer(char *buffer, int size, int offset)
{
    for (int i = 0; i < size; i++)
    {
        buffer[i] += offset;
    }
}

// Open a file with specified flags and handle errors
int open_file(const char *filename, int flags)
{
    int fd = open(filename, flags, 0644);
    return fd;
}

// Close files
void close_files(int input_fd, int output_fd)
{
    close(input_fd);
    close(output_fd);
}

// Create output filename based on input filename and suffix
int create_output_filename(const char *input_filename, char *output_filename, const char *suffix)
{
    // Find the last occurrence of '.' in the input filename
    char *extension = strrchr(input_filename, '.');
    if (extension != NULL)
    {
        // Copy the input filename up to the extension
        int base_length = extension - input_filename;
        strncpy(output_filename, input_filename, base_length);
        output_filename[base_length] = '\0';
        strcat(output_filename, suffix);
        strcat(output_filename, extension);
    }
    else
    {
        strcpy(output_filename, input_filename);
        strcat(output_filename, suffix);
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3 || (strcmp(argv[1], "-e") != 0 && strcmp(argv[1], "-d") != 0))
    {
        fprintf(stderr, "Usage: %s -e|-d [file_name]\n", argv[0]);
        return -1;
    }
    return process_file(argv[1], argv[2]);
}
