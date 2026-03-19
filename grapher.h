#ifndef GRAPHER_H
#define GRAPHER_H
#include <stdlib.h>
#include <termios.h>
#include <stdbool.h>

typedef struct {
    char *items;
    size_t len;
} Sb;

typedef struct {
    double x_range;
    double y_range;

    double x_inc;
    double y_inc;

    int term_width;
    int term_height;

    int term_width_scale;
    int term_height_scale;
} GraphOptions;

typedef struct {
    int o_x;
    int o_y;
    int x;
    int y;
    struct termios orig_termios;
    GraphOptions opts;
} Graph;

#define _increment(c_range, t_range) ((double)(c_range)/(t_range))
#define _new_options_scale(x_range, y_range, term_width, term_height, term_width_scale, term_height_scale) \
    (GraphOptions){ (x_range), (y_range),\
                    _increment(x_range, term_width), _increment(y_range, term_height),\
                    (term_width), (term_height), (term_width_scale), (term_height_scale)\
                  }
#define new_options(x_range, y_range, t_width, t_height) _new_options_scale((x_range), (y_range),(t_width), (t_height), 2, 1)

/* Graphical fns */
void draw(Sb *buf);
void bounding_box(Sb *buf, Graph *g);
void graph(Sb *buf, double (*f)(double x), Graph *g);
void graph_precise(Sb *buf, double (*f)(double x), Graph *g);
void implicit_graph(Sb *buf, bool (*f)(double x, double y), Graph *g);
int init(Sb *buf, Graph *g);
void axes(Sb *buf, Graph *g);
void restore_term(Graph *g);
void reset_pos(Sb *buf, Graph *g);
void mark_at_precise(Sb *buf, double x, double y, Graph *g);
void mark_at(Sb *buf, int x,int y, Graph *g);

#define new_graph_p(x_range, y_range, t_width, t_height) &((Graph){ .opts = new_options( ( x_range ), ( y_range ), ( t_width ), ( t_height )) })

#ifdef GRAPHER_ALIAS
#define  DRAW() draw(buf)
#define  BOUNDING_BOX() bounding_box(buf, g)
#define  GRAPH(f) graph(buf, f, g)
#define  GRAPH_PRECISE(f) graph_precise(buf, f, g)
#define  IMPLICIT_GRAPH(f) implicit_graph(buf, f, g)
#define  INIT() init(buf, g)
#define  AXES() axes(buf, g)
#define  RESTORE_TERM() restore_term(g)
#endif // GRAPHER_ALIAS

#define ORIGIN g->o_x, g->o_y 

#endif // GRAPHER_H
