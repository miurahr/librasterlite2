/* 
/ wmslite
/
/ a light-weight WMS server supporting RasterLite2 DataSources
/
/ version 1.0, 2014 January 29
/
/ Author: Sandro Furieri a.furieri@lqt.it
/
/ Copyright (C) 2014  Alessandro Furieri
/
/    This program is free software: you can redistribute it and/or modify
/    it under the terms of the GNU General Public License as published by
/    the Free Software Foundation, either version 3 of the License, or
/    (at your option) any later version.
/
/    This program is distributed in the hope that it will be useful,
/    but WITHOUT ANY WARRANTY; without even the implied warranty of
/    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/    GNU General Public License for more details.
/
/    You should have received a copy of the GNU General Public License
/    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>


#ifdef _WIN32
/* This code is for win32 only */
#include <windows.h>
#include <process.h>
#else
/* this code is for any sane minded system (*nix) */
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#endif

#include <rasterlite2/rasterlite2.h>
#include <spatialite.h>

#define ARG_NONE		0
#define ARG_DB_PATH		1
#define ARG_IP_PORT		2
#define ARG_CACHE_SIZE		3

#define WMS_ILLEGAL_REQUEST	0
#define WMS_GET_CAPABILITIES	1
#define WMS_GET_MAP		2

#define WMS_UNKNOWN		-1
#define WMS_TRANSPARENT		10
#define WMS_OPAQUE		11
#define WMS_PNG			12
#define WMS_JPEG		13

#define WMS_INVALID_CRS		101
#define WMS_INVALID_DIMENSION	102
#define WMS_INVALID_BBOX	103
#define WMS_INVALID_LAYER	104
#define WMS_INVALID_BGCOLOR	105
#define WMS_INVALID_STYLE	106
#define WMS_INVALID_FORMAT	107
#define WMS_INVALID_TRANSPARENT	108
#define WMS_NOT_EXISTING_LAYER	109
#define WMS_LAYER_OUT_OF_BBOX	110
#define WMS_MISMATCHING_SRID	111

struct neutral_socket
{
#ifdef _WIN32
    SOCKET socket;
#else
    int socket;
#endif
};

struct wms_layer
{
/* a struct wrapping a WMS layer */
    char *layer_name;
    char *title;
    char *abstract;
    int srid;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double geo_minx;
    double geo_miny;
    double geo_maxx;
    double geo_maxy;
    unsigned char sample;
    unsigned char pixel;
    unsigned char num_bands;
    int jpeg;
    int png;
    struct wms_layer *next;
};

struct wms_list
{
/* a struct wrapping a list of WMS layers */
    struct wms_layer *first;
    struct wms_layer *last;
};

struct wms_argument
{
/* a struct wrapping a single WMS arg */
    char *arg_name;
    char *arg_value;
    struct wms_argument *next;
};

