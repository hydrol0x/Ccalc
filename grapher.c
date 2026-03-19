#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <math.h>
#include <stdbool.h>
#include <stdarg.h>

#include "grapher.h"

void sb_append(Sb *buf, const char *string, size_t len) {
    // TODO: make based on capacity and resize 2x capacity
    char *new= realloc(buf->items, buf->len + len);

    if (new==NULL) return;
    memcpy(&new[buf->len], string, len);
    buf->items=new;
    buf->len += len;
}
#define append(string,n) sb_append(buf, (string), n)

Sb dbg_buf = {0};
void dbg_format_and_append(const char *fmt, ...) {
    char temp_buf[256];
    
    va_list args;
    va_start(args, fmt);
    
    int len = vsnprintf(temp_buf, sizeof(temp_buf), fmt, args);
    
    va_end(args);
    
    if (len > 0) {
        if (len >= sizeof(temp_buf)) {
            len = sizeof(temp_buf) - 1; 
        }
        sb_append(&dbg_buf, temp_buf, len);
    }
}

#define dbg(fmt, ...) dbg_format_and_append((fmt), __VA_ARGS__)

void up(Sb *buf, int n) {
    char s[32];
    int len = snprintf(s, sizeof(s), "\033[%dA", n);
    if (len>0) append(s, len);
}

void down(Sb *buf, int n) {
    char s[32];
    int len = snprintf(s, sizeof(s), "\033[%dB", n);
    if (len>0) append(s, len);
}

void forward(Sb *buf, int n) {
    char s[32];
    int len = snprintf(s, sizeof(s), "\033[%dC", n);
    if (len>0) append(s, len);
}

void back(Sb *buf, int n) {
    char s[32];
    int len = snprintf(s, sizeof(s), "\033[%dD", n);
    if (len>0) append(s, len);

}
void move(Sb *buf, int x, int y) {
    char s[32];
    int len = snprintf(s, sizeof(s), "\033[%d;%dH", y,x);
    if (len>0) append(s, len);
}


void space(Sb *buf, int n) {
    for (int i=0; i<n; i++){
        append("\n", 1);
    }
}

int getCursorPosition(int *row, int *col) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\033[6n", 4) != 4) return -1;

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }

  buf[i]='\0';
  if (buf[0] != '\033' || buf[1] != '[' ) return -1;
  if (sscanf(&buf[2], "%d;%d", row, col) != 2) return -1; 

  return 0;
}


void axes(Sb *buf, Graph *g) {
    GraphOptions o = g->opts;
    space(buf, (o.term_height-1)/2);

    append("\033[38;5;248m", strlen("\033[38;5;248m"));
    for (int i=0; i<o.term_width; i++) {
        append("_", 1);
    }
    space(buf, (o.term_height)/2);

    // cursor is now at bottom left
    forward(buf, (o.term_width-1)/2);
    for (int i=o.term_height; i>0; i--){
        append("|",1);
        up(buf, 1);
        back(buf, 1);
    }
    append("\033[0m",strlen("\033[0m"));
}

void draw(Sb *buf) {
    write(STDOUT_FILENO, buf->items,buf->len);
    buf->len=0;
}

int getTermCoords(int graphx, int graphy, int *termx, int *termy, Graph *g) {
    GraphOptions o = g->opts;
    // range is X: [g.x, g.x+WIDTH]
    //          Y: [g.y, g.y+HEIGHT]
    //          (g.x,g.y) ------------- (g.x+WIDTH, g.y)
    //           |                         | 
    //           |                         |
    //           |                         |
    //  (g.x,g.y+HEIGHT) ------------ (g.x+WIDTH, g.y+HEIGHT)

    // Graph space: 
    // range is X: [-(WIDTH-1)/2, (WIDTH-1)/2]
    // range is Y: [-(HEIGHT-1)/2, (HEIGHT-1)/2]
    

    // inc -> c_range / t_range
    // c_range / inc = t_range

    int new_x = g->o_x+(graphx);
    int new_y = g->o_y-(graphy);
    dbg("new_x, new_y: (%d, %d)\n", new_x, new_y);

    if (new_x<g->x || new_x>(g->x+o.term_width)) return -1;
    if (new_y<g->y || new_y>=(g->y+o.term_height)) return -1;
    *termx=new_x;
    *termy=new_y;
    return 0;
}

void bounding_box(Sb *buf, Graph *g) {
    GraphOptions o = g->opts;
    move(buf, g->x, g->y);
    append("X", 1);
    move(buf, g->x+o.term_width-1, g->y);
    append("X", 1);
    move(buf, g->x+o.term_width-1, g->y+o.term_height-1);
    append("X", 1);
    move(buf, g->x, g->y+o.term_height-1);
    append("X", 1);
    move(buf, ORIGIN);
    append("O", 1);
}

void mark_at(Sb *buf, int x,int y, Graph *g){
    int termx,termy;
    if (getTermCoords(x, y, &termx, &termy, g)<0) {
     //   printf("Out of bounds (%d, %d)", x,y);
        return; // out of bounds
    }
    move(buf, termx,termy);
    append("∙",3);
}

