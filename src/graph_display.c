#include "common.h"
#include "bio_graph.h"
#include "graph_display.h"

struct display_vertex {
        float           acc_x;
        float           acc_y;

        float           pos_x;
        float           pos_y;

        float           mass;
};

struct display_data {
        struct display_vertex*  vertices;
        int                     num_verts;
        float                   x_scale;
        float                   y_scale;
        struct bio_graph*       graph;
};

static void __data_init(struct display_data* self)
{
        self->vertices  = nullptr;
        self->graph     = nullptr;
        self->x_scale   = 1.0f;
        self->y_scale   = 1.0f;
}

static void __data_free(struct display_data* self)
{
        free(self->vertices);
        memset(self, 0, sizeof(*self));
}

static struct display_vertex* __data_get_vertices(struct display_data* self)
{
        return self->vertices;
}

static int __data_get_vertex_num(struct display_data* self)
{
        return self->num_verts;
}

static void __data_get_scale(struct display_data* self, float* x_scale, float* y_scale)
{
        *x_scale = self->x_scale;
        *y_scale = self->y_scale;
}

static void __data_find_system_scale(struct display_data* self, float* min_x, float* min_y, float* max_x, float* max_y)
{
        float max_x_tmp = FLT_MIN, min_x_tmp = FLT_MAX;
        float max_y_tmp = FLT_MIN, min_y_tmp = FLT_MAX;;
        int j;
        for (j = 0; j < self->num_verts; j ++) {
                min_x_tmp = MIN(min_x_tmp, self->vertices[j].pos_x);
                max_x_tmp = MAX(max_x_tmp, self->vertices[j].pos_x);
                min_y_tmp = MIN(min_y_tmp, self->vertices[j].pos_y);
                max_y_tmp = MAX(max_y_tmp, self->vertices[j].pos_y);
        }
        *min_x = min_x_tmp;
        *min_y = min_y_tmp;
        *max_x = max_x_tmp;
        *max_y = max_y_tmp;
}

static void __data_redefine_system_position(struct display_data* self)
{
        // redefine the scale
        float max_x, min_x,
              max_y, min_y;
        __data_find_system_scale(self, &min_x, &min_y, &max_x, &max_y);
        self->x_scale = max_x - min_x;
        self->y_scale = max_y - min_y;

        int j;
        for (j = 0; j < self->num_verts; j ++) {
                self->vertices[j].pos_x -= min_x;
                self->vertices[j].pos_y -= min_y;
        }
}

static void __display_bind_data(const struct bio_graph_vertex* v0, void* user_data)
{
        struct display_data* self = user_data;
        int v0_id = bio_graph_vertex_get_id(v0);
        bio_graph_vertex_bind_data((struct bio_graph_vertex*) v0, &self->vertices[v0_id]);
}

static const float c_MetersPerParticle = 2.0f;

static void __data_retrieve_data_from_graph(struct display_data* self, struct bio_graph* graph)
{
        // initialize vector data
        self->graph = graph;
        self->num_verts = bio_graph_get_vertex_num(self->graph);
        if (self->vertices) free(self->vertices);
        self->vertices = malloc(sizeof(*self->vertices)*self->num_verts);
        // bind to the graph vertex
        bio_graph_visit_vertices(self->graph, __display_bind_data, self);

        self->x_scale = sqrtf(self->num_verts)*c_MetersPerParticle;
        self->y_scale = sqrtf(self->num_verts)*c_MetersPerParticle;
}

static struct bio_graph* __data_get_graph(struct display_data* self)
{
        return self->graph;
}

struct display_cell {
        struct display_vertex*  verts;
        int                     num_verts;
        float                   centroid_x;
        float                   centroid_y;
        int                     id_marker;
};
/*
#define c_Grid_X_Span           4
#define c_Grid_Y_Span           4
*/
struct display_grid {
        struct display_cell*    cells;          // grid subdivide approximation
        struct display_cell**   tmp;
        int                     n_grid_x;
        int                     n_grid_y;

        float                   min_x;          // global scale
        float                   max_x;
        float                   min_y;
        float                   max_y;
        float                   interp_x;
        float                   interp_y;
};


