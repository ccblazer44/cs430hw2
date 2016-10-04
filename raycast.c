#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

//#define DEBUG
#define AUTHOR "CBLAZER"
#define RGB_NUMBER 255


typedef struct {
    char *type;
    double *color;
    double *position;
    double *normal;
    double radius;
} Object;

Object objects[128];

int line = 1;
int Width;
int Height;
int color;
double cameraWidth;
double cameraHeight;



// next_c() wraps the getc() function and provides error checking and line
// number maintenance
int next_c(FILE* json) {
  int c = fgetc(json);
#ifdef DEBUG
  printf("next_c: '%c'\n", c);
#endif
  if (c == '\n') {
    line += 1;
  }
  if (c == EOF) {
    fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
    exit(1);
  }
  return c;
}

// expect_c() checks that the next character is d.  If it is not it emits
// an error.
void expect_c(FILE* json, int d) {
  int c = next_c(json);
  if (c == d) return;
  fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
  exit(1);
}

// skip_ws() skips white space in the file.
void skip_ws(FILE* json) {
  int c = next_c(json);
  while (isspace(c)) {
    c = next_c(json);
  }
  ungetc(c, json);
}

// next_string() gets the next string from the file handle and emits an error
// if a string can not be obtained.
char* next_string(FILE* json) {
  char buffer[129];
  int c = next_c(json);
  if (c != '"') {
    fprintf(stderr, "Error: Expected string on line %d.\n", line);
    exit(1);
  }
  c = next_c(json);
  int i = 0;
  while (c != '"') {
    if (i >= 128) {
      fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
      exit(1);
    }
    if (c == '\\') {
      fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
      exit(1);
    }
    if (c < 32 || c > 126) {
      fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
      exit(1);
    }
    buffer[i] = c;
    i += 1;
    c = next_c(json);
  }
  buffer[i] = 0;
  return strdup(buffer);
}

double next_number(FILE* json) {
  double value;
  fscanf(json, "%lf", &value);
  // Error check this..
  return value;
}

double* next_vector(FILE* json) {
  double* v = malloc(3*sizeof(double));
  expect_c(json, '[');
  skip_ws(json);
  v[0] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[1] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[2] = next_number(json);
  skip_ws(json);
  expect_c(json, ']');
  return v;
}

void read_scene(char* filename) {
  int c;
  FILE* json = fopen(filename, "r");

  if (json == NULL) {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
    exit(1);
  }

  skip_ws(json);

  // Find the beginning of the list
  expect_c(json, '[');

  skip_ws(json);

  // Find the objects
  int index = -1;
  while (1) {
    index++;
    c = fgetc(json);
    if (c == ']') {
      fprintf(stderr, "Error: This is the worst scene file EVER.\n");
      fclose(json);
      return;
    }
    if (c == '{') {
      skip_ws(json);

      // Parse the object
      char* key = next_string(json);
      if (strcmp(key, "type") != 0) {
        fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
        exit(1);
      }

      skip_ws(json);

      expect_c(json, ':');

      skip_ws(json);

      char* value = next_string(json);

      if (strcmp(value, "camera") == 0) {
        // Do nothing, camera isn't an object in the scene.
        index--;
      } else if (strcmp(value, "sphere") == 0) {
        objects[index].type = "sphere";
      } else if (strcmp(value, "plane") == 0) {
        objects[index].type = "plane";
      } else {
        fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
        exit(1);
      }

      skip_ws(json);

      while (1) {
    // , }
    c = next_c(json);
    if (c == '}') {
      // stop parsing this object
      break;
    } else if (c == ',') {
      // read another field
      skip_ws(json);
      char* key = next_string(json);
      skip_ws(json);
      expect_c(json, ':');
      skip_ws(json);
      if (strcmp(key, "width") == 0) {
        cameraWidth = next_number(json);
      }
      else if (strcmp(key, "height") == 0) {
        cameraHeight = next_number(json);
      }
      else if (strcmp(key, "radius") == 0) {
        objects[index].radius = next_number(json);
      }
      else if (strcmp(key, "color") == 0) {
        objects[index].color = next_vector(json);
      }
      else if (strcmp(key, "position") == 0) {
        objects[index].position = next_vector(json);
      }
      else if (strcmp(key, "normal") == 0) {
        objects[index].normal = next_vector(json);
      }
      else {
        fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
          key, line);
        //char* value = next_string(json);
      }
      skip_ws(json);
    } else {
      fprintf(stderr, "Error: Unexpected value on line %d\n", line);
      exit(1);
    }
      }
      skip_ws(json);
      c = next_c(json);
      if (c == ',') {
  // noop
  skip_ws(json);
      } else if (c == ']') {
  fclose(json);
  return;
      } else {
        fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
        exit(1);
      }
    }
  }
}


void raycast() {
  //TODO
}


void write_scene(char *filename, int format) {

  FILE *ppm = fopen(filename, "wb");
  if (!ppm) {
    fprintf(stderr, "Error opening file\n");
    exit(1);
  }

  //header
  if (format == 6) {
    fprintf(ppm, "P6\n");
  }
  else if (format == 3) {
    fprintf(ppm, "P3\n");
  }

  fprintf(ppm, "# Created by %s\n", AUTHOR);
  fprintf(ppm, "%d %d\n", Width, Height);
  fprintf(ppm, "%d\n", RGB_NUMBER);


  //image data
  int index;
  if (format == 6) {
    for (index = 0; index < Width * Height; index++) {


      int color = 0; // color will be set into some type of array during raycast, ie raycast[index][0];
      fwrite(&color, 1, 1, ppm); //red
      color = 255;
      fwrite(&color, 1, 1, ppm); //green
      color = 0;
      fwrite(&color, 1, 1, ppm); //blue
    }
  }
  else if (format == 3) {
    for (index = 0; index < Width * Height; index++) {

      int color = 0;
      fprintf(ppm, "%d\n", color);
      color = 0;
      fprintf(ppm, "%d\n", color);
      color = 255;
      fprintf(ppm, "%d\n", color);
    }
  }
  fclose(ppm);
}

int main(int argc, char** argv) {

  if (argc != 5){
        printf("usage: raycast width height input.json output.ppm");
        return(1);
    }

    Width = atoi(argv[1]);
    Height = atoi(argv[2]);

    read_scene(argv[3]);
    //raycast
    write_scene(argv[4], 3);
    return 0;
}