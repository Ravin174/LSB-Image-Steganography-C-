Overview
A secure data-hiding utility implemented in C that utilizes Least Significant Bit (LSB) substitution to embed secret text within 24-bit BMP images. This tool allows for the invisible transmission of data by modifying the lowest bit of each pixel byte, ensuring that the visual integrity of the carrier image remains intact while storing encrypted or private information.

Key Features
  
  LSB Encoding: Hides secret .txt files inside .bmp images by replacing the least significant bit of the image's RGB data.
  Secure Decoding: Extracts hidden bitstreams from "stego" images to perfectly reconstruct the original secret message.
  Format Preservation: Maintains the BMP header and structure to ensure the output image remains viewable by any standard image viewer.
  Argument Validation: Features robust command-line interface (CLI) validation for input files, secret files, and optional output naming.
  High Payload Capacity: Optimized to handle secret data based on the carrier image's size.

Technical Implementation

  Language: C.
  Core Techniques: Bitwise operations (AND/OR/Shifting), File I/O (Binary Mode), and DMA (Dynamic Memory Allocation).
  Modular Design: Divided into encode.c and decode.c modules for clear separation of logic.
  Image Processing: Specifically designed for the BMP (Bitmap) format to manipulate raw pixel arrays.

How to Use

Compile: use gcccompailer to compail
Encode Data:
Decode Data:
