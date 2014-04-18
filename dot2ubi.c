/*******************************************************
 * Visualize a Graphviz dot file in 3-D using Ubigraph *
 * By Scott Pakin <scott+dt2ub@pakin.org>              *
 *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <graphviz/cgraph.h>
#include <graphviz/color.h>
#include <UbigraphAPI.h>

/* Produce a Ubigraph ID from a Graphviz object. */
#define UBID(N) ((int)AGID(N))

/* Get the value of a node/edge attribute and return true if not NULL or empty. */
#define GET_NODE_VALUE(KEY) ((value = agget(node, KEY)) != NULL && value[0] != '\0')
#define GET_EDGE_VALUE(KEY) ((value = agget(edge, KEY)) != NULL && value[0] != '\0')

extern int colorxlate(const char *str, gvcolor_t *color, color_type_t target_type);
extern void setColorScheme (char *s);

const char *progname;   /* Name of this program */

/* Associate planar with solid shapes. */
typedef struct {
  const char *planar;      /* Name of a planar shape */
  const char *solid;       /* Name of a solid shape */
} planar_solid_t;

/* Define a mapping from planar to solid shapes. */
planar_solid_t **planar_to_solid;
size_t planar_to_solid_len;

/* Initialize the planar-to-solid mapping. */
void initialize_planar_to_solid (void)
{
  static planar_solid_t initial_map[] = {
    { "box",            "cube"         },
    { "polygon",        "dodecahedron" },
    { "ellipse",        "sphere"       },
    { "circle",         "sphere"       },
    { "point",          "sphere"       },
    { "egg",            "sphere"       },
    { "triangle",       "octahedron"   },
    { "plaintext",      "cube"         },
    { "diamond",        "octahedron"   },
    { "trapezium",      "cube"         },
    { "parallelogram",  "cube"         },
    { "house",          "octahedron"   },
    { "octagon",        "icosahedron"  },
    { "doublecircle",   "sphere"       },
    { "doubleoctagon",  "icosahedron"  },
    { "tripleoctagon",  "icosahedron"  },
    { "invtriangle",    "octahedron"   },
    { "invtrapezium",   "cube"         },
    { "invhouse",       "octahedron"   },
    { "Mdiamond",       "octahedron"   },
    { "Msquare",        "cube"         },
    { "Mcircle",        "sphere"       },
    { "none",           "cube"         },
    { "record",         "cube"         },
    { "Mrecord",        "cube"         }
  };
  size_t i;

  planar_to_solid_len = sizeof(initial_map)/sizeof(planar_solid_t);
  planar_to_solid = (planar_solid_t **) malloc(planar_to_solid_len*sizeof(planar_solid_t *));
  if (planar_to_solid == NULL) {
    perror("malloc");
    exit(1);
  }
  for (i = 0; i < planar_to_solid_len; i++)
    planar_to_solid[i] = &initial_map[i];
}

/* Read a dot file into memory and a return the associated graph.
 * Abort on error. */
Agraph_t *read_dot (const char *dotfilename)
{
  int use_stdin;    /* 1=read from stdin; 0=read from a file */
  FILE *dotfile;
  Agraph_t *graph;

  /* Open the input file.  "-" means standard input. */
  use_stdin = dotfilename[0] == '-' && dotfilename[1] == '\0';
  if (use_stdin)
    dotfile = stdin;
  else {
    dotfile = fopen(dotfilename, "r");
    if (!dotfile) {
      perror("fopen");
      exit(1);
    }
  }

  /* Parse the input file. */
  graph = agread(dotfile, &AgDefaultDisc);
  if (!graph) {
    fprintf(stderr, "%s: Failed to parse %s\n", progname, use_stdin ? "standard input" : dotfilename);
    exit(1);
  }

  /* Close the input file. */
  if (!use_stdin)
    fclose(dotfile);
  return graph;
}

