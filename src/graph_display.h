#ifndef GRAPH_DISPLAY_H_INCLUDED
#define GRAPH_DISPLAY_H_INCLUDED

#include <gtk/gtk.h>

__attribute__((align(1))) struct graph_display_color {
        union {
                struct {
                        uint8_t         b;
                        uint8_t         g;
                        uint8_t         r;
                };
                uint8_t                 c[3];
        };
};

enum AccelerateMethod {
        AccelerateMethodNone,
        AccelerateMethodGrid,
        AccelerateMethodFADE,
        c_NumAccelerateMethod
};

struct graph_display*   graph_display_create(enum AccelerateMethod acc);
void                    graph_display_free(struct graph_display* self);
void                    graph_display_set_dimension(struct graph_display* self, int width, int height);
void                    graph_display_force_directed(struct graph_display* self, struct bio_graph* g, int max_steps);
int                     graph_display_force_directed_progressive(struct graph_display* self, struct bio_graph* g, int iterator);
void                    graph_display_rasterize(struct graph_display* self);
void                    graph_display_progressive_draw_to_gtk_screen(struct graph_display* self, struct bio_graph* g,
                                                                     GtkWidget* widget, int* argc, char*** argv);
const void*             graph_display_fetch_memory(const struct graph_display* self, int* width, int* height, int* ps);


#endif // GRAPH_DISPLAY_H_INCLUDED
