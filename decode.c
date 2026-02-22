#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decode.h"
#include "types.h"
#include "common.h"

/* Helper decode function: Decode 1 byte of secret data from the LSBs of 8 bytes of image data */
Status_d decode_byte_from_lsb(char *data, char *image_buffer)
{
    if (data == NULL || image_buffer == NULL)
        return d_failure;

    char decoded_byte = 0;
    
    // Iterate through the 8 bytes of the image buffer
    for (int i = 0; i < 8; i++)
    {
        // 1. Shift the decoded_byte one position to the left.
        decoded_byte = decoded_byte << 1;
        
        // 2. Extract the LSB from the current image_buffer byte (image_buffer[i] & 0x01).
        // 3. Combine the extracted LSB with the decoded_byte using bitwise OR.
        decoded_byte |= (image_buffer[i] & 0x01);
    }
    
    // Store the resulting decoded byte
    *data = decoded_byte;

    return d_success;
}

/* Helper decode function: Extract a 32-bit integer from 32 LSBs */
Status_d decode_size_from_lsb(int *size, char *imageBuffer)
{
    int value = 0;
    for (int i = 0; i < 32; i++)
    {
        value = value << 1;
        value |= (imageBuffer[i] & 0x01);
    }
    *size = value;
    return d_success;
}


/* Read and validate decode arguments */
Status_d read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    // Validate stego image filename
    if (strlen(argv[2]) < 4)
    {
        printf("ERROR: Invalid source file name length.\n");
        return d_failure;
    }

    char *src_ext = argv[2] + strlen(argv[2]) - 4;
    if (strcmp(src_ext, ".bmp") != 0)
    {
        printf("ERROR: Invalid source file. Use .bmp files.\n");
        return d_failure;
    }

    strcpy(decInfo->stego_image_fname, argv[2]);

    // Handle output argument
    if (argv[3] != NULL)
    {
        // Copy output filename for later use
        strncpy(decInfo->output_fname, argv[3], sizeof(decInfo->output_fname) - 1);
        decInfo->output_fname[sizeof(decInfo->output_fname) - 1] = '\0';
        decInfo->fptr_output = NULL;
    }
    else
    {
        // This case should ideally be handled by main.c argument check, but defensively set empty
        decInfo->output_fname[0] = '\0';
        decInfo->fptr_output = NULL;
    }

    return d_success;
}

/* Open files */
Status_d open_decode_files(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb");
    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open stego image %s\n", decInfo->stego_image_fname);
        return d_failure;
    }
    return d_success;
}

/* Decode the magic string to verify hidden data */
Status_d decode_magic_string(DecodeInfo *decInfo)
{
    char imageBuffer[8];
    char ch;
    
    // Allocate buffer for magic string + NULL terminator
    char magic[strlen(MAGIC_STRING) + 1]; 
    
    // Skip BMP header (54 bytes)
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET); 

    for (int i = 0; i < strlen(MAGIC_STRING); i++)
    {
        if (fread(imageBuffer, 1, 8, decInfo->fptr_stego_image) != 8)
        {
            fprintf(stderr, "ERROR: Failed to read image data for magic string.\n");
            return d_failure;
        }
        
        if (decode_byte_from_lsb(&ch, imageBuffer) == d_failure)
            return d_failure;
            
        magic[i] = ch;
    }
    magic[strlen(MAGIC_STRING)] = '\0'; // Null-terminate

    if (strcmp(magic, MAGIC_STRING) == 0)
    {
        printf("Magic string verified: %s\n", magic);
        return d_success;
    }
    else
    {
        fprintf(stderr, "ERROR: Magic string mismatch! No hidden data found.\n");
        return d_failure;
    }
}

/* Decode size of file extension (32 bits) */
Status_d decode_secret_file_extn_size(DecodeInfo *decInfo, int *extn_size)
{
    char imageBuffer[32];
    
    if (fread(imageBuffer, 1, 32, decInfo->fptr_stego_image) != 32)
    {
        fprintf(stderr, "ERROR: Failed to read image data for extension size.\n");
        return d_failure;
    }
    
    if (decode_size_from_lsb(extn_size, imageBuffer) == d_failure)
        return d_failure;

    // Use the constant defined in decode.h (10) for maximum check
    if (*extn_size >= sizeof(decInfo->extn_secret_file) || *extn_size < 0) 
    {
        fprintf(stderr, "ERROR: Decoded extension size (%d) is too large or invalid.\n", *extn_size);
        return d_failure;
    }

    printf("Extension size decoded: %d\n", *extn_size);
    return d_success;
}

/* Decode extension string (8 bits per char) */
Status_d decode_secret_file_extn(DecodeInfo *decInfo, char *extn, int extn_size)
{
    char imageBuffer[8];
    char ch;

    // The size check is redundant if decode_secret_file_extn_size passed, but kept for safety.
    if (extn_size >= sizeof(decInfo->extn_secret_file) || extn_size < 0)
    {
        fprintf(stderr, "ERROR: Decoded extension size is invalid for buffer.\n");
        return d_failure;
    }
    
    for (int i = 0; i < extn_size; i++)
    {
        if (fread(imageBuffer, 1, 8, decInfo->fptr_stego_image) != 8)
        {
            fprintf(stderr, "ERROR: Failed to read image data for extension string.\n");
            return d_failure;
        }
        
        if (decode_byte_from_lsb(&ch, imageBuffer) == d_failure)
            return d_failure;
            
        extn[i] = ch;
    }

    extn[extn_size] = '\0';
    // Use strncpy for safe copy into the struct, ensuring null termination
    strncpy(decInfo->extn_secret_file, extn, sizeof(decInfo->extn_secret_file) - 1);
    decInfo->extn_secret_file[sizeof(decInfo->extn_secret_file) - 1] = '\0';

    printf("Extension decoded: %s\n", decInfo->extn_secret_file);
    return d_success;
}