/* Initialize the Ubigraph canvas. */
void initialize_ubigraph (Agraph_t *graph)
{
  /* Set defaults that resemble dot's defaults.  There are some
   * differences due to what makes sense in 3-D vs. 2-D and what makes
   * sense on a black background vs. a white background. */
  ubigraph_clear();
  ubigraph_set_vertex_style_attribute(0, "color", "#ffffff");  /* dot: no fill but white background */
  ubigraph_set_vertex_style_attribute(0, "shape", "sphere");   /* dot: ellipse */
  ubigraph_set_vertex_style_attribute(0, "size", "0.75");   /* dot: 0.75 by 0.5 */
  ubigraph_set_vertex_style_attribute(0, "fontfamily", "Times-Roman");   /* Same as dot */
  ubigraph_set_vertex_style_attribute(0, "fontcolor", "#ffffff");   /* dot: black */
  ubigraph_set_vertex_style_attribute(0, "fontsize", "14");   /* Same as dot */
  if (agisdirected(graph)) {
    ubigraph_set_edge_style_attribute(0, "arrow", "true");   /* Same as dot */
    ubigraph_set_edge_style_attribute(0, "arrow_position", "1.0");   /* Same as dot */
  }
  ubigraph_set_edge_style_attribute(0, "color", "#ffffff");  /* dot: black */
  ubigraph_set_edge_style_attribute(0, "fontfamily", "Times-Roman");   /* Same as dot */
  ubigraph_set_edge_style_attribute(0, "fontcolor", "#ffffff");   /* dot: black */
  ubigraph_set_edge_style_attribute(0, "fontsize", "14");   /* Same as dot */
  initialize_planar_to_solid();
}

/* Convert a color to #RRGGBB format.  Return a pointer to a static string. */
char *convert_color (const char *color_name)
{
  static char rgb[10];
  gvcolor_t color;

  if (colorxlate(color_name, &color, RGBA_BYTE) != COLOR_OK) {
    fprintf(stderr, "%s: Failed to identify color \"%s\"\n", progname, color_name);
    exit(1);
  }
  sprintf(rgb, "#%02X%02X%02X",
          color.u.rgba[0],
          color.u.rgba[1],
          color.u.rgba[2]);
  return rgb;
}

/* Convert a planar shape to a solid shape. */
const char *convert_shape (const char *planar)
{
  size_t i;

  for (i = 0; i < planar_to_solid_len; i++)
    if (!strcmp(planar_to_solid[i]->planar, planar)) {
      /* Found -- bubble up the association for faster lookup next time. */
      const char *solid = planar_to_solid[i]->solid;
      if (0 && i > 0) {
        planar_solid_t *prev = planar_to_solid[i - 1];
        planar_to_solid[i - 1] = planar_to_solid[i];
        planar_to_solid[i] = prev;
      }
      return solid;
    }
  return "torus";   /* Use "torus" as the not-found solid. */
}

/* Draw a node, appropriately mapping its attributes. */
void draw_node (Agnode_t *node)
{
  int node_id = UBID(node);
  char *value;
  double size = 0.0;

  /* Create a new node. */
  ubigraph_new_vertex_w_id(node_id);

  /* Set the dot color scheme. */
  if (GET_NODE_VALUE("colorscheme"))
    setColorScheme(value);

  /* Handle fonts and labels. */
  if (GET_NODE_VALUE("label")) {
    ubigraph_set_vertex_attribute(node_id, "label", value == NULL ? agnameof(node) : value);
    if (GET_NODE_VALUE("fontcolor"))
      ubigraph_set_vertex_attribute(node_id, "fontcolor", convert_color(value));
    if (GET_NODE_VALUE("fontname"))
      ubigraph_set_vertex_attribute(node_id, "fontfamily", value);
    if (GET_NODE_VALUE("fontsize"))
      ubigraph_set_vertex_attribute(node_id, "fontsize", value);
  }

  /* Set the color only if the "style" attribute includes "filled". */
  if (GET_NODE_VALUE("style")) {
    if (strstr(value, "filled") != NULL) {
      char *value;

      if (GET_NODE_VALUE("fillcolor"))
        ubigraph_set_vertex_attribute(node_id, "color", convert_color(value));
    }
    if (strstr(value, "invis") != NULL)
      ubigraph_set_vertex_attribute(node_id, "visible", "false");
  }

  /* Map 2-D shapes to 3-D shapes. */
  if (GET_NODE_VALUE("shape"))
    ubigraph_set_vertex_attribute(node_id, "shape", convert_shape(value));

  /* Adjust the shape size. */
  if (GET_NODE_VALUE("width")) {
    double width = atof(value);
    if (width > size)
      size = width;
  }
  if (GET_NODE_VALUE("height")) {
    double height = atof(value);
    if (height > size)
      size = height;
  }
  if (size > 0) {
    char size_str[50];

    sprintf(size_str, "%31.5f", size);
    ubigraph_set_vertex_attribute(node_id, "size", size_str);
  }
}

