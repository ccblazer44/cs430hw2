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
double **viewPlane;





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
  int i;
  int j;
  int index = 0;
  int objectIndex;
  //loop through all pixels
  for(i = 0; i < Width; i++){
    for(j = 0; j < Height; j++){

      
      double x, y, z = 1; // z is always 1 because the view plane is 1 unit away from camera

      x = 0 - (cameraWidth/2) + ((cameraWidth/Width)*(j + 0.5));
      y = 0 - (cameraHeight/2) + ((cameraHeight/Height)*(i + 0.5));

      printf("Vector x and y: %f %f\n", x, y);

      // calculate magnitude using "distance formula"
      double magnitude = sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));

      // replace vector with unit vector
      x = x/magnitude;
      y = y/magnitude;
      z = z/magnitude; 

      // set min so that close objects display over further ones
      double min = 100000;

      //printf("before while\n");
      objectIndex = 0;

      while(objects[objectIndex].color != NULL){

        

        if(strcmp(objects[objectIndex].type, "sphere") == 0){

          
          //use unit vector to calculate collision
          double t = (((x * objects[objectIndex].position[0]) + (y * objects[objectIndex].position[1]) + (z * objects[objectIndex].position[2]))/(pow(x, 2) + pow(y, 2) + pow(z, 2)));



          //find point on vector closest to center of sphere
          double tCloseX = x * t;
          double tCloseY = y * t;
          double tCloseZ = z * t;
          double d = sqrt(pow((tCloseX - objects[objectIndex].position[0]), 2) + pow((tCloseY - objects[objectIndex].position[1]), 2) + pow((tCloseZ - objects[objectIndex].position[2]), 2));

          //printf("d radius: %f %f\n", d, objects[objectIndex].radius);

          if(d <= objects[objectIndex].radius){


            //find distance from camera to actual intersection point and set it to t
            double a = sqrt(pow(objects[objectIndex].radius, 2) - pow(d, 2));
            t = t - a;

            //printf("t is: %f\n", t);
            //set new min so that close objects display over further ones
            if (min > t){
              printf("adding something");
              min = t;
              viewPlane[index] = objects[objectIndex].color;
            }

          }

        }
        else if(strcmp(objects[objectIndex].type, "plane") == 0){

        }

        // printf("while %d\n", index);
        // printf("object index %d\n", objectIndex);
        objectIndex++;

      }


      //printf("after while\n");
      index++;
    }
  }
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

      //printf("%d\n", index);
      int color = (int) (viewPlane[index][0] * 255); // color will be set into some type of array during raycast, ie raycast[index][0];
      fwrite(&color, 1, 1, ppm); //red
      color = (int) viewPlane[index][1] * 255;
      fwrite(&color, 1, 1, ppm); //green
      color = (int) viewPlane[index][2] * 255;
      fwrite(&color, 1, 1, ppm); //blue
    }
  }
  else if (format == 3) {
    for (index = 0; index < Width * Height; index++) {

      int color = viewPlane[index][0];
      fprintf(ppm, "%d\n", color);
      color = viewPlane[index][1];
      fprintf(ppm, "%d\n", color);
      color = viewPlane[index][2];
      fprintf(ppm, "%d\n", color);
    }
  }
  fclose(ppm);
}

void initializeViewPlane() {
  double black[3] = {0,0,0};
  int row, column;
  for (row = 0; row < Width; row++) {
    for (column = 0; column < Height; column++) {
      viewPlane[row * Height + column] = black;
    }
  }
}

int main(int argc, char** argv) {

  if (argc != 5){
        printf("usage: raycast width height input.json output.ppm");
        return(1);
    }

    Width = atoi(argv[1]);
    Height = atoi(argv[2]);

    read_scene(argv[3]);

    viewPlane = (double **)malloc(Width * Height * 3 * sizeof(double));
    initializeViewPlane();
    raycast();
    write_scene(argv[4], 6);
    return 0;
}