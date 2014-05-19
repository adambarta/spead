#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sysexits.h>

#define DEG_TO_RAD        0.017453292519943295769236907684886
#define EARTH_RADIUS_M    6372797.560856

struct pos {
  double lat;
  double lng;
};

int arc_in_rad(const struct pos *from, const struct pos *to, double *dist)
{
  double lat_arc, lng_arc, lat_h, lng_h;
  
  if (from == NULL || to == NULL || dist == NULL)
    return -1;

  lat_arc = (from->lat - to->lat) * DEG_TO_RAD;
  lng_arc = (from->lng - to->lng) * DEG_TO_RAD;

  lat_h = sin(lat_arc * 0.5f);
  lat_h *= lat_h;

  lng_h = sin(lng_arc * 0.5f);
  lng_h = lng_h;

  *dist = 2.0f * asin(sqrt(lat_h + cos(from->lat * DEG_TO_RAD) * cos(to->lat * DEG_TO_RAD) * lng_h))*EARTH_RADIUS_M;

  return 0;
}


int main(int argc, char * argv[])
{
  struct pos *from;
  struct pos *to;

  double dist;

  from = malloc(sizeof(struct pos));
  if (from == NULL) {
    return EX_SOFTWARE;
  }

  to = malloc(sizeof(struct pos));
  if (to == NULL) {
    free(from);
    return EX_SOFTWARE;
  }

  
  from->lat = 51.52297369999999;
  from->lng = -0.1400877999999465;

  to->lat = 51.22297369999999;
  to->lng = -0.1400877999999465;

  if (arc_in_rad(from, to, &dist) < 0){
    goto freeme;
  }

  fprintf(stderr, "DISt: %f\n", dist);

freeme:
  free(from);
  free(to);

  return 0;
}