/* Draw an edge, appropriately mapping its attributes. */
void draw_edge (Agedge_t *edge)
{
  int edge_id = UBID(edge);
  char *value;

  /* Create a new edge. */
  ubigraph_new_edge_w_id(edge_id, UBID(agtail(edge)), UBID(aghead(edge)));

  /* Set the dot color scheme. */
  if (GET_EDGE_VALUE("colorscheme"))
    setColorScheme(value);

  /* Set the edge color. */
  if (GET_EDGE_VALUE("color")) {
    if (!strcmp(value, "invis"))
      ubigraph_set_edge_attribute(edge_id, "visible", "false");
    else
      ubigraph_set_edge_attribute(edge_id, "color", convert_color(value));
  }

  /* Handle fonts and labels. */
  if (GET_EDGE_VALUE("label")) {
    if (GET_EDGE_VALUE("label"))
      ubigraph_set_edge_attribute(edge_id, "label", value);
    if (GET_EDGE_VALUE("fontcolor"))
      ubigraph_set_edge_attribute(edge_id, "fontcolor", convert_color(value));
    if (GET_EDGE_VALUE("fontname"))
      ubigraph_set_edge_attribute(edge_id, "fontfamily", value);
    if (GET_EDGE_VALUE("fontsize"))
      ubigraph_set_edge_attribute(edge_id, "fontsize", value);
  }

  /* Set the edge width. */
  if (GET_EDGE_VALUE("penwidth"))
    ubigraph_set_edge_attribute(edge_id, "width", value);

  /* Set the edge style. */
  if (GET_EDGE_VALUE("style")) {
    if (strstr(value, "solid") != NULL)
      ubigraph_set_edge_attribute(edge_id, "stroke", "solid");
    else if (strstr(value, "dashed") != NULL)
      ubigraph_set_edge_attribute(edge_id, "stroke", "dashed");
    else if (strstr(value, "dotted") != NULL)
      ubigraph_set_edge_attribute(edge_id, "stroke", "dotted");
    else if (strstr(value, "invis") != NULL)
      ubigraph_set_edge_attribute(edge_id, "stroke", "none");
  }

  /* Set the edge strength. */
  if (GET_EDGE_VALUE("weight"))
    ubigraph_set_edge_attribute(edge_id, "strength", value);

  /* Set the arrowhead size and position. */
  if (GET_EDGE_VALUE("arrowsize")) {
    ubigraph_set_edge_attribute(edge_id, "arrow_radius", value);
    ubigraph_set_edge_attribute(edge_id, "arrow_length", value);
  }
  if (GET_EDGE_VALUE("dir")) {
    /* dir=forward is the default, and Ubigraph doesn't support dir=both. */
    if (!strcmp(value, "back"))
      ubigraph_set_edge_attribute(edge_id, "arrow_reverse", "true");
    else if (!strcmp(value, "none"))
      ubigraph_set_edge_attribute(edge_id, "arrow", "false");
  }
}

int main (int argc, const char *argv[])
{
  Agraph_t *graph;
  Agnode_t *node;

  /* Parse the specified dot file. */
  progname = argv[0];
  graph = read_dot(argc < 2 ? "-" : argv[1]);

  /* Establish a connection to the Ubigraph server and set default options. */
  initialize_ubigraph(graph);

  /* Draw all of the nodes in the entire graph. */
  for (node = agfstnode(graph); node != NULL; node = agnxtnode(graph, node))
    draw_node(node);

  /* Draw all of the edges in the entire graph. */
  for (node = agfstnode(graph); node != NULL; node = agnxtnode(graph, node)) {
    Agedge_t *edge;
    for (edge = agfstout(graph, node); edge != NULL; edge = agnxtout(graph, edge))
      draw_edge(edge);
  }

  /* Exit cleanly. */
  agclose(graph);
  return 0;
}