static void __grid_init(struct display_grid* self, int nx, int ny)
{
        memset(self, 0, sizeof(*self));
        self->n_grid_x = nx;
        self->n_grid_y = ny;
        self->cells = malloc(sizeof(struct display_cell)*self->n_grid_x*self->n_grid_y);
        self->tmp   = malloc(sizeof(struct display_cell*)*self->n_grid_x*self->n_grid_y);
        int i, j;
        for (i = 0; i < self->n_grid_y; i ++) {
                for (j = 0; j < self->n_grid_x; j ++) {
                        self->cells[j + i*self->n_grid_x].verts = nullptr;
                }
        }
}

static void __grid_free(struct display_grid* self)
{
        if (self->cells) {
                int i, j;
                for (i = 0; i < self->n_grid_y; i ++) {
                        for (j = 0; j < self->n_grid_x; j ++) {
                                free(self->cells[j + i*self->n_grid_x].verts);
                        }
                }
                free(self->cells);
        }
        if (self->tmp) {
                free(self->tmp);
        }
        memset(self, 0, sizeof(*self));
}

static void __grid_reserve_vertex_count(struct display_grid* self, int num_verts)
{
        int i, j;
        for (i = 0; i < self->n_grid_y; i ++) {
                for (j = 0; j < self->n_grid_x; j ++) {
                        int p = j + i*self->n_grid_x;
                        if (self->cells[p].verts) {
                                free(self->cells[p].verts);
                        }
                        self->cells[p].verts = malloc(sizeof(struct display_vertex)*num_verts);
                }
        }
}

static struct display_cell* __grid_which_cell(struct display_grid* self, float x_pos, float y_pos)
{
        int i = (int)((x_pos - self->min_x)*self->interp_x);
        int j = (int)((y_pos - self->min_y)*self->interp_y);
        return &self->cells[i + j*self->n_grid_x];
}

static struct display_cell** __grid_which_cells_within_radius(struct display_grid* self,
                                                             float x_pos, float y_pos, float radius, int* num_cells)
{
        int s_x = (int) ((x_pos - radius - self->min_x)*self->interp_x);
        int s_y = (int) ((y_pos - radius - self->min_y)*self->interp_y);
        int e_x = (int) ((x_pos + radius - self->min_x)*self->interp_x);
        int e_y = (int) ((y_pos + radius - self->min_y)*self->interp_y);

        s_x = MAX(s_x, 0);
        s_y = MAX(s_y, 0);
        e_x = MIN(e_x, self->n_grid_x - 1);
        e_y = MIN(e_y, self->n_grid_y - 1);

        int n_cells = 0;
        int k, l;
        float r2 = radius*radius;
        for (k = s_y; k <= e_y; k ++) {
                for (l = s_x; l <= e_x; l ++) {
                        int p = l + k*self->n_grid_x;
                        float vx = self->cells[p].centroid_x - x_pos;
                        float vy = self->cells[p].centroid_y - y_pos;
                        if (vx*vx + vy*vy < r2) {
                                self->tmp[n_cells ++] = &self->cells[p];
                        }
                }
        }

        *num_cells = n_cells;
        return self->tmp;
}

static void __grid_update_with_vertex(struct display_grid* self, struct display_data* data)
{
        // initialize grid setup
        __data_find_system_scale(data, &self->min_x, &self->min_y, &self->max_x, &self->max_y);
        self->interp_x = (self->n_grid_x - 1)/(self->max_x - self->min_x);
        self->interp_y = (self->n_grid_y - 1)/(self->max_y - self->min_y);

        int i, j;
        for (i = 0; i < self->n_grid_y; i ++) {
                for (j = 0; j < self->n_grid_x; j ++) {
                        int p = j + i*self->n_grid_x;
                        self->cells[p].num_verts = 0;
                        self->cells[p].centroid_x = 0.0f;
                        self->cells[p].centroid_y = 0.0f;
                        self->cells[p].id_marker = -1;
                }
        }
        // put vertex in cell and calculate the centroid of each cell
        struct display_vertex* verts = __data_get_vertices(data);
        for (i = 0; i < __data_get_vertex_num(data); i ++) {
                struct display_cell* cell = __grid_which_cell(self, verts[i].pos_x, verts[i].pos_y);
                cell->verts[cell->num_verts ++] = verts[i];
                cell->centroid_x += verts[i].pos_x;
                cell->centroid_y += verts[j].pos_y;
        }
        for (i = 0; i < self->n_grid_y; i ++) {
                for (j = 0; j < self->n_grid_x; j ++) {
                        int p = j + i*self->n_grid_x;
                        if (self->cells[p].num_verts != 0) {
                                self->cells[p].centroid_x /= self->cells[p].num_verts;
                                self->cells[p].centroid_y /= self->cells[p].num_verts;
                        }
                }
        }
}

