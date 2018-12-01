  /*
   * Cameron Moberg, Devin Hudson, Brandon Ruoff
   * read_png_file and write_png_file logic from GitHub user @niw
   */
  #include <math.h>
  #include <string.h>
  #include <stdlib.h>
  #include <stdio.h>
  #include <png.h>

  #define SIZE 32
  #define SBYTE 8

  int width, height;
  png_byte color_type, bit_depth;
  png_bytep * row_pointers;

  long file_size_bytes(FILE * fp) {
      /*
       * Function: file_size_bytes
       * --------------------
       * Determines the number of bytes that a file contains.
       *
       *  fp: file to determine size of
       *
       *  returns: File size in bytes
       */
      fseek(fp, 0L, SEEK_END);
      long sz = ftell(fp);
      rewind(fp);
      return sz;
  }

  long bstr_to_dec(const char * str, int len) {
      /*
       * Function: bstr_to_dec
       * --------------------
       * Converts a char array of 1s and 0s (binary) to decimal
       *
       *  str: pointer to string to convert
       *  len: length of the string to convert
       *
       *  returns: long of decimal representation of binary str
       */
      long val = 0;

      for (int i = 0; i < len; i++)
          if (str[i] == 1)
              val = val + pow(2, len - 1 - i);

      return val;
  }

  void write_size_to_px(long size, png_bytep * row_pointers) {
      /*
       * Function: write_size_to_px
       * --------------------
       *  Given the row pointers, will write a long, 4 bytes, to pixels 1 bit per pixel.
       */
      unsigned char parsed_size[SIZE];
      for (int i = 0; i < SIZE; i++) /* Iterate 32 times for the 4 bytes per long */ {
          png_bytep px = & (row_pointers[i / width][i * 4]);
          px[0] = (px[0] & ~1) | size >> 31 - i;
      }
  }
  long extract_size_of_cipher() {
      /*
       * Function: extract_size_of_cipher
       * --------------------
       *  After parsing a png file, reads the first 32 pixels and
       *  parses it into a long, so 0 0 0 0 0 1 1 would be returned as a long 3.
       *
       *  returns: Cipher size as a long
       */
      unsigned char parsed_size[SIZE];
      for (int i = 0; i < SIZE; i++) { /* Iterate 32 times for the 4 bytes per long */
          png_bytep px = & (row_pointers[i / width][i * 4]);
          parsed_size[i] = (char)(px[0] & 1);
      }
      return bstr_to_dec(parsed_size, SIZE);
  }

  void read_png_file(char * fn) {
      /*
       * Function: read_png_file
       * --------------------
       *  Given a png's file name, opens that png and converts it, if need be,
       *  and then parses it into the global variables above.
       *
       *  returns: Nothing, but updates global variables.
       */
      FILE * fp = fopen(fn, "rb");

      png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
      if (!png || !fp)
          exit(1);

      png_infop info = png_create_info_struct(png);
      if (!info)
          exit(1);

      if (setjmp(png_jmpbuf(png))) abort();

      png_init_io(png, fp);
      png_read_info(png, info);

      width = png_get_image_width(png, info);
      height = png_get_image_height(png, info);
      color_type = png_get_color_type(png, info);
      bit_depth = png_get_bit_depth(png, info);

      // Read any color_type into 8bit depth, RGBA format.
      // See http://www.libpng.org/pub/png/libpng-manual.txt

      if (bit_depth == 16)
          png_set_strip_16(png);

      if (color_type == PNG_COLOR_TYPE_PALETTE)
          png_set_palette_to_rgb(png);

      // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
      if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
          png_set_expand_gray_1_2_4_to_8(png);

      if (png_get_valid(png, info, PNG_INFO_tRNS))
          png_set_tRNS_to_alpha(png);

      // The included types are RGB, Grayscale or Palette
      if (color_type < PNG_COLOR_TYPE_GRAY_ALPHA)
          png_set_filler(png, 255, PNG_FILLER_AFTER);

      if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
          png_set_gray_to_rgb(png);

      png_read_update_info(png, info);

      row_pointers = (png_bytep * ) malloc(sizeof(png_bytep) * height);
      for (int y = 0; y < height; y++) {
          row_pointers[y] = (png_byte * ) malloc(png_get_rowbytes(png, info));
      }

      png_read_image(png, row_pointers);

      fclose(fp);
  }

  void write_png_file(char * filename) {
      /*
       * Function: write_png_file
       * --------------------
       *  Given a png's file name, opens that png and parses and writes the global
       * variables into a valid png file form and writes it.
       *
       *  returns: Nothing, but updates global vars to file.
       */
      FILE * fp = fopen(filename, "wb");
      png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

      if (!fp || !png)
          exit(1);

      png_infop info = png_create_info_struct(png);

      if (!info || setjmp(png_jmpbuf(png)))
          exit(1);

      png_init_io(png, fp);

      // Output is 8bit depth, RGBA format.
      png_set_IHDR(
          png,
          info,
          width, height,
          8,
          PNG_COLOR_TYPE_RGBA,
          PNG_INTERLACE_NONE,
          PNG_COMPRESSION_TYPE_DEFAULT,
          PNG_FILTER_TYPE_DEFAULT
      );

      png_write_info(png, info);
      png_write_image(png, row_pointers);
      png_write_end(png, NULL);

      for (int y = 0; y < height; y++)
          free(row_pointers[y]);

      free(row_pointers);

      fclose(fp);
  }


  void add_steg_png(char * plaintext_fn) {
      /* Hides the given plaintext file into the global variables, which will then be
       * written to file later.
       */
      FILE * fp = fopen(plaintext_fn, "rb");
      int byte_r = 0;

      unsigned char buffer[256];

      if (file_size_bytes(fp) > (height * width - 32) / SBYTE)
      {
          printf("Plaintext too large for given PNG.\n");
          exit(1);
      }
      write_size_to_px(file_size_bytes(fp), row_pointers);

      // Set init_i to SIZE since we just wrote the file size.
      int offset = SIZE;
      long png_idx = 32;

      while (byte_r = (fread(buffer, sizeof(unsigned char), 256, fp))) {
          int byte_counter, byte_index = 0;
          for (int i = offset; i < offset + (byte_r * SBYTE); i++) {
              png_bytep row = row_pointers[png_idx / width];
              png_bytep px = & (row[(png_idx % width) * 4]);
              unsigned char c = buffer[byte_index];
              // Replace last bit of px[0] with current bit of character
              px[0] = (px[0] & ~1) | (c >> SBYTE - 1 - (i % 8));

              // One character finished
              if (byte_counter++ % SBYTE == SBYTE - 1) {
                  byte_index++;
              }
              png_idx++;
          }
          offset = 0;
      }
  }

  void extract_steg_from_png(char * plaintext_fn) {
    /* Extracts the hidden steganography from parsed file
     * then writes it to the provided file.
     */
    FILE * fp = fopen(plaintext_fn, "wb");

    long cipher_size = extract_size_of_cipher();

    unsigned char parsed_byte[SBYTE];
    for (int i = SIZE; i < SIZE + (cipher_size * SBYTE); i++) {
        png_bytep row = row_pointers[i / width];
        png_bytep px = & (row[(i % width) * 4]);
        parsed_byte[i % SBYTE] = px[0] & 1;

        if (i % SBYTE == SBYTE - 1) {
            char conv_byte = (char) bstr_to_dec(parsed_byte, SBYTE);
            fwrite( & conv_byte, sizeof(unsigned char), 1, fp);
        }
    }
    fclose(fp);
  }

  int main(int argc, char * argv[]) {
    // Encoding is executed like:
    // ./a.out -e <COVER.PNG> <CIPHER.TXT> <OUTPUT.PNG>
    if (strcmp(argv[1], "-e") == 0) {
        // Cover png
        read_png_file(argv[2]);
        // Cipher text
        add_steg_png(argv[3]);
        // Steganography PNG
        write_png_file(argv[4]);
    }
    // Decoding is executed like:
    // ./a.out -d <STEG.PNG> <OUTPUT.TXT>
    else if (strcmp(argv[1], "-d") == 0) {
        // Steganography PNG
        read_png_file(argv[2]);
        // Cipher text output file
        extract_steg_from_png(argv[3]);
    } else
        printf("Arguments not understood.");

    return 0;
}