struct wms_args
{
/* a struct wrapping a WMS request URL */
    sqlite3 *db_handle;
    char *service_name;
    struct wms_argument *first;
    struct wms_argument *last;
    int request_type;
    int error;
    const char *layer;
    double minx;
    double miny;
    double maxx;
    double maxy;
    unsigned short width;
    unsigned short height;
    char *style;
    unsigned char format;
    int transparent;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

struct http_request
{
/* a struct wrapping an HTTP request */
    unsigned int id;		/* request ID */
    int port_no;
#ifdef _WIN32
    SOCKET socket;		/* Socket on which to receive data */
    SOCKADDR_IN addr;		/* Address from which data is coming */
#else
    int socket;			/* Socket on which to receive data */
    struct sockaddr addr;	/* Address from which data is coming */
#endif
    struct wms_list *list;
    sqlite3 *db_handle;
};

static struct wms_layer *
alloc_wms_layer (const char *layer, const char *title, const char *abstract,
		 int srid, double minx, double miny, double maxx, double maxy,
		 unsigned char sample, unsigned char pixel,
		 unsigned char num_bands)
{
/* creating a WMS layer item */
    int len;
    struct wms_layer *lyr = malloc (sizeof (struct wms_layer));
    len = strlen (layer);
    lyr->layer_name = malloc (len + 1);
    strcpy (lyr->layer_name, layer);
    len = strlen (title);
    lyr->title = malloc (len + 1);
    strcpy (lyr->title, title);
    len = strlen (abstract);
    lyr->abstract = malloc (len + 1);
    strcpy (lyr->abstract, abstract);
    lyr->srid = srid;
    lyr->minx = minx;
    lyr->miny = miny;
    lyr->maxx = maxx;
    lyr->maxy = maxy;
    lyr->sample = sample;
    lyr->pixel = pixel;
    lyr->num_bands = num_bands;
    if (pixel == RL2_PIXEL_MONOCHROME || pixel == RL2_PIXEL_PALETTE)
      {
	  lyr->png = 1;
	  lyr->jpeg = 0;
      }
    else
      {
	  lyr->png = 1;
	  lyr->jpeg = 1;
      }
    lyr->next = NULL;
    return lyr;
}

static void
destroy_wms_layer (struct wms_layer *lyr)
{
/* memory cleanup - freeing a WMS layer item */
    if (lyr == NULL)
	return;
    if (lyr->layer_name != NULL)
	free (lyr->layer_name);
    if (lyr->title != NULL)
	free (lyr->title);
    if (lyr->abstract != NULL)
	free (lyr->abstract);
    free (lyr);
}

static struct wms_list *
alloc_wms_list ()
{
/* allocating a list of WMS layers */
    struct wms_list *list = malloc (sizeof (struct wms_list));
    list->first = NULL;
    list->last = NULL;
    return list;
}

static void
destroy_wms_list (struct wms_list *list)
{
/* memory cleanup - destroying a list of WMS layers */
    struct wms_layer *pl;
    struct wms_layer *pln;
    if (list == NULL)
	return;
    pl = list->first;
    while (pl != NULL)
      {
	  pln = pl->next;
	  destroy_wms_layer (pl);
	  pl = pln;
      }
    free (list);
}

static struct wms_argument *
alloc_wms_argument (char *name, char *value)
{
/* allocating a WMS argument */
    struct wms_argument *arg;
    arg = malloc (sizeof (struct wms_argument));
    arg->arg_name = name;
    arg->arg_value = value;
    arg->next = NULL;
    return arg;
}

static void
destroy_wms_argument (struct wms_argument *arg)
{
/* memory cleanup - destroying a WMS arg struct */
    if (arg == NULL)
	return;
    if (arg->arg_name != NULL)
	free (arg->arg_name);
    if (arg->arg_value != NULL)
	free (arg->arg_value);
    free (arg);
}

static struct wms_args *
alloc_wms_args (const char *service_name)
{
/* allocating an empty WMS args struct */
    int len;
    struct wms_args *args;
    if (service_name == NULL)
	return NULL;
    args = malloc (sizeof (struct wms_args));
    args->db_handle = NULL;
    len = strlen (service_name);
    args->service_name = malloc (len + 1);
    strcpy (args->service_name, service_name);
    args->first = NULL;
    args->last = NULL;
    args->request_type = WMS_ILLEGAL_REQUEST;
    args->error = WMS_UNKNOWN;
    return args;
}

static void
destroy_wms_args (struct wms_args *args)
{
/* memory cleanup - destroying a WMS args struct */
    struct wms_argument *pa;
    struct wms_argument *pan;
    if (args == NULL)
	return;
    if (args->service_name != NULL)
	free (args->service_name);
    pa = args->first;
    while (pa != NULL)
      {
	  pan = pa->next;
	  destroy_wms_argument (pa);
	  pa = pan;
      }
    free (args);
}

static int
add_wms_argument (struct wms_args *args, const char *token)
{
/* attempting to add a WMS argument */
    int len;
    struct wms_argument *arg;
    char *name;
    char *value;
    const char *ptr = strstr (token, "=");
    if (ptr == NULL)
	return 0;
    len = strlen (ptr + 1);
    value = malloc (len + 1);
    strcpy (value, ptr + 1);
    len = ptr - token;
    name = malloc (len + 1);
    memcpy (name, token, len);
    *(name + len) = '\0';
    arg = alloc_wms_argument (name, value);
    if (args->first == NULL)
	args->first = arg;
    if (args->last != NULL)
	args->last->next = arg;
    args->last = arg;
    return 1;
}

static int
parse_srs (const char *srs)
{
/* parsing the EPSG:x item */
    int srid;
    if (strlen (srs) < 6)
	return -1;
    if (strncmp (srs, "EPSG:", 4) != 0)
	return -1;
    srid = atoi (srs + 5);
    return srid;
}

static const char *
parse_layers (const char *layers)
{
/* only a single layer for each request is supported */
    int comma = 0;
    const char *p = layers;
    while (*p != '\0')
      {
	  if (*p++ == ',')
	      comma++;
      }
    if (!comma)
	return layers;
    return NULL;
}

static int
parse_dim (const char *dim, unsigned short *value)
{
/* parsing the Width / Height items */
    int x;
    for (x = 0; x < (int) strlen (dim); x++)
      {
	  if (*(dim + x) < '0' || *(dim + x) > '9')
	      return 0;
      }
    x = atoi (dim);
    if (x <= 0 || x > UINT16_MAX)
	return 0;
    *value = x;
    return 1;
}

static unsigned char *
parse_style (const char *style)
{
/* the unique and only supported style is DEFAULT */
    char *out_style = NULL;
    if (strlen (style) == 0)
	style = "default";
    if (strcasecmp (style, "default") == 0)
      {
	  out_style = malloc (strlen (style) + 1);
	  strcpy (out_style, style);
      }
    return out_style;
}

static int
parse_format (const char *format)
{
/* parsing the output format */
    if (strcasecmp (format, "image/png") == 0)
	return WMS_PNG;
    if (strcasecmp (format, "image/jpeg") == 0)
	return WMS_JPEG;
    return WMS_UNKNOWN;
}

static int
parse_transparent (const char *str)
{
/* parsing the Transparent value */
    if (strcasecmp (str, "TRUE") == 0)
	return WMS_TRANSPARENT;
    if (strcasecmp (str, "FALSE") == 0)
	return WMS_OPAQUE;
    return WMS_UNKNOWN;
}

static int
parse_bbox (const char *bbox, double *minx, double *miny, double *maxx,
	    double *maxy)
{
/* attempting to parse the BBOX */
    char buf[2000];
    int count = 0;
    char *out = buf;
    const char *in = bbox;
    while (1)
      {
	  if (*in == '\0')
	    {
		if (count == 3)
		  {
		      *maxy = atof (buf);
		  }
		else
		    return 0;
		break;
	    }
	  if (*in == ',')
	    {
		*out = '\0';
		switch (count)
		  {
		  case 0:
		      *minx = atof (buf);
		      break;
		  case 1:
		      *miny = atof (buf);
		      break;
		  case 2:
		      *maxx = atof (buf);
		      break;
		  default:
		      return 0;
		  };
		out = buf;
		in++;
		count++;
		continue;
	    }
	  *out++ = *in++;
      }
    return 1;
}

static int
parse_hex (char hi, char lo, unsigned char *value)
{
/* parsing an Hex byte */
    unsigned char x;
    switch (hi)
      {
      case '0':
	  x = 0;
	  break;
      case '1':
	  x = 1 * 16;
	  break;
      case '2':
	  x = 2 * 16;
	  break;
      case '3':
	  x = 3 * 16;
	  break;
      case '4':
	  x = 4 * 16;
	  break;
      case '5':
	  x = 5 * 16;
	  break;
      case '6':
	  x = 6 * 16;
	  break;
      case '7':
	  x = 7 * 16;
	  break;
      case '8':
	  x = 8 * 16;
	  break;
      case '9':
	  x = 9 * 16;
	  break;
      case 'a':
      case 'A':
	  x = 10 * 16;
	  break;
      case 'b':
      case 'B':
	  x = 11 * 16;
	  break;
      case 'c':
      case 'C':
	  x = 12 * 16;
	  break;
      case 'd':
      case 'D':
	  x = 13 * 16;
	  break;
      case 'e':
      case 'E':
	  x = 14 * 16;
	  break;
      case 'f':
      case 'F':
	  x = 15 * 16;
	  break;
      default:
	  return 0;
      };
    switch (lo)
      {
      case '0':
	  x += 0;
	  break;
      case '1':
	  x += 1;
	  break;
      case '2':
	  x += 2;
	  break;
      case '3':
	  x += 3;
	  break;
      case '4':
	  x += 4;
	  break;
      case '5':
	  x += 5;
	  break;
      case '6':
	  x += 6;
	  break;
      case '7':
	  x += 7;
	  break;
      case '8':
	  x += 8;
	  break;
      case '9':
	  x += 9;
	  break;
      case 'a':
      case 'A':
	  x += 10;
	  break;
      case 'b':
      case 'B':
	  x += 11;
	  break;
      case 'c':
      case 'C':
	  x += 12;
	  break;
      case 'd':
      case 'D':
	  x += 13;
	  break;
      case 'e':
      case 'E':
	  x += 14;
	  break;
      case 'f':
      case 'F':
	  x += 15;
	  break;
      default:
	  return 0;
      };
    *value = x;
    return 1;
}

static int
parse_bgcolor (const char *bgcolor, unsigned char *red, unsigned char *green,
	       unsigned char *blue)
{
/* attempting to parse an RGB color */
    if (strlen (bgcolor) != 8)
	return 0;
    if (bgcolor[0] != '0')
	return 0;
    if (bgcolor[1] == 'x' || bgcolor[1] == 'X')
	;
    else
	return 0;
    if (!parse_hex (bgcolor[2], bgcolor[3], red))
	return 0;
    if (!parse_hex (bgcolor[4], bgcolor[5], green))
	return 0;
    if (!parse_hex (bgcolor[6], bgcolor[7], blue))
	return 0;
    return 1;
}

static int
exists_layer (struct wms_list *list, const char *layer, int srid, double minx,
	      double miny, double maxx, double maxy)
{
/* checking a required layer for validity */
    struct wms_layer *lyr = list->first;
    while (lyr != NULL)
      {
	  if (strcmp (lyr->layer_name, layer) == 0)
	    {
		if (lyr->srid != srid)
		    return WMS_MISMATCHING_SRID;
		if (lyr->minx > maxx)
		    return WMS_LAYER_OUT_OF_BBOX;
		if (lyr->maxx < minx)
		    return WMS_LAYER_OUT_OF_BBOX;
		if (lyr->miny > maxy)
		    return WMS_LAYER_OUT_OF_BBOX;
		if (lyr->maxy < miny)
		    return WMS_LAYER_OUT_OF_BBOX;
		return 0;
	    }
	  lyr = lyr->next;
      }
    return WMS_NOT_EXISTING_LAYER;
}

static int
check_wms_request (struct wms_list *list, struct wms_args *args)
{
/* checking for a valid WMS request */
    struct wms_argument *arg;
    int ok_wms = 0;
    int ok_version = 0;
    int is_get_capabilities = 0;
    int is_get_map = 0;
    const char *p_layers = NULL;
    const char *p_srs = NULL;
    const char *p_bbox = NULL;
    const char *p_width = NULL;
    const char *p_height = NULL;
    const char *p_styles = NULL;
    const char *p_format = NULL;
    const char *p_bgcolor = NULL;
    const char *p_transparent = NULL;

    if (strcasecmp (args->service_name, "GET /wmslite") != 0)
	return 400;
    arg = args->first;
    while (arg != NULL)
      {
	  if (strcasecmp (arg->arg_name, "SERVICE") == 0)
	    {
		if (strcasecmp (arg->arg_value, "WMS") == 0)
		    ok_wms = 1;
	    }
	  if (strcasecmp (arg->arg_name, "VERSION") == 0)
	    {
		if (strcasecmp (arg->arg_value, "1.3.0") == 0)
		    ok_version = 1;
		if (strcasecmp (arg->arg_value, "1.1.1") == 0)
		    ok_version = 1;
		if (strcasecmp (arg->arg_value, "1.1.0") == 0)
		    ok_version = 1;
		if (strcasecmp (arg->arg_value, "1.0.0") == 0)
		    ok_version = 1;
	    }
	  if (strcasecmp (arg->arg_name, "REQUEST") == 0)
	    {
		if (strcasecmp (arg->arg_value, "GetCapabilities") == 0)
		    is_get_capabilities = 1;
		if (strcasecmp (arg->arg_value, "GetMap") == 0)
		    is_get_map = 1;
	    }
	  if (strcasecmp (arg->arg_name, "LAYERS") == 0)
	      p_layers = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "SRS") == 0
	      || strcasecmp (arg->arg_name, "CRS") == 0)
	      p_srs = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "BBOX") == 0)
	      p_bbox = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "WIDTH") == 0)
	      p_width = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "HEIGHT") == 0)
	      p_height = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "STYLES") == 0)
	      p_styles = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "FORMAT") == 0)
	      p_format = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "BGCOLOR") == 0)
	      p_bgcolor = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "TRANSPARENT") == 0)
	      p_transparent = arg->arg_value;
	  arg = arg->next;
      }
    if (is_get_capabilities && !is_get_map)
      {
	  /* testing for valid GetCapabilities */
	  if (ok_wms)
	    {
		args->request_type = WMS_GET_CAPABILITIES;
		return 200;
	    }
      }
    if (is_get_map && !is_get_capabilities)
      {
	  /* testing for valid GetMap */
	  if (ok_wms && ok_version && p_layers && p_srs && p_bbox && p_width
	      && p_height && p_styles && p_format)
	    {
		int ret;
		int srid;
		const char *layer;
		double minx;
		double miny;
		double maxx;
		double maxy;
		unsigned short width;
		unsigned short height;
		char *style = NULL;
		int format;
		int transparent = WMS_OPAQUE;
		unsigned char red = 255;
		unsigned char green = 255;
		unsigned char blue = 255;
		srid = parse_srs (p_srs);
		if (srid <= 0)
		  {
		      args->error = WMS_INVALID_CRS;
		      return 200;
		  }
		layer = parse_layers (p_layers);
		if (layer == NULL)
		  {
		      args->error = WMS_INVALID_LAYER;
		      return 200;
		  }
		if (!parse_bbox (p_bbox, &minx, &miny, &maxx, &maxy))
		  {
		      args->error = WMS_INVALID_BBOX;
		      return 200;
		  }
		if (!parse_dim (p_width, &width))
		  {
		      args->error = WMS_INVALID_DIMENSION;
		      return 200;
		  }
		if (!parse_dim (p_height, &height))
		  {
		      args->error = WMS_INVALID_DIMENSION;
		      return 200;
		  }
		style = parse_style (p_styles);
		if (style == NULL)
		  {
		      args->error = WMS_INVALID_STYLE;
		      return 200;
		  }
		format = parse_format (p_format);
		if (format == WMS_UNKNOWN)
		  {
		      args->error = WMS_INVALID_FORMAT;
		      return 200;
		  }
		if (p_transparent != NULL)
		  {
		      transparent = parse_transparent (p_transparent);
		      if (transparent == WMS_UNKNOWN)
			{
			    args->error = WMS_INVALID_TRANSPARENT;
			    return 200;
			}
		  }
		if (p_bgcolor != NULL)
		  {
		      if (!parse_bgcolor (p_bgcolor, &red, &green, &blue))
			{
			    args->error = WMS_INVALID_BGCOLOR;
			    return 200;
			}
		  }
		ret = exists_layer (list, layer, srid, minx, miny, maxx, maxy);
		if (ret == WMS_NOT_EXISTING_LAYER
		    || ret == WMS_LAYER_OUT_OF_BBOX
		    || ret == WMS_MISMATCHING_SRID)
		  {
		      args->error = ret;
		      return 200;
		  }
		args->request_type = WMS_GET_MAP;
		args->layer = layer;
		args->minx = minx;
		args->miny = miny;
		args->maxx = maxx;
		args->maxy = maxy;
		args->width = width;
		args->height = height;
		args->style = style;
		args->format = format;
		args->transparent = transparent;
		args->red = red;
		args->green = green;
		args->blue = blue;
	    }
      }
    return 200;
}