struct display_quad {
};

struct display_fade {
};

struct graph_display {
        void*                   buffer;
        int                     width;
        int                     height;
        int                     stride;
        int                     ps;

        struct display_data     data;

        bool                    use_grid;
        struct display_grid     grid;
};

struct graph_display* graph_display_create(enum AccelerateMethod acc)
{
        struct graph_display* self = malloc(sizeof(*self));
        memset(self, 0, sizeof(*self));
        self->width     = 800;
        self->height    = 600;
#ifdef USE_GTK
        self->stride    = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, self->width);
#endif
        self->ps        = self->stride/self->width;
        self->buffer    = malloc(self->stride*self->height);

        __data_init(&self->data);

        switch(acc) {
        case AccelerateMethodNone:
                break;
        case AccelerateMethodGrid:
                self->use_grid  = true;
                break;
        default:
                printf("accelerating structure %d is not supported\n", acc);
                break;
        }

        if (self->use_grid) {
                // self->grid = __grid_create();
        }
        return self;
}

void graph_display_free(struct graph_display* self)
{
        if (self == nullptr) {
                return ;
        }
        free(self->buffer);
        __data_free(&self->data);
        if (self->use_grid) {
                __grid_free(&self->grid);
        }
        memset(self, 0, sizeof(*self));
        free(self);
}

void graph_display_set_dimension(struct graph_display* self, int width, int height)
{
        free(self->buffer);
        self->width = width;
        self->height = height;
        if (width <= 0) {
                self->width = 800;
        }
        if (height <= 0) {
                self->height = 600;
        }
#ifdef USE_GTK
        self->stride    = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, self->width);
#endif // USE_GTK
        self->ps        = self->stride/self->width;
        self->buffer    = malloc(self->stride*self->height);
}

static const float      c_c1 = 1.0f;
static const float      c_c2 = 1.0f;
static const float      c_c3 = 1.0f;
static const float      c_c4 = 0.01f;

static void __edge_string_acceleration(const struct bio_graph_vertex* v0, const struct bio_graph_vertex* v1, void* user_data)
{
        // struct graph_display* self = user_data;

        struct display_vertex* dv0 = bio_graph_vertex_retrieve_data(v0);
        struct display_vertex* dv1 = bio_graph_vertex_retrieve_data(v1);
        dv0->mass = bio_graph_vertex_get_degree(v0);
        dv1->mass = bio_graph_vertex_get_degree(v1);

        float vx = dv1->pos_x - dv0->pos_x;
        float vy = dv1->pos_y - dv0->pos_y;
        float dist2 = vx*vx + vy*vy;
        float dist = sqrtf(dist2);
        if (dist2 < 1e-3) {
                dist = 1e-3;
        }
        float f_spring = c_c1*log(dist/c_c2);
        vx /= dist;
        vy /= dist;
        dv0->acc_x += vx*f_spring;
        dv0->acc_y += vy*f_spring;
}

static void __calc_electrical_acc(struct display_vertex* v0, struct display_vertex* v1_list, int num_v1, float scale_xy)
{
        struct display_vertex* dv0 = v0;

        int i;
        for (i = 0; i < num_v1; i ++) {
                struct display_vertex* dv1 = &v1_list[i];
                if (dv0 == dv1) continue;
                float vx = dv1->pos_x - dv0->pos_x;
                float vy = dv1->pos_y - dv0->pos_y;
                float dist2 = vx*vx + vy*vy;
                float dist = sqrtf(dist2);
                if (dist2 < 1e-3) {
                        dist = 1e-3;
                        dist2 = dist*dist;
                }
                float f_electron = CLAMP(-c_c3/dist2, -scale_xy/10.0f, 0.0f);
                vx /= dist;
                vy /= dist;
                dv0->acc_x += vx*f_electron;
                dv0->acc_y += vy*f_electron;
        }
}
static void __vertex_electrical_acceleration(const struct bio_graph_vertex* v, void* user_data)
{
        struct graph_display* self = user_data;
        float x_scale, y_scale;
        __data_get_scale(&self->data, &x_scale, &y_scale);
        __calc_electrical_acc(bio_graph_vertex_retrieve_data(v),
                              __data_get_vertices(&self->data), __data_get_vertex_num(&self->data),
                              (x_scale + y_scale)*0.5f);
}

