/* splurt
 * The Useless Terminal Jpeg Viewer
 *
 * Copyright (c) 2012 Ryan Crum <ryan.j.crum@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <ncurses.h>
#include <jpeglib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "splurt.h"

/**
 * Find the squared euclidean distance of a 3-dimensional
 * vector.
 */
int
euclidean_dist_sq_3(unsigned char *q,
                    unsigned char *p) {
  int diff1 = (int)q[0] - (int)p[0];
  int diff2 = (int)q[1] - (int)p[1];
  int diff3 = (int)q[2] - (int)p[2];

  return (diff1 * diff1) +
    (diff2 * diff2) +
    (diff3 * diff3);
}

/**
 * Convert a 24-bit color into an 8-bit terminal color.
 */
unsigned char
rgb(unsigned char r, unsigned char g, unsigned char b) {
  unsigned int lowest_dist = UINT_MAX;
  int lowest_idx = 0;
  int i = 0;
  int dist = 0;

  for (i = 1; i < 256; ++i) {
    unsigned char vec[3] = { r, g, b };
    dist = euclidean_dist_sq_3(COLOR_TABLE[i],
                               vec);

    if (dist < lowest_dist) {
      lowest_dist = dist;
      lowest_idx = i;
    }
  }

  return lowest_idx;
}

/**
 * Read a jpeg file into an image_t struct.
 */
void
load_jpeg_file(FILE *in_file, image_t *img) {
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  JSAMPARRAY buffer;
  int row_stride;
  long counter = 0;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, in_file);

  jpeg_read_header(&cinfo, TRUE);

  jpeg_start_decompress(&cinfo);

  row_stride = cinfo.output_width * cinfo.output_components;

  buffer = (JSAMPARRAY)malloc(sizeof(JSAMPROW));
  assert(buffer != NULL);
  buffer[0] = (JSAMPROW)malloc(sizeof(JSAMPLE) * row_stride);
  assert(buffer[0] != NULL);

  img->width = cinfo.output_width;
  img->height = cinfo.output_height;
  img->components = cinfo.output_components;

  img->pixels = (unsigned char *)malloc(cinfo.output_width *
                                        cinfo.output_height *
                                        cinfo.output_components);

  assert(img->pixels != NULL);

  while (cinfo.output_scanline < cinfo.output_height) {
    jpeg_read_scanlines(&cinfo, buffer, 1);
    memcpy(img->pixels + counter, buffer[0], row_stride);
    counter += row_stride;
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  free(buffer[0]);
  free(buffer);
}

/**
 * Draw the image_t onto the terminal inside the specified dimensions.
 * Scales the image as necessary for aspect-ratio'ing with a naive
 * nearest-neighbor algorithm.
 */
void
draw_jpeg_file(image_t *image, int fit_width, int fit_height) {

  // terminal height /2 due to character heights
  float image_aspect = (float)image->width / (float)image->height,
    term_aspect = ((float)fit_width / (float)fit_height) / 2,
    scale = 1.0f;

  int x = 0,
    y = 0,
    m = 0,
    x_margin = 0,
    y_margin = 0,
    img_x = 0,
    img_y = 0,
    y_offset = 0;

  unsigned long index = 0;
  unsigned char color = 0;
  unsigned char *offset = NULL;


  if (term_aspect > image_aspect) {
    // terminal is wider than image
    scale = (float)fit_height / (float)image->height;
    fit_width = (int)((float)image->width * scale) * 2;
  } else {
    scale = (float)fit_width / (float)image->width;
    fit_height = (int)((float)image->height * scale) / 2;
  }

  x_margin = fit_width < COLS ? (COLS - fit_width - 1) / 2 : 0;
  y_margin = fit_height < LINES ? (LINES - fit_height - 1) / 2 : 0;

  for (y = 0; y < fit_height; y++) {
    img_y = (int)(((float)y / (float)fit_height) * image->height);
    y_offset = (img_y *
                image->width *
                image->components);

    for (x = 0; x < fit_width; x++) {
      img_x = (int)(((float)x / (float)fit_width) * image->width);
      index = y_offset + (img_x * image->components);

      offset = (unsigned char *)(image->pixels + index);

      if (image->components == 1) {
        // grayscale
        color = rgb(*offset, *offset, *offset);
      } else {
        // full color
        color = rgb(*(offset), *(offset + 1), *(offset + 2));
      }

      attron(COLOR_PAIR(color));
      mvaddch(y + y_margin, x + x_margin, ' ' | A_REVERSE);
      attroff(COLOR_PAIR(color));
    }
  }
}

int
main(int argc, char **argv) {
  image_t image;
  FILE *fin = NULL;
  int i = 0, j = 0;

  if (argc < 2) {
    printf("Correct usage: splurt FILENAME\n");
    exit(1);
  }

  // Validate the terminal properties
  initscr();
  if (!has_colors()) {
    endwin();
    printf("Color support not detected.\n");
    exit(1);
  }

  if (tigetnum("colors") != 256) {
    endwin();
    printf("256 color support not detected.\n");
    exit(1);
  }
  endwin();

  for (j = 1; j < argc; j++) {
    // Load jpeg outside of ncurses context so any errors leave the terminal
    // in a good state
    fin = fopen(argv[j], "r");
    if (fin != NULL) {
      load_jpeg_file(fin, &image);
      fclose(fin);

      initscr();

      // hide cursor
      curs_set(0);

      start_color();

      // initialize the color pairs
      for (i = 0; i < 256; ++i) {
        init_pair(i, i, COLOR_BLACK);
      }

      draw_jpeg_file(&image, COLS, LINES);

      refresh();
      getch();

      clear(); // clear the terminal to prepare for the next image

      endwin();

      free(image.pixels);
      image.pixels = NULL;
    } else {
      printf("File not found: %s\n", argv[j]);
    }
  }

  return 0;
}