static struct wms_args *
parse_http_request (const char *http_hdr)
{
/* attempting to parse an HTTP Request */
    struct wms_args *args = NULL;
    char token[2000];
    char *out;
    const char *p;
    const char *start = strstr (http_hdr, "GET ");
    const char *end = NULL;
    if (start == NULL)
	return NULL;
    end = strstr (start, " HTTP/1.1");
    if (end == NULL)
	end = strstr (start, " HTTP/1.0");
    if (end == NULL)
	return NULL;

    p = start;
    out = token;
    while (p < end)
      {
	  if (*p == '?')
	    {
		/* the service name */
		*out = '\0';
		if (args != NULL)
		    goto error;
		args = alloc_wms_args (token);
		out = token;
		p++;
		continue;
	    }
	  if (*p == '&')
	    {
		/* a key-value pair ends here */
		*out = '\0';
		if (args == NULL)
		    goto error;
		if (!add_wms_argument (args, token))
		    goto error;
		out = token;
		p++;
		continue;
	    }
	  *out++ = *p++;
      }
    if (out > token)
      {
	  /* processing the last arg */
	  *out = '\0';
	  if (args == NULL)
	      goto error;
	  if (!add_wms_argument (args, token))
	      goto error;
      }
    return args;

  error:
    if (args != NULL)
	destroy_wms_args (args);
    return NULL;
}

static unsigned char *
get_current_timestamp ()
{
/* formatting the current timestamp */
    char *dummy;
    struct tm *xtm;
    time_t now;
    const char *day;
    const char *month;
    time (&now);
    xtm = gmtime (&now);
    switch (xtm->tm_wday)
      {
      case 0:
	  day = "Sun";
	  break;
      case 1:
	  day = "Mon";
	  break;
      case 2:
	  day = "Tue";
	  break;
      case 3:
	  day = "Wed";
	  break;
      case 4:
	  day = "Thu";
	  break;
      case 5:
	  day = "Fri";
	  break;
      case 6:
	  day = "Sat";
	  break;
      };
    switch (xtm->tm_mon)
      {
      case 0:
	  month = "Jan";
	  break;
      case 1:
	  month = "Feb";
	  break;
      case 2:
	  month = "Mar";
	  break;
      case 3:
	  month = "Apr";
	  break;
      case 4:
	  month = "May";
	  break;
      case 5:
	  month = "Jun";
	  break;
      case 6:
	  month = "Jul";
	  break;
      case 7:
	  month = "Aug";
	  break;
      case 8:
	  month = "Sep";
	  break;
      case 9:
	  month = "Oct";
	  break;
      case 10:
	  month = "Nov";
	  break;
      case 11:
	  month = "Jan";
	  break;
      };
    dummy =
	sqlite3_mprintf ("Date: %s, %02d %s %04d %02d:%02d:%02d GMT\r\n", day,
			 xtm->tm_mday, month, xtm->tm_year + 1900, xtm->tm_hour,
			 xtm->tm_min, xtm->tm_sec);
    return dummy;
}