static void __vertex_electrical_acceleration_with_grid(const struct bio_graph_vertex* v, void* user_data)
{
        struct graph_display* self = user_data;
        struct display_vertex* v0 = bio_graph_vertex_retrieve_data(v);
        struct display_cell** cells;
        int n_cells;
        cells = __grid_which_cells_within_radius(&self->grid, v0->pos_x, v0->pos_y, 15.0f, &n_cells);
        // interaction of points in nearby cells
        float x_scale, y_scale;
        __data_get_scale(&self->data, &x_scale, &y_scale);
        float scale_xy = (x_scale + y_scale)*0.5f;
        int id = bio_graph_vertex_get_id(v);
        int i;
        for (i = 0; i < n_cells; i ++) {
                __calc_electrical_acc(v0, cells[i]->verts, cells[i]->num_verts, scale_xy);
                cells[i]->id_marker = id;
        }
        // interaction between v and grids
        struct display_grid* grid = &self->grid;
        int j;
        for (i = 0; i < grid->n_grid_y; i ++) {
                for (j = 0; j < grid->n_grid_x; j ++) {
                        int p = j + i*grid->n_grid_x;
                        if (grid->cells[p].num_verts == 0 || grid->cells[p].id_marker == id) {
                                goto omit;
                        }
                        float mass = grid->cells[p].num_verts;
                        float vx = grid->cells[p].centroid_x - v0->pos_x;
                        float vy = grid->cells[p].centroid_y - v0->pos_y;
                        float dist2 = vx*vx + vy*vy;
                        float dist = sqrtf(dist2);
                        float f_electron = CLAMP(-mass*c_c3/dist2, -scale_xy/10.0f, 0.0f);
                        vx /= dist;
                        vy /= dist;
                        v0->acc_x += vx*f_electron;
                        v0->acc_y += vy*f_electron;
omit:;
                }
        }
}

static void __preparation_step(struct graph_display* self, struct bio_graph* g)
{
        __data_retrieve_data_from_graph(&self->data, g);
        float x_scale, y_scale;
        __data_get_scale(&self->data, &x_scale, &y_scale);
        struct display_vertex* verts = __data_get_vertices(&self->data);
        int i;
        for (i = 0; i < __data_get_vertex_num(&self->data); i ++) {
                verts[i].pos_x = (rand()%10001)/10000.0f*x_scale;
                verts[i].pos_y = (rand()%10001)/10000.0f*y_scale;
                verts[i].acc_x = 0.0f;
                verts[i].acc_y = 0.0f;
        }
        // allocate for grid subdivide
        if (self->use_grid) {
                int n = __data_get_vertex_num(&self->data)/4;
                int nxy = (int) sqrtf((float) n);
                __grid_init(&self->grid, nxy, nxy);
                __grid_reserve_vertex_count(&self->grid, __data_get_vertex_num(&self->data));
        }
}

static int __simulation_step(struct graph_display* self, int i)
{
        // simulate mechanical system
#define c_MaxSimulatingSteps               2000
        if (i >= c_MaxSimulatingSteps || i == -1) {
                return -1;
        }

        struct bio_graph* graph = __data_get_graph(&self->data);
        if (self->use_grid) {
                // initialize grid setup
                __grid_update_with_vertex(&self->grid, &self->data);
                // compute acceleration with grid
                bio_graph_visit_edges(graph, __edge_string_acceleration, self);
                bio_graph_visit_vertices(graph, __vertex_electrical_acceleration_with_grid, self);
        } else {
                // compute acceleration
                bio_graph_visit_edges(graph, __edge_string_acceleration, self);
                bio_graph_visit_vertices(graph, __vertex_electrical_acceleration, self);
        }
        // move vertices
        // cooling schedule: t = e^-(i/max_steps)^2
        float width = (float) i/c_MaxSimulatingSteps;
        float d_limit = 0.1f*exp(-width*width);

        float acc_sum = 0.0f;
        struct display_vertex* verts    = __data_get_vertices(&self->data);
        int n_verts                     = __data_get_vertex_num(&self->data);
        int j;
        for (j = 0; j < n_verts; j ++) {
                float mass = MAX(1.0f, verts[j].mass);
                float dx = d_limit*verts[j].acc_x/mass;
                float dy = d_limit*verts[j].acc_y/mass;
                verts[j].pos_x = verts[j].pos_x + dx;
                verts[j].pos_y = verts[j].pos_y + dy;
                acc_sum += fabs(dy) + fabs(dy);
                verts[j].acc_x = 0.0f;
                verts[j].acc_y = 0.0f;
        }
        acc_sum /= (2.0f*n_verts);
        // determine cut-off
        float x_scale, y_scale;
        __data_get_scale(&self->data, &x_scale, &y_scale);
        if (fabs(acc_sum) < 0.00001*(x_scale + y_scale)) {
                return -1;
        }
        return ++ i;
}

