#include <ncurses.h>
#include <jpeglib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int width;
  int height;
  int components;
  unsigned char * pixels;
} image_t;

unsigned char
rgb(unsigned char r, unsigned char g, unsigned char b) {
  float rf = (float)r/255.0f;
  float gf = (float)g/255.0f;
  float bf = (float)b/255.0f;

  return (unsigned char)(((int)(rf * 5.0f) * 36) +
                         ((int)(gf * 5.0f) * 6) +
                         (int)(bf * 5.0f) + 16);
}

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
  buffer[0] = (JSAMPROW)malloc(sizeof(JSAMPLE) * row_stride);

  img->width = cinfo.output_width;
  img->height = cinfo.output_height;
  img->components = cinfo.output_components;
  
  img->pixels =
    (unsigned char *)malloc(cinfo.output_width *
                            cinfo.output_height *
                            cinfo.output_components);

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

void
draw_jpeg_file(image_t *image, int fit_width, int fit_height) {
  // terminal heights generally /2 due to character heightsw
  float image_aspect = (float)image->width / (float)image->height;
  float term_aspect = ((float)fit_width / (float)fit_height) / 2;

  float scale;

  if (term_aspect > image_aspect) {
    // terminal is wider than image
    scale = (float)fit_height / (float)image->height;
    fit_width = (int)((float)image->width * scale) * 2;
  } else {
    scale = (float)fit_width / (float)image->width;
    fit_height = (int)((float)image->height * scale) / 2;
  }

  int x = 0,
    y = 0,
    m = 0;

  int x_margin = fit_width < COLS ? (COLS - fit_width - 1) / 2 : 0;
  int y_margin = fit_height < LINES ? (LINES - fit_height - 1) / 2 : 0;
  
  for (m = 0; m < y_margin; m++) {
    addch('\n');
  }
  
  for (y = 0; y < fit_height; y++) {

    for (m = 0; m < x_margin; m++) {
      addch(' ');
    }
    
    int img_y = (int)(((float)y / (float)fit_height) * image->height);
    for (x = 0; x < fit_width; x++) {
      
      int img_x = (int)(((float)x / (float)fit_width) * image->width);
      unsigned long index = (img_y *
                             image->width *
                             image->components) +
        (img_x * image->components);
      
      unsigned char *offset = (unsigned char *)(image->pixels + index);

      unsigned char color;
      if (image->components == 1) {
        color = rgb(*offset, *offset, *offset);
      } else {
        color = rgb(*(offset), *(offset + 1), *(offset + 2));
      }
      
      attron(COLOR_PAIR(color));
      addch(' ' | A_REVERSE);
      attroff(COLOR_PAIR(color));
    }

    if (x < (COLS - 1)) {
      addch('\n');
    }
  }
}

int
main(int argc, char **argv) {
  if (argc != 2) {
    printf("Correct usage: termimg FILENAME\n");
    exit(1);
  }
  char *filename = argv[1];

  image_t image;
  FILE *fin = fopen(filename, "r");
  load_jpeg_file(fin, &image);
  fclose(fin);
  
  initscr();
  if (!has_colors()) {
    endwin();
    printf("Color support not detected.\n");
    exit(1);
  }

  start_color();
  for (int i = 0; i < 256; i++) {
    init_pair(i, i, COLOR_BLACK);
  }


  draw_jpeg_file(&image, COLS, LINES);

  refresh();
  getch();
  endwin();

  free(image.pixels);
  return 0;
}