static void
build_wms_exception (struct wms_args *args, gaiaOutBufferPtr xml_response)
{
/* preparing an XML exception */
    char *dummy;
    gaiaOutBuffer xml_text;
    gaiaOutBufferInitialize (&xml_text);
    gaiaAppendToOutBuffer (&xml_text,
			   "<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<ServiceExceptionReport version=\"1.3.0\" ");
    gaiaAppendToOutBuffer (&xml_text, "xmlns=\"http://www.opengis.net/ogc\" ");
    gaiaAppendToOutBuffer (&xml_text,
			   "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
    gaiaAppendToOutBuffer (&xml_text,
			   "xsi:schemaLocation=\"http://www.opengis.net/ogc ");
    gaiaAppendToOutBuffer (&xml_text,
			   "http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">\r\n");
    if (args == NULL)
      {
	  gaiaAppendToOutBuffer (&xml_text, "<ServiceException>\r\n");
	  gaiaAppendToOutBuffer (&xml_text, "General error.\r\n");
      }
    else
      {
	  switch (args->error)
	    {
	    case WMS_INVALID_DIMENSION:
		gaiaAppendToOutBuffer (&xml_text,
				       "<ServiceException code=\"InvalidDimensionValue\">\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Request is for a Layer not offered by the service instance.\r\n");
		break;
	    case WMS_INVALID_BBOX:
		gaiaAppendToOutBuffer (&xml_text, "<ServiceException>\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Invalid BBOX parameter.\r\n");
		break;
	    case WMS_NOT_EXISTING_LAYER:
	    case WMS_INVALID_LAYER:
		gaiaAppendToOutBuffer (&xml_text,
				       "<ServiceException code=\"LayerNotDefined\">\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Request is for a Layer not offered by the service instance.\r\n");
		break;
	    case WMS_INVALID_BGCOLOR:
		gaiaAppendToOutBuffer (&xml_text, "<ServiceException>\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Invalid BGCOLOR parameter.\r\n");
		break;
	    case WMS_INVALID_STYLE:
		gaiaAppendToOutBuffer (&xml_text,
				       "<ServiceException code=\"StyleNotDefined\">\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Request is for a Layer in a Style not offered by the service instance.\r\n");
		break;
	    case WMS_INVALID_FORMAT:
		gaiaAppendToOutBuffer (&xml_text,
				       "<ServiceException code=\"InvalidFormat\">\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Request contains a Format not offered by the service instance.\r\n");
		break;
	    case WMS_INVALID_TRANSPARENT:
		gaiaAppendToOutBuffer (&xml_text, "<ServiceException>\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Invalid TRANSPARENT parameter.\r\n");
		break;
	    case WMS_LAYER_OUT_OF_BBOX:
		gaiaAppendToOutBuffer (&xml_text, "<ServiceException>\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "The BBOX parameter is outside the layer's extent.\r\n");
		break;
	    case WMS_INVALID_CRS:
		gaiaAppendToOutBuffer (&xml_text,
				       "<ServiceException code=\"InvalidSRS\">\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Request contains a malformed SRS.\r\n");
		break;
	    case WMS_MISMATCHING_SRID:
		gaiaAppendToOutBuffer (&xml_text,
				       "<ServiceException code=\"InvalidSRS\">\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Request contains an SRS not offered by the service ");
		gaiaAppendToOutBuffer (&xml_text,
				       "instance for one or more of the Layers in the request.\r\n");
		break;
	    default:
		gaiaAppendToOutBuffer (&xml_text, "<ServiceException>\r\n");
		gaiaAppendToOutBuffer (&xml_text, "Malformed request.\r\n");
		break;
	    }
      }
    gaiaAppendToOutBuffer (&xml_text,
			   "</ServiceException>\r\n</ServiceExceptionReport>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "");
    gaiaAppendToOutBuffer (xml_response, "HTTP/1.1 200 OK\r\n");
    dummy = get_current_timestamp ();
    gaiaAppendToOutBuffer (xml_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (xml_response,
			   "Content-Type: text/xml;charset=UTF-8\r\n");
    dummy = sqlite3_mprintf ("Content-Length: %d\r\n", xml_text.WriteOffset);
    gaiaAppendToOutBuffer (xml_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (xml_response, "Connection: close\r\n\r\n");
    gaiaAppendToOutBuffer (xml_response, xml_text.Buffer);
    gaiaOutBufferReset (&xml_text);
}

static void
build_http_error (int http_status, gaiaOutBufferPtr xml_response, int port_no)
{
/* preparing an HTTP error */
    char *dummy;
    gaiaOutBuffer http_text;
    gaiaOutBufferInitialize (&http_text);
    gaiaAppendToOutBuffer (&http_text,
			   "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n");
    gaiaAppendToOutBuffer (&http_text, "<html><head>\r\n");
    if (http_status == 400)
      {
	  gaiaAppendToOutBuffer (xml_response, "HTTP/1.1 400 Bad Request\r\n");
	  gaiaAppendToOutBuffer (&http_text,
				 "<title>400 Bad Request</title>\r\n");
	  gaiaAppendToOutBuffer (&http_text, "</head><body>\r\n");
	  gaiaAppendToOutBuffer (&http_text, "<h1>Bad Request</h1>\n");
      }
    else
      {
	  gaiaAppendToOutBuffer (xml_response,
				 "HTTP/1.1 500 Internal Server Error\r\n");
	  gaiaAppendToOutBuffer (&http_text,
				 "<title>500 Internal Server Error</title>\r\n");
	  gaiaAppendToOutBuffer (&http_text, "</head><body>\r\n");
	  gaiaAppendToOutBuffer (&http_text,
				 "<h1>Internal Server Error</h1>\n");
      }
    dummy =
	sqlite3_mprintf
	("<address>WmsLite/%s [%s] at localhost (127.0.0.1) Port %d</address>\r\n",
	 rl2_version (), rl2_target_cpu (), port_no);
    gaiaAppendToOutBuffer (&http_text, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (&http_text, "</body></html>\r\n");
    gaiaAppendToOutBuffer (&http_text, "");
    dummy = get_current_timestamp ();
    gaiaAppendToOutBuffer (xml_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (xml_response,
			   "Content-Type: text/html;charset=UTF-8\r\n");
    dummy = sqlite3_mprintf ("Content-Length: %d\r\n", http_text.WriteOffset);
    gaiaAppendToOutBuffer (xml_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (xml_response, "Connection: close\r\n\r\n");
    gaiaAppendToOutBuffer (xml_response, http_text.Buffer);
    gaiaOutBufferReset (&http_text);
}

static void
wms_get_capabilities (struct wms_list *list, gaiaOutBufferPtr xml_response)
{
/* preparing the WMS GetCapabilities XML document */
    struct wms_layer *lyr;
    char *dummy;
    gaiaOutBuffer xml_text;
    gaiaOutBufferInitialize (&xml_text);
    gaiaAppendToOutBuffer (&xml_text,
			   "<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<WMS_Capabilities version=\"1.3.0\" ");
    gaiaAppendToOutBuffer (&xml_text, "xmlns=\"http://www.opengis.net/wms\" ");
    gaiaAppendToOutBuffer (&xml_text,
			   "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
    gaiaAppendToOutBuffer (&xml_text,
			   "xsi:schemaLocation=\"http://www.opengis.net/wms ");
    gaiaAppendToOutBuffer (&xml_text,
			   "http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd\">\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Service>\r\n<Name>WMS</Name>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Title>WmsLite test server</Title>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Abstract>A simple light-weight WMS server for testing RasterLite2 Coverages.</Abstract>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<KeywordList>\r\n<Keyword>maps</Keyword>\r\n</KeywordList>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<OnlineResource xlink:href=\"http://127.0.0.1:8080/wmslite?\" ");
    gaiaAppendToOutBuffer (&xml_text,
			   "xlink:type=\"simple\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"/>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<ContactInformation>\r\n<ContactPersonPrimary>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<ContactPerson>James T. Kirk</ContactPerson>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<ContactOrganization>United Federation of Planets, Starfleet</ContactOrganization>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "</ContactPersonPrimary>\r\n<ContactPosition>Starship Captain.</ContactPosition>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<ContactAddress>\r\n<AddressType>stellar</AddressType>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Address>USS Enterprise</Address>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<City>Planet Earth</City>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<StateOrProvince>Solar System</StateOrProvince>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<PostCode>12345#WYZ47NL@512</PostCode>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Country>Milky Way Galaxy</Country>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "</ContactAddress>\r\n</ContactInformation>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Fees>none</Fees>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<AccessConstraints>none</AccessConstraints>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<LayerLimit>1</LayerLimit>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<MaxWidth>5000</MaxWidth>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<MaxHeight>5000</MaxHeight>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "</Service>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Capability>\r\n<Request>\r\n<GetCapabilities>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Format>text/xml</Format>\r\n<DCPType>\r\n<HTTP>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Get><OnlineResource xlink:href=\"http://127.0.0.1:8080/wmslite?\" ");
    gaiaAppendToOutBuffer (&xml_text,
			   "xlink:type=\"simple\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"/></Get>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "</HTTP>\r\n</DCPType>\r\n</GetCapabilities>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<GetMap>\r\n<Format>image/png</Format>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Format>image/jpeg</Format>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Format>application/x-pdf</Format>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Format>image/tiff</Format>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<DCPType>\r\n<HTTP>\r\n<Get>");
    gaiaAppendToOutBuffer (&xml_text,
			   "<OnlineResource xlink:href=\"http://127.0.0.1:8080/wmslite?\" ");
    gaiaAppendToOutBuffer (&xml_text,
			   "xlink:type=\"simple\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"/></Get>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "</HTTP>\r\n</DCPType>\r\n</GetMap>\r\n</Request>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Exception>\r\n<Format>XML</Format>\r\n</Exception>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Layer>\r\n<Name>wms_main_layer</Name>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Title>RasterLite2 Coverages</Title>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Abstract>OGC WMS compliant Service</Abstract>\r\n");
    lyr = list->first;
    while (lyr != NULL)
      {
	  /* available Coverages */
	  gaiaAppendToOutBuffer (&xml_text,
				 "<Layer queryable=\"0\" opaque=\"0\" cascaded=\"0\">\r\n");
	  dummy = sqlite3_mprintf ("<Name>%s</Name>\r\n", lyr->layer_name);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy = sqlite3_mprintf ("<Title>%s</Title>\r\n", lyr->title);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy =
	      sqlite3_mprintf ("<Abstract>%s</Abstract>\r\n", lyr->abstract);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy = sqlite3_mprintf ("<CRS>EPSG:%d</CRS>\r\n", lyr->srid);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  gaiaAppendToOutBuffer (&xml_text, "<EX_GeographicBoundingBox>");
	  dummy =
	      sqlite3_mprintf ("<westBoundLongitude>%1.6f</westBoundLongitude>",
			       lyr->geo_minx);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy =
	      sqlite3_mprintf ("<eastBoundLongitude>%1.6f</eastBoundLongitude>",
			       lyr->geo_maxx);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy =
	      sqlite3_mprintf ("<southBoundLatitude>%1.6f</southBoundLatitude>",
			       lyr->geo_miny);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy =
	      sqlite3_mprintf ("<northBoundLatitude>%1.6f</northBoundLatitude>",
			       lyr->geo_maxy);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  gaiaAppendToOutBuffer (&xml_text, "</EX_GeographicBoundingBox>\r\n");
	  dummy = sqlite3_mprintf ("<BoundingBox CRS=\"EPSG:%d\" "
				   "minx=\"%1.6f\" miny=\"%1.6f\" maxx=\"%1.6f\" maxy=\"%1.6f\"/>\r\n",
				   lyr->srid, lyr->minx, lyr->miny, lyr->maxx,
				   lyr->maxy);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  gaiaAppendToOutBuffer (&xml_text, "</Layer>\r\n");
	  lyr = lyr->next;
      }
    gaiaAppendToOutBuffer (&xml_text,
			   "</Layer>\r\n</Capability>\r\n</WMS_Capabilities>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "");
    gaiaAppendToOutBuffer (xml_response, "HTTP/1.1 200 OK\r\n");
    dummy = get_current_timestamp ();
    gaiaAppendToOutBuffer (xml_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (xml_response,
			   "Content-Type: text/xml;charset=UTF-8\r\n");
    dummy = sqlite3_mprintf ("Content-Length: %d\r\n", xml_text.WriteOffset);
    gaiaAppendToOutBuffer (xml_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (xml_response, "Connection: close\r\n\r\n");
    gaiaAppendToOutBuffer (xml_response, xml_text.Buffer);
    gaiaOutBufferReset (&xml_text);
}

static void
wms_get_map (struct wms_args *args, gaiaOutBufferPtr http_response,
	     unsigned char **payload, int *payload_size)
{
/* preparing the WMS GetMap payload */
    int ret;
    char *dummy;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    unsigned char *black;
    int black_sz;

    *payload = NULL;
    *payload_size = 0;

    sql =
	sqlite3_mprintf
	("SELECT RL2_GetMapImage(?, BuildMbr(?, ?, ?, ?), ?, ?, ?, ?, ?, ?)");
    ret = sqlite3_prepare_v2 (args->db_handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, args->layer, strlen (args->layer),
		       SQLITE_STATIC);
    sqlite3_bind_double (stmt, 2, args->minx);
    sqlite3_bind_double (stmt, 3, args->miny);
    sqlite3_bind_double (stmt, 4, args->maxx);
    sqlite3_bind_double (stmt, 5, args->maxy);
    sqlite3_bind_int (stmt, 6, args->width);
    sqlite3_bind_int (stmt, 7, args->height);
    sqlite3_bind_text (stmt, 8, args->style, strlen (args->style),
		       SQLITE_STATIC);
    if (args->format == WMS_PNG)
	sqlite3_bind_text (stmt, 9, "image/png", strlen ("image/png"),
			   SQLITE_TRANSIENT);
    else
	sqlite3_bind_text (stmt, 9, "image/jpeg", strlen ("image/jpeg"),
			   SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 10, args->transparent);
    if (args->format == WMS_PNG)
	sqlite3_bind_int (stmt, 11, 100);
    else
	sqlite3_bind_int (stmt, 11, 80);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      *payload_size = sqlite3_column_bytes (stmt, 0);
		      *payload = malloc (*payload_size);
		      memcpy (*payload, blob, *payload_size);
		  }
	    }
      }
    sqlite3_finalize (stmt);
    if (*payload == NULL)
	goto error;

    gaiaAppendToOutBuffer (http_response, "HTTP/1.1 200 OK\r\n");
    dummy = get_current_timestamp ();
    gaiaAppendToOutBuffer (http_response, dummy);
    sqlite3_free (dummy);
    if (args->format == WMS_JPEG)
	gaiaAppendToOutBuffer (http_response, "Content-Type: image/jpeg\r\n");
    if (args->format == WMS_PNG)
	gaiaAppendToOutBuffer (http_response, "Content-Type: image/png\r\n");
    dummy = sqlite3_mprintf ("Content-Length: %d\r\n", *payload_size);
    gaiaAppendToOutBuffer (http_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (http_response, "Connection: close\r\n\r\n");
    free (args->style);
    return;

  error:
/* returning a black image */
    black_sz = args->width * args->height;
    black = malloc (black_sz);
    memset (black, 0, black_sz);
    if (args->format == WMS_JPEG)
	rl2_gray_to_jpeg (args->width, args->height, black, 80, payload,
			  payload_size);
    if (args->format == WMS_PNG)
	rl2_gray_to_png (args->width, args->height, black, payload,
			 payload_size);
    free (black);
    gaiaAppendToOutBuffer (http_response, "HTTP/1.1 200 OK\r\n");
    dummy = get_current_timestamp ();
    gaiaAppendToOutBuffer (http_response, dummy);
    sqlite3_free (dummy);
    if (args->format == WMS_JPEG)
	gaiaAppendToOutBuffer (http_response, "Content-Type: image/jpeg\r\n");
    if (args->format == WMS_PNG)
	gaiaAppendToOutBuffer (http_response, "Content-Type: image/png\r\n");
    dummy = sqlite3_mprintf ("Content-Length: %d\r\n", *payload_size);
    gaiaAppendToOutBuffer (http_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (http_response, "Connection: close\r\n\r\n");
    free (args->style);
}

static int
get_xml_bytes (gaiaOutBufferPtr xml_response, int curr, int block_sz)
{
/* determining how many bytes will be sent on the output socket */
    int wr = block_sz;
    if (xml_response->Buffer == NULL || xml_response->Error)
	return 0;
    if (curr + wr > xml_response->WriteOffset)
	wr = xml_response->WriteOffset - curr;
    return wr;
}

static int
get_payload_bytes (int total, int curr, int block_sz)
{
/* determining how many bytes will be sent on the output socket */
    int wr = block_sz;
    if (curr + wr > total)
	wr = total - curr;
    return wr;
}

#ifdef _WIN32
/* Winsockets - some king of Windows */
static void
win32_http_request (void *data)
{
/* Processing an incoming HTTP request */
    gaiaOutBuffer xml_response;
    struct http_request *req = (struct http_request *) data;
    struct wms_args *args = NULL;
    int curr;
    int rd;
    int wr;
    char *ptr;
    char http_hdr[2000];	/* The HTTP request header */
    int http_status;
    unsigned char *payload = NULL;
    int payload_size;

    curr = 0;
    while ((unsigned int) curr < sizeof (http_hdr))
      {
	  rd = recv (req->socket, &http_hdr[curr], sizeof (http_hdr) - 1 - curr,
		     0);
	  if (rd == SOCKET_ERROR)
	      goto end_request;
	  if (rd == 0)
	      break;
	  curr += rd;
	  http_hdr[curr] = '\0';
	  ptr = strstr (http_hdr, "\r\n\r\n");
	  if (ptr)
	      break;
      }
    if ((unsigned int) curr >= sizeof (http_hdr))
      {
	  http_status = 400;
	  goto http_error;
      }

    args = parse_http_request (http_hdr);
    if (args == NULL)
      {
	  http_status = 400;
	  goto http_error;
      }

    http_status = check_wms_request (req->list, args);
    if (http_status != 200)
	goto http_error;
    if (args->request_type == WMS_ILLEGAL_REQUEST)
	goto illegal_request;
    if (args->request_type == WMS_GET_CAPABILITIES)
      {
	  /* preparing the XML WMS GetCapabilities */
	  gaiaOutBufferInitialize (&xml_response);
	  wms_get_capabilities (req->list, &xml_response);
	  curr = 0;
	  while (1)
	    {
		rd = get_xml_bytes (&xml_response, curr, 1024);
		if (rd == 0)
		    break;
		wr = send (req->socket, xml_response.Buffer + curr, rd, 0);
		if (wr < 0)
		    break;
		curr += wr;
	    }
	  gaiaOutBufferReset (&xml_response);
      }
    if (args->request_type == WMS_GET_MAP)
      {
	  /* preparing the WMS GetMap payload */
	  gaiaOutBufferInitialize (&xml_response);
	  args->db_handle = req->db_handle;
	  wms_get_map (args, &xml_response, &payload, &payload_size);
	  curr = 0;
	  while (1)
	    {
		rd = get_xml_bytes (&xml_response, curr, 1024);
		if (rd == 0)
		    break;
		wr = send (req->socket, xml_response.Buffer + curr, rd, 0);
		if (wr < 0)
		    break;
		curr += wr;
	    }
	  curr = 0;
	  while (1)
	    {
		rd = get_payload_bytes (payload_size, curr, 1024);
		if (rd == 0)
		    break;
		wr = send (req->socket, payload + curr, rd, 0);
		if (wr < 0)
		    break;
		curr += wr;
	    }
      }
    destroy_wms_args (args);
    goto end_request;

/* preparing an HTTP error code */
  http_error:
    gaiaOutBufferInitialize (&xml_response);
    build_http_error (http_status, &xml_response, req->port_no);
    if (args != NULL)
	destroy_wms_args (args);
    curr = 0;
    while (1)
      {
	  rd = get_xml_bytes (&xml_response, curr, 1024);
	  if (rd == 0)
	      break;
	  send (req->socket, xml_response.Buffer + curr, rd, 0);
	  curr += rd;
      }
    gaiaOutBufferReset (&xml_response);
    goto end_request;

/* preparing an XML WMS Exception message */
  illegal_request:
    gaiaOutBufferInitialize (&xml_response);
    build_wms_exception (args, &xml_response);
    if (args != NULL)
	destroy_wms_args (args);
    curr = 0;
    while (1)
      {
	  rd = get_xml_bytes (&xml_response, curr, 1024);
	  if (rd == 0)
	      break;
	  send (req->socket, xml_response.Buffer + curr, rd, 0);
	  curr += rd;
      }
    gaiaOutBufferReset (&xml_response);

  end_request:
    closesocket (req->socket);
    free (req);
}
#else
/* standard Berkeley Sockets - may be Linux or *nix */
static void *
berkeley_http_request (void *data)
{
/* Processing an incoming HTTP request */
    gaiaOutBuffer xml_response;
    struct http_request *req = (struct http_request *) data;
    struct wms_args *args = NULL;
    int curr;
    int rd;
    char *ptr;
    char http_hdr[2000];	/* The HTTP request header */
    int http_status;
    unsigned char *payload = NULL;
    int payload_size;

    curr = 0;
    while ((unsigned int) curr < sizeof (http_hdr))
      {
	  rd = recv (req->socket, &http_hdr[curr], sizeof (http_hdr) - 1 - curr,
		     0);
	  if (rd == -1)
	      goto end_request;
	  if (rd == 0)
	      break;
	  curr += rd;
	  http_hdr[curr] = '\0';
	  ptr = strstr (http_hdr, "\r\n\r\n");
	  if (ptr)
	      break;
      }
    if ((unsigned int) curr >= sizeof (http_hdr))
      {
	  http_status = 400;
	  goto http_error;
      }

    args = parse_http_request (http_hdr);
    if (args == NULL)
      {
	  http_status = 400;
	  goto http_error;
      }

    http_status = check_wms_request (req->list, args);
    if (http_status != 200)
	goto http_error;
    if (args->request_type == WMS_ILLEGAL_REQUEST)
	goto illegal_request;
    if (args->request_type == WMS_GET_CAPABILITIES)
      {
	  /* preparing the XML WMS GetCapabilities */
	  gaiaOutBufferInitialize (&xml_response);
	  wms_get_capabilities (req->list, &xml_response);
	  curr = 0;
	  while (1)
	    {
		rd = get_xml_bytes (&xml_response, curr, 1024);
		if (rd == 0)
		    break;
		send (req->socket, xml_response.Buffer + curr, rd, 0);
		curr += rd;
	    }
	  gaiaOutBufferReset (&xml_response);
      }
    if (args->request_type == WMS_GET_MAP)
      {
	  /* preparing the WMS GetMap payload */
	  gaiaOutBufferInitialize (&xml_response);
	  args->db_handle = req->db_handle;
	  wms_get_map (req->list, &xml_response, &payload, &payload_size);
	  curr = 0;
	  while (1)
	    {
		rd = get_xml_bytes (&xml_response, curr, 1024);
		if (rd == 0)
		    break;
		send (req->socket, xml_response.Buffer + curr, rd, 0);
		curr += rd;
	    }
	  gaiaOutBufferReset (&xml_response);
	  curr = 0;
	  while (1)
	    {
		rd = get_payload_bytes (payload_size, curr, 1024);
		if (rd == 0)
		    break;
		send (req->socket, payload + curr, rd, 0);
		curr += rd;
	    }
	  free (payload);
      }
    destroy_wms_args (args);
    goto end_request;

/* preparing an HTTP error code */
  http_error:
    gaiaOutBufferInitialize (&xml_response);
    build_http_error (http_status, &xml_response, req->port_no);
    if (args != NULL)
	destroy_wms_args (args);
    curr = 0;
    while (1)
      {
	  rd = get_xml_bytes (&xml_response, curr, 1024);
	  if (rd == 0)
	      break;
	  send (req->socket, xml_response.Buffer + curr, rd, 0);
	  curr += rd;
      }
    gaiaOutBufferReset (&xml_response);
    goto end_request;

/* preparing an XML WMS Exception message */
  illegal_request:
    gaiaOutBufferInitialize (&xml_response);
    build_wms_exception (args, &xml_response);
    if (args != NULL)
	destroy_wms_args (args);
    curr = 0;
    while (1)
      {
	  rd = get_xml_bytes (&xml_response, curr, 1024);
	  if (rd == 0)
	      break;
	  send (req->socket, xml_response.Buffer + curr, rd, 0);
	  curr += rd;
      }
    gaiaOutBufferReset (&xml_response);

  end_request:
    close (req->socket);
    free (req);
    pthread_exit (NULL);
}
#endif

static int
do_accept_loop (struct neutral_socket *skt, struct wms_list *list, int port_no,
		sqlite3 * db_handle)
{
/* implementing the ACCEPT loop */
    unsigned int id = 0;
#ifdef _WIN32
    SOCKET socket = skt->socket;
    SOCKET client;
    SOCKADDR_IN client_addr;
    int len = sizeof (client_addr);
    int wsaError;
    struct http_request *req;

    while (1)
      {
	  /* never ending loop */
	  client = accept (socket, (struct sockaddr *) &client_addr, &len);
	  if (client == INVALID_SOCKET)
	    {
		wsaError = WSAGetLastError ();
		if (wsaError == WSAEINTR || wsaError == WSAENOTSOCK)
		  {
		      WSACleanup ();
		      fprintf (stderr, "accept error: %d\n", wsaError);
		      return 0;
		  }
		else
		  {
		      closesocket (socket);
		      WSACleanup ();
		      fprintf (stderr, "error from accept()\n");
		      return 0;
		  }
	    }
	  req = malloc (sizeof (struct http_request));
	  req->id = id++;
	  req->port_no = port_no;
	  req->socket = client;
	  req->addr = client_addr;
	  req->list = list;
	  req->db_handle = db_handle;
	  _beginthread (win32_http_request, 0, (void *) req);
      }
    return 1;
#else
    pthread_t thread_id;
    pthread_attr_t attr;
    int socket = skt->socket;
    int client;
    struct sockaddr client_addr;
    socklen_t len = sizeof (struct sockaddr);
    struct http_request *req;

    while (1)
      {
	  /* never ending loop */
	  client = accept (socket, (struct sockaddr *) &client_addr, &len);
	  if (client == -1)
	    {
		close (socket);
		fprintf (stderr, "error from accept()\n");
		return 0;
	    }
	  req = malloc (sizeof (struct http_request));
	  req->id = id++;
	  req->port_no = port_no;
	  req->socket = client;
	  req->addr = client_addr;
	  req->list = list;
	  req->db_handle = db_handle;
	  pthread_attr_init (&attr);
	  pthread_create (&thread_id, NULL, berkeley_http_request,
			  (void *) req);
      }
    return 1;
#endif
}

static int
do_start_http (int port_no, struct neutral_socket *srv_skt)
{
/* starting the HTTP server */
#ifdef _WIN32
/* Winsockets */
    WSADATA wd;
    SOCKET skt = INVALID_SOCKET;
    SOCKADDR_IN addr;

    if (WSAStartup (MAKEWORD (1, 1), &wd))
      {
	  fprintf (stderr, "unable to initialize winsock\n");
	  return 0;
      }
    skt = socket (AF_INET, SOCK_STREAM, 0);
    if (skt == INVALID_SOCKET)
      {
	  fprintf (stderr, "unable to create a socket\n");
	  return 0;
      }
    addr.sin_family = AF_INET;
    addr.sin_port = htons (port_no);
    addr.sin_addr.s_addr = inet_addr ("127.0.0.1");
    if (bind (skt, (struct sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
      {
	  fprintf (stderr, "unable to bind the socket\n");
	  closesocket (skt);
	  return 0;
      }
    if (listen (skt, SOMAXCONN) == SOCKET_ERROR)
      {
	  fprintf (stderr, "unable to listen on the socket\n");
	  closesocket (skt);
	  return 0;
      }
    srv_skt->socket = skt;
#else
/* standard Berkeley sockets */
    int skt = -1;
    struct sockaddr_in addr;

    skt = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (skt == -1)
      {
	  fprintf (stderr, "unable to create a socket\n");
	  return 0;
      }
    addr.sin_family = AF_INET;
    addr.sin_port = htons (port_no);
    addr.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (skt, (struct sockaddr *) &addr, sizeof (addr)) == -1)
      {
	  fprintf (stderr, "unable to bind the socket\n");
	  close (skt);
	  return 0;
      }
    if (listen (skt, SOMAXCONN) == -1)
      {
	  fprintf (stderr, "unable to listen on the socket\n");
	  close (skt);
	  return 0;
      }
    srv_skt->socket = skt;
#endif
    printf ("======================================================\n");
    printf ("    HTTP micro-server listening on port: %d\n", port_no);
    printf ("======================================================\n");
    return 1;
}

static void
compute_geographic_extents (sqlite3 * handle, struct wms_list *list)
{
/* computing the Geographic Bounding Box for every WMS layer */
    const char *sql;
    sqlite3_stmt *stmt = NULL;
    int ret;
    struct wms_layer *lyr = list->first;
    while (lyr != NULL)
      {
	  sql = "SELECT MbrMinX(g.g), MbrMinY(g.g), MbrMaxX(g.g), MbrMaxY(g.g) "
	      "FROM (SELECT ST_Transform(BuildMbr(?, ?, ?, ?, ?), 4326) AS g) AS g";
	  ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		lyr->geo_minx = -180.0;
		lyr->geo_miny = -90.0;
		lyr->geo_maxx = 180.0;
		lyr->geo_maxy = 90.0;
		goto error;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_double (stmt, 1, lyr->minx);
	  sqlite3_bind_double (stmt, 2, lyr->miny);
	  sqlite3_bind_double (stmt, 3, lyr->maxx);
	  sqlite3_bind_double (stmt, 4, lyr->maxy);
	  sqlite3_bind_int (stmt, 5, lyr->srid);
	  while (1)
	    {
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;
		if (ret == SQLITE_ROW)
		  {
		      lyr->geo_minx = sqlite3_column_double (stmt, 0);
		      lyr->geo_miny = sqlite3_column_double (stmt, 1);
		      lyr->geo_maxx = sqlite3_column_double (stmt, 2);
		      lyr->geo_maxy = sqlite3_column_double (stmt, 3);
		      printf ("Publishing layer \"%s\"\n", lyr->layer_name);
		  }
	    }
	  sqlite3_finalize (stmt);
	error:
	  lyr = lyr->next;
      }
}

static struct wms_list *
get_raster_coverages (sqlite3 * handle)
{
/* preparing a list of available Raster Coverages */
    struct wms_list *list = NULL;
    int ret;
    sqlite3_stmt *stmt;
    const char *sql = "SELECT coverage_name, title, abstract, sample_type, "
	"pixel_type, num_bands, srid, extent_minx, extent_miny, extent_maxx, extent_maxy "
	"FROM raster_coverages "
	"WHERE extent_minx IS NOT NULL AND extent_miny IS NOT NULL "
	"AND extent_maxx IS NOT NULL AND extent_maxy IS NOT NULL "
	"AND sample_type IN ('1-BIT', '2-BIT', '4-BIT', 'UINT8') "
	"AND pixel_type IN ('MONOCHROME', 'PALETTE', 'GRAYSCALE', 'RGB')";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return NULL;
    list = alloc_wms_list ();
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		struct wms_layer *lyr = NULL;
		const char *coverage_name =
		    (const char *) sqlite3_column_text (stmt, 0);
		const char *title =
		    (const char *) sqlite3_column_text (stmt, 1);
		const char *abstract =
		    (const char *) sqlite3_column_text (stmt, 2);
		const char *sample_type =
		    (const char *) sqlite3_column_text (stmt, 3);
		const char *pixel_type =
		    (const char *) sqlite3_column_text (stmt, 4);
		int num_bands = sqlite3_column_int (stmt, 5);
		int srid = sqlite3_column_int (stmt, 6);
		double minx = sqlite3_column_double (stmt, 7);
		double miny = sqlite3_column_double (stmt, 8);
		double maxx = sqlite3_column_double (stmt, 9);
		double maxy = sqlite3_column_double (stmt, 10);
		if (strcmp (sample_type, "1-BIT") == 0
		    && strcmp (pixel_type, "MONOCHROME") == 0 && num_bands == 1)
		    lyr =
			alloc_wms_layer (coverage_name, title, abstract, srid,
					 minx, miny, maxx, maxy,
					 RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME,
					 1);
		if (strcmp (sample_type, "1-BIT") == 0
		    && strcmp (pixel_type, "PALETTE") == 0 && num_bands == 1)
		    lyr =
			alloc_wms_layer (coverage_name, title, abstract, srid,
					 minx, miny, maxx, maxy,
					 RL2_SAMPLE_1_BIT, RL2_PIXEL_PALETTE,
					 1);
		if (strcmp (sample_type, "2-BIT") == 0
		    && strcmp (pixel_type, "PALETTE") == 0 && num_bands == 1)
		    lyr =
			alloc_wms_layer (coverage_name, title, abstract, srid,
					 minx, miny, maxx, maxy,
					 RL2_SAMPLE_2_BIT, RL2_PIXEL_PALETTE,
					 1);
		if (strcmp (sample_type, "4-BIT") == 0
		    && strcmp (pixel_type, "PALETTE") == 0 && num_bands == 1)
		    lyr =
			alloc_wms_layer (coverage_name, title, abstract, srid,
					 minx, miny, maxx, maxy,
					 RL2_SAMPLE_4_BIT, RL2_PIXEL_PALETTE,
					 1);
		if (strcmp (sample_type, "UINT8") == 0
		    && strcmp (pixel_type, "PALETTE") == 0 && num_bands == 1)
		    lyr =
			alloc_wms_layer (coverage_name, title, abstract, srid,
					 minx, miny, maxx, maxy,
					 RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE,
					 1);
		if (strcmp (sample_type, "UINT8") == 0
		    && strcmp (pixel_type, "GRAYSCALE") == 0 && num_bands == 1)
		    lyr =
			alloc_wms_layer (coverage_name, title, abstract, srid,
					 minx, miny, maxx, maxy,
					 RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE,
					 1);
		if (strcmp (sample_type, "UINT8") == 0
		    && strcmp (pixel_type, "RGB") == 0 && num_bands == 3)
		    lyr =
			alloc_wms_layer (coverage_name, title, abstract, srid,
					 minx, miny, maxx, maxy,
					 RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
		if (lyr != NULL)
		  {
		      if (list->first == NULL)
			  list->first = lyr;
		      if (list->last != NULL)
			  list->last->next = lyr;
		      list->last = lyr;
		  }
	    }
      }
    sqlite3_finalize (stmt);
    if (list->first == NULL)
	goto error;
    return list;

  error:
    if (list != NULL)
	destroy_wms_list (list);
    return NULL;
}

static void
open_db (const char *path, sqlite3 ** handle, int cache_size, void *cache)
{
/* opening the DB */
    sqlite3 *db_handle;
    int ret;
    char sql[1024];

    *handle = NULL;
    printf ("\n======================================================\n");
    printf ("              WmsLite server startup\n");
    printf ("======================================================\n");
    printf ("         SQLite version: %s\n", sqlite3_libversion ());
    printf ("     SpatiaLite version: %s\n", spatialite_version ());
    printf ("    RasterLite2 version: %s\n", rl2_version ());
    printf ("======================================================\n");

    ret = sqlite3_open_v2 (path, &db_handle, SQLITE_OPEN_READONLY, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open '%s': %s\n", path,
		   sqlite3_errmsg (db_handle));
	  sqlite3_close (db_handle);
	  return;
      }
    spatialite_init_ex (db_handle, cache, 0);
    if (cache_size > 0)
      {
	  /* setting the CACHE-SIZE */
	  sprintf (sql, "PRAGMA cache_size=%d", cache_size);
	  sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
      }
    rl2_init (db_handle, 0);

    *handle = db_handle;
    return;
}

static void
do_help ()
{
/* printing the argument list */
    fprintf (stderr, "\n\nusage: wmslite ARGLIST ]\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr, "-db or --db-path      pathname  RasterLite2 DB path\n");
    fprintf (stderr,
	     "-p or --ip-port       number    IP port number [default: 8080]\n\n");
    fprintf (stderr,
	     "-cs or --cache-size    num      DB cache size (how many pages)\n");
}

int
main (int argc, char *argv[])
{
/* the MAIN function simply perform arguments checking */
    struct neutral_socket skt_ptr;
    sqlite3 *handle;
    int i;
    int error = 0;
    int next_arg = ARG_NONE;
    const char *db_path = NULL;
    int port_no = 8080;
    int cache_size = 0;
    void *cache;
    struct wms_list *list;

    for (i = 1; i < argc; i++)
      {
	  /* parsing the invocation arguments */
	  if (next_arg != ARG_NONE)
	    {
		switch (next_arg)
		  {
		  case ARG_DB_PATH:
		      db_path = argv[i];
		      break;
		  case ARG_IP_PORT:
		      port_no = atoi (argv[i]);
		      break;
		  case ARG_CACHE_SIZE:
		      cache_size = atoi (argv[i]);
		      break;
		  };
		next_arg = ARG_NONE;
		continue;
	    }

	  if (strcasecmp (argv[i], "--help") == 0
	      || strcmp (argv[i], "-h") == 0)
	    {
		do_help ();
		return -1;
	    }
	  if (strcmp (argv[i], "-db") == 0
	      || strcasecmp (argv[i], "--db-path") == 0)
	    {
		next_arg = ARG_DB_PATH;
		continue;
	    }
	  if (strcmp (argv[i], "-p") == 0
	      || strcasecmp (argv[i], "--ip-port") == 0)
	    {
		next_arg = ARG_IP_PORT;
		continue;
	    }
	  if (strcasecmp (argv[i], "--cache-size") == 0
	      || strcmp (argv[i], "-cs") == 0)
	    {
		next_arg = ARG_CACHE_SIZE;
		continue;
	    }
	  fprintf (stderr, "unknown argument: %s\n", argv[i]);
	  error = 1;
      }
    if (error)
      {
	  do_help ();
	  return -1;
      }

/* checking the arguments */
    if (db_path == NULL)
      {
	  fprintf (stderr, "did you forget to specify the --db-path arg ?\n");
	  error = 1;
      }
    if (error)
      {
	  do_help ();
	  return -1;
      }

/* opening the DB */
    cache = spatialite_alloc_connection ();
    open_db (db_path, &handle, cache_size, cache);
    if (!handle)
	return -1;

    list = get_raster_coverages (handle);
    if (list == NULL)
      {
	  fprintf (stderr,
		   "the DB \"%s\" doesn't contain any valid Raster Coverage\n",
		   db_path);
	  goto stop;
      }
    compute_geographic_extents (handle, list);

/* starting the HTTP server */
    if (!do_start_http (port_no, &skt_ptr))
	goto stop;

/* looping on requests */
    if (!do_accept_loop (&skt_ptr, list, port_no, handle))
	goto stop;

  stop:
    destroy_wms_list (list);
    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);
    spatialite_shutdown ();
    return 0;
}