static void __finalize_step(struct graph_display* self)
{
        __data_redefine_system_position(&self->data);
}

void graph_display_force_directed(struct graph_display* self, struct bio_graph* g, int max_steps)
{
        __preparation_step(self, g);
        int i, j;
        for (i= 0, j = 0; j < max_steps && i != -1; j ++) {
               i = __simulation_step(self, i);
        }
        __finalize_step(self);
}

int graph_display_force_directed_progressive(struct graph_display* self, struct bio_graph* g, int iterator)
{
        if (iterator == 0) {
                __preparation_step(self, g);
        }
        int i = __simulation_step(self, iterator);
        __finalize_step(self);
        return i;
}

static void __draw_pixel(struct graph_display_color* c, uint8_t* image, int x, int y, int stride, int ps)
{
        struct graph_display_color* pixel = (struct graph_display_color*) &image[x*ps + y*stride];
        *pixel = *c;
}
#if 0
static void __draw_pixel_safe(struct graph_display_color* c, uint8_t* image, int x, int y,
                              int width, int height, int stride, int ps)
{
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= width) x = width - 1;
        if (y >= height) y = height - 1;
        struct graph_display_color* pixel = (struct graph_display_color*) &image[x*ps + y*stride];
        *pixel = *c;
}
#endif
static void __draw_line_safe(struct graph_display_color* c, uint8_t* image,
                             int x_start, int x_end, int y0, int stride, int ps,
                             int width, int height)
{
        if (y0 < 0)             y0 = 0;
        if (y0 >= height)       y0 = height - 1;
        if (x_start < 0)        x_start = 0;
        if (x_start >= width)  x_start = width - 1;
        if (x_end < 0)          x_end = 0;
        if (x_end >= width)    x_end = width - 1;
        int x = x_start;
        int y = y0;
        for (; x <= x_end; x ++) {
                struct graph_display_color* pixel = (struct graph_display_color*) &image[x*ps + y*stride];
                *pixel = *c;
        }
}

static void __draw_circle(int x0, int y0, int radius, struct graph_display_color* c, uint8_t* image,
                          int stride, int ps, int width, int height)
{
        int x = radius;
        int y = 0;
        int d2 = 1 - x;

        while (y <= x) {
                __draw_line_safe(c, image, -x + x0, x + x0, y + y0, stride, ps, width, height);
                __draw_line_safe(c, image, -y + x0, y + x0, x + y0, stride, ps, width, height);
                __draw_line_safe(c, image, -x + x0, x + x0, -y + y0, stride, ps, width, height);
                __draw_line_safe(c, image, -y + x0, y + x0, -x + y0, stride, ps, width, height);
                y ++;
                if (d2 <= 0) {
                        d2 += 2 * y + 1;
                } else {
                        x--;
                        d2 += 2 * (y - x) + 1;
                }
        }
}

void __draw_line(int x0, int y0, int x1, int y1, struct graph_display_color* c, uint8_t* image,
                 int width, int height, int stride, int ps)
{

        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = (dx > dy ? dx : -dy)/2, e2;

        while(true)
        {
                __draw_pixel(c, image, x0, y0, stride, ps);
                if (x0 == x1 && y0 == y1) {
                        break;
                }
                e2 = err;
                if (e2 > -dx) {
                        err -= dy;
                        x0 += sx;
                }
                if (e2 < dy) {
                        err += dx;
                        y0 += sy;
                }
        }
}

struct edge_pack {
        struct graph_display*           display;
        struct graph_display_color      edge_color;
};
static void graph_display_rasterize_edge(const struct bio_graph_vertex* v0, const struct bio_graph_vertex* v1, void* user_data)
{
        struct edge_pack* pack = user_data;
        struct graph_display* self = pack->display;

        struct display_vertex* dv0 = bio_graph_vertex_retrieve_data(v0);
        struct display_vertex* dv1 = bio_graph_vertex_retrieve_data(v1);

        float x_scale, y_scale;
        __data_get_scale(&self->data, &x_scale, &y_scale);
        int px0 = dv0->pos_x/x_scale*(self->width - 1);
        int py0 = dv0->pos_y/y_scale*(self->height - 1);
        int px1 = dv1->pos_x/x_scale*(self->width - 1);
        int py1 = dv1->pos_y/y_scale*(self->height - 1);

        __draw_line(px0, py0, px1, py1, &pack->edge_color, self->buffer, self->width, self->height, self->stride, self->ps);
}