/* Decode secret file size (32 bits) */
Status_d decode_secret_file_size(DecodeInfo *decInfo, long *file_size)
{
    char imageBuffer[32];
    
    if (fread(imageBuffer, 1, 32, decInfo->fptr_stego_image) != 32)
    {
        fprintf(stderr, "ERROR: Failed to read image data for secret file size.\n");
        return d_failure;
    }
    
    int temp_size;

    if (decode_size_from_lsb(&temp_size, imageBuffer) == d_failure)
        return d_failure;

    // Cast the read integer size to long for safety with large files
    *file_size = (long)temp_size; 
    
    printf("Secret file size decoded: %ld bytes\n", *file_size);
    return d_success; 
}

/* Decode the actual secret data and write to file */
Status_d decode_secret_file_data(DecodeInfo *decInfo, long file_size)
{
    char imageBuffer[8];
    char ch;
    char output_fname_final[100];
    char temp_output[100];

    // 1. Prepare filename for output
    strncpy(temp_output, decInfo->output_fname, sizeof(temp_output) - 1);
    temp_output[sizeof(temp_output) - 1] = '\0';
    temp_output[strcspn(temp_output, "\r\n")] = '\0'; // Remove newline if present

    const char *extn = decInfo->extn_secret_file;
    char *dot = strrchr(temp_output, '.');

    // Check if the output name already ends with the correct extension
    // We only append the extension if the decoded extension is not empty.
    if (*extn != '\0')
    {
        size_t extn_len = strlen(extn);
        // Check if output file name already ends with the required extension
        if (dot != NULL && (strlen(dot + 1) == extn_len) && (strcmp(dot + 1, extn) == 0))
        {
            // Extension matches, use name as is.
            strcpy(output_fname_final, temp_output);
        }
        else
        {
            // Extension needs to be added or corrected.
            if (dot != NULL)
            {
                *dot = '\0'; // Truncate the filename before the dot
            }

            // Append the correct extension
            strcpy(output_fname_final, temp_output); 
            strcat(output_fname_final, ".");        
            strcat(output_fname_final, extn);       
        }
    }
    else
    {
        // Decoded extension is empty (file had no extension), use output_fname as is.
        strcpy(output_fname_final, temp_output);
    }
    
    decInfo->fptr_output = fopen(output_fname_final, "wb");
    if (decInfo->fptr_output == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open output file for writing: %s\n", output_fname_final);
        return d_failure;
    }

    // Decode and write the secret data byte by byte
    for (long i = 0; i < file_size; i++)
    {
        if (fread(imageBuffer, 1, 8, decInfo->fptr_stego_image) != 8)
        {
            fprintf(stderr, "ERROR: Failed to read image data for secret file content at byte %ld.\n", i);
            fclose(decInfo->fptr_output);
            return d_failure;
        }
        
        if (decode_byte_from_lsb(&ch, imageBuffer) == d_failure)
        {
            fprintf(stderr, "ERROR: Failed to decode byte %ld.\n", i);
            fclose(decInfo->fptr_output);
            return d_failure;
        }
        
        if (fputc(ch, decInfo->fptr_output) == EOF)
        {
            fprintf(stderr, "ERROR: Failed to write byte %ld to output file.\n", i);
            fclose(decInfo->fptr_output);
            return d_failure;
        }
    }

    fclose(decInfo->fptr_output);
    printf("Secret file successfully decoded and saved as '%s'\n", output_fname_final);
    return d_success;
}

/* Full decoding workflow */
Status_d do_decoding(DecodeInfo *decInfo)
{
    // 1. Open stego image
    if (open_decode_files(decInfo) != d_success)
    {
        fprintf(stderr, "ERROR: Opening stego image failed\n");
        return d_failure;
    }
    printf("Stego image opened successfully.\n");

    // 2. Decode magic string
    if (decode_magic_string(decInfo) != d_success)
    {
        fclose(decInfo->fptr_stego_image);
        return d_failure;
    }

    // 3. Decode extension size
    int extn_size;
    if (decode_secret_file_extn_size(decInfo, &extn_size) != d_success)
    {
        fclose(decInfo->fptr_stego_image);
        return d_failure; 
    }
    
    // 4. Decode extension string
    char extn[10];
    if (decode_secret_file_extn(decInfo, extn, extn_size) != d_success)
    {
        fclose(decInfo->fptr_stego_image);
        return d_failure; 
    }

    // 5. Decode file size
    long file_size;
    if (decode_secret_file_size(decInfo, &file_size) != d_success)
    {
        fclose(decInfo->fptr_stego_image);
        return d_failure;
    }

    // 6. Decode secret data
    if (decode_secret_file_data(decInfo, file_size) != d_success)
    {
        fclose(decInfo->fptr_stego_image);
        return d_failure;
    }

    // The decode_secret_file_data closes the output file.
    fclose(decInfo->fptr_stego_image);
    printf("Decoding completed successfully!\n");
    return d_success;
}