void mark_at_precise(Sb *buf, double x, double y, Graph *g) {
    GraphOptions o = g->opts;
    int termx, termy;

    int abs_idx = (int)roundf((y*4.0)/o.y_inc);

    int graph_x = (int)roundf(x/o.x_inc);
    int graph_y = (int)floor(abs_idx / 4.0);

    dbg("graph_x, graph_y: (%f, %f)\t", graph_x, graph_y);
    if (getTermCoords(graph_x, graph_y, &termx, &termy, g) < 0) {
        return; // out of bounds
    }
    
    move(buf, termx, termy);

    int cell_idx = abs_idx % 4;
    if (cell_idx < 0) cell_idx+=4;

    dbg("Marking at (%f, %f) -> Term: %d, %d\n", x,y,termx,termy);

    switch (cell_idx) {
        case 0: append("⡀", 3); break;
        case 1: append("⠄", 3); break;
        case 2: append("⠂", 3); break;
        case 3: append("⠁", 3); break;
    }
}

void graph(Sb *buf, double (*f)(double x), Graph *g) {
    GraphOptions o = g->opts;
    for (double x=-o.x_range/2; x<o.x_range/2; x+=o.x_inc) {
        double y=f(x);
        mark_at(buf,(int)roundf(x/o.x_inc),(int)roundf(y/o.y_inc), g);
    }
}

void graph_precise(Sb *buf, double (*f)(double x), Graph *g) {
    GraphOptions o = g->opts;
    double x_r = o.x_range;
    for (double x=-x_r/2.0; x<x_r/2.0; x+=o.x_inc) {
        double y=f(x);
        mark_at_precise(buf,x,y, g); 
    }
}

void implicit_graph(Sb *buf, bool (*f)(double x, double y), Graph *g) {
    GraphOptions o = g->opts;
    for (double x=-o.x_range/2; x<=o.x_range/2; x+=o.x_inc) {
        for (double y=-o.y_range/2; y<=o.y_range/2; y+=o.y_inc) {
            if (f(x,y)) mark_at_precise(buf,x,y,g);
        }
    }
}

void restore_term(Graph *g){
    tcsetattr(STDIN_FILENO, TCSANOW, &g->orig_termios);
}

int init(Sb *buf, Graph *g){
    GraphOptions o = g->opts;
    struct termios term;

    if(tcgetattr(STDIN_FILENO, &g->orig_termios) != 0) return -1;

    term = g->orig_termios;
    term.c_lflag &= ~(ICANON | ECHO);
    if(tcsetattr(STDIN_FILENO, TCSANOW, &term) != 0) return -1;

    for (int i=0; i<o.term_height;i++){
        write(STDOUT_FILENO, "\n", 1);
    }

    char up_seq[32];
    int len = snprintf(up_seq, sizeof(up_seq), "\033[%dA", o.term_height);
    write(STDOUT_FILENO, up_seq, len);

    int x,y;
    if (getCursorPosition(&y, &x)<0) return -1;
    g->x=x;
    g->y=y;
    g->o_x= x + ((o.term_width-1)/2);
    g->o_y= y + ((o.term_height-1)/2);

    return 0;
}

void reset_pos(Sb *buf, Graph *g){
    move(buf, g->x,g->y+g->opts.term_height);
}

//#define PI 3.14
//
//double f(double x) { return exp(-pow(x,2.0)); }
//double f2(double x) { return 0.25*x*x; }
//double f3(double x) { return 1.0/x; }
//#define EPS .1
//bool circle(double x, double y) { return fabs(5-(x*x + y*y))<=EPS; }
//bool k(double x, double y) { return x==-9 || y==-8; }
//double linear(double x) { return x;}
//double constant(double x) { return 1;}
//double parabola(double x) { return x*x;}
//double trig(double x) { return sin(x); }
//double wobly(double x) { return sin(exp(x+3)); }
//double inverse(double x) { return 1.0/x;}
//double polynomial(double x) { return (x+2)*(x+3)*(x+4)*(x+5)*(x+6);}
//bool x_pi_2(double x, double y) {return fabs(x-PI/2.0)<=EPS;}

//int main(int argc, char **argv) {
//    GraphOptions o = new_options(10, 10, 50, 25);
//    Graph g = {0};
//    g.opts = o;
//
//    printf("\n\r");
//    /* Initialize */
//    Sb buf={0};
//    if (init(&buf, &g)<0) {
//        restore_term(&g);
//        exit(1);
//    }
//    axes(&buf, &g);
//    bounding_box(&buf, &g);
//
//    /* -- Graph Functions -- */
//   graph_precise(&buf, sin, &g);
//
//    /* Draw */
//    reset_pos(&buf, &g);
//    draw(&buf);
//    restore_term(&g);
//
//    /* Graph Info */
//    printf("\n\r");
//    printf("X range: [%f, %f]; Y range: [%f, %f]; x-step: %f; y-step: %f\n", -o.x_range/2.0, o.x_range/2.0, -o.y_range/2.0, o.y_range/2.0, o.x_inc, o.y_inc);
//
//    /* Debug Info */
//    //draw(&dbg_buf);
//}