void graph_display_rasterize(struct graph_display* self)
{
        uint8_t* image = self->buffer;
        // fill background
        struct graph_display_color background;
        background.r = background.g = background.b = 0;
        int i, j;
        for (i = 0; i < self->height; i ++) {
                for (j = 0; j < self->width; j ++) {
                        __draw_pixel(&background, image, j, i, self->stride, self->ps);
                }
        }
        // draw vertices
        struct graph_display_color dots;
        dots.r = 255; dots.g = 0; dots.b = 0;
        struct display_vertex* verts = __data_get_vertices(&self->data);
        float x_scale, y_scale;
        __data_get_scale(&self->data, &x_scale, &y_scale);
        for (i = 0; i < __data_get_vertex_num(&self->data); i ++) {
                int px = verts[i].pos_x/x_scale*(self->width - 1);
                int py = verts[i].pos_y/y_scale*(self->height - 1);
                __draw_circle(px, py, 4, &dots, image, self->stride, self->ps, self->width, self->height);
        }
        // draw edges
        struct edge_pack pack;
        pack.display            = self;
        pack.edge_color.r       = 0;
        pack.edge_color.g       = 0;
        pack.edge_color.b       = 255;
        struct bio_graph* g = __data_get_graph(&self->data);
        if (g) {
                bio_graph_visit_edges(g, graph_display_rasterize_edge, &pack);
        }
}

struct gtk_display_pack {
        int                             iterator;
        struct graph_display*           display;
        struct bio_graph*               graph;
        bool                            first_time;
};

#ifdef USE_GTK
static gboolean __display_callback(GtkWidget *widget, cairo_t *cairo, gpointer user_data)
{
        struct gtk_display_pack* pack           = user_data;
        struct graph_display* display           = pack->display;
        struct bio_graph* graph                 = pack->graph;

        pack->iterator = graph_display_force_directed_progressive(display, graph, pack->iterator);
        if (pack->iterator != -1 || pack->first_time) {
                graph_display_rasterize(display);
                pack->first_time = false;
        } else {
                usleep(100000);
        }

        cairo_surface_t *co_surface =
                cairo_image_surface_create_for_data(display->buffer, CAIRO_FORMAT_RGB24,
                                                    display->width, display->height, display->stride);
        cairo_set_source_surface(cairo, co_surface, 0, 0);
        cairo_paint(cairo);
        cairo_surface_destroy(co_surface);

        return 0;
}

static gboolean __activiate_draw(gpointer user_data)
{
        gtk_widget_queue_draw(user_data);
        return 1;
}

static void __make_gtk_window(struct graph_display* self, struct bio_graph* g)
{
        GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW (window), "Bio-Graph Display");
        gtk_window_set_default_size(GTK_WINDOW (window), self->width, self->height);

        GtkWidget* draw_area = gtk_drawing_area_new();
        gtk_widget_set_size_request(draw_area, self->width, self->height);
        gtk_container_add(GTK_CONTAINER(window), draw_area);

        struct gtk_display_pack pack;
        pack.display    = self;
        pack.graph      = g;
        pack.iterator   = 0;
        pack.first_time = true;
        g_signal_connect(draw_area, "draw", G_CALLBACK(__display_callback), (gpointer) &pack);
        g_signal_connect(window, "destroy", gtk_main_quit, nullptr);
        g_idle_add(__activiate_draw, draw_area);

        gtk_widget_show_all(window);

        gtk_main();
}

#endif // USE_GTK


void graph_display_progressive_draw_to_gtk_screen(struct graph_display* self, struct bio_graph* g,
                                                  GtkWidget* widget, int* argc, char*** argv)
{
#ifdef USE_GTK
        printf("Launching gui for display...\n");
        if (!widget) {
                gtk_init(argc, argv);
                __make_gtk_window(self, g);
        }
#endif // USE_GTK
}

const void* graph_display_fetch_memory(const struct graph_display* self, int* width, int* height, int* ps)
{
        *width  = self->width;
        *height = self->height;
        *ps     = self->ps;
        return self->buffer;
}
