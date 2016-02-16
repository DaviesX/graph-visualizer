// bio_graph.c Wen, Chifeng - Sept. 26, 2015
#include "common.h"
#include "bio_graph.h"


struct bio_graph_list {
        struct bio_graph_vertex*       vert_next;
        struct bio_graph_list*         list_next;
};

struct bio_graph_vertex {
        int                             id;
        int                             degree;
        struct bio_graph_list*          linked_vert;
        struct bio_graph_list*          head;
        void*                           data;
};


struct bio_graph {
        int                            num_verts;
        struct bio_graph_vertex*       verts;
};


static void __bio_graph_init(struct bio_graph* self, int num_verts)
{
        self->verts     = malloc(sizeof(*self->verts)*num_verts);
        self->num_verts = num_verts;

        int i;
        for (i = 0; i < num_verts; i ++) {
                self->verts[i].id               = i;
                self->verts[i].degree           = 0;
                self->verts[i].head             = malloc(sizeof(*self->verts[i].head));
                self->verts[i].linked_vert      = self->verts[i].head;
                self->verts[i].linked_vert->list_next = nullptr;
                self->verts[i].linked_vert->vert_next = nullptr;
        }
}

struct bio_graph* bio_graph_create(int num_verts)
{
        struct bio_graph* self = malloc(sizeof(*self));
        __bio_graph_init(self, num_verts);
        return self;
}

void bio_graph_free(struct bio_graph* self)
{
        if (self == nullptr) {
                return ;
        }
        int i;
        for (i = 0; i < self->num_verts; i ++) {
                self->verts[i].degree           = 0;
                struct bio_graph_list* list =  self->verts[i].head;
                while (list) {struct bio_graph_list* t = list->list_next; free(list); list = t;}
        }

        free(self->verts);
        self->num_verts = 0;
        free(self);
}


void bio_graph_make_edge_undirected(struct bio_graph* self, int v0, int v1)
{
        // resolve cycle as a single dot
        if (v0 == v1) {
                self->verts[v0].id = v0;
                return ;
        }
        // reject repetition
        struct bio_graph_list* list =  self->verts[v0].head;
        while (list) {
                if (list->vert_next == &self->verts[v1]) {
                        return ;
                }
                list = list->list_next;
        }
        // make linkage
        struct bio_graph_vertex* gv0 = &self->verts[v0];
        struct bio_graph_vertex* gv1 = &self->verts[v1];
        //gv0->id                         = v0;
        gv0->linked_vert->vert_next     = gv1;
        gv0->linked_vert->list_next     = malloc(sizeof(*gv0));
        gv0->linked_vert                = gv0->linked_vert->list_next;
        gv0->linked_vert->vert_next     = nullptr;
        gv0->linked_vert->list_next     = nullptr;
        gv0->degree ++;

        //gv1->id                         = v1;
        gv1->linked_vert->vert_next     = gv0;
        gv1->linked_vert->list_next     = malloc(sizeof(*gv1));
        gv1->linked_vert                = gv1->linked_vert->list_next;
        gv1->linked_vert->vert_next     = nullptr;
        gv1->linked_vert->list_next     = nullptr;
        gv1->degree ++;
}

static void __bio_graph_traverse_build_graph_dfs(const struct bio_graph_vertex* vert, bool* visited_vert,
                                                 struct bio_graph* new_graph, int* num_vert)
{
        // copy current vertex
        struct bio_graph_list* list = vert->head;
        while (list->list_next) {
                bio_graph_make_edge_undirected(new_graph, vert->id, list->vert_next->id);
                list = list->list_next;
        }
        // mark visited and recurse
        list = vert->head;
        while (list->list_next) {
                if (!visited_vert[list->vert_next->id]) {
                        visited_vert[list->vert_next->id] = true;
                        *num_vert = *num_vert + 1;
                        __bio_graph_traverse_build_graph_dfs(list->vert_next, visited_vert, new_graph, num_vert);
                }
                list = list->list_next;
        }
}

struct bio_graph* bio_graph_get_connected_components(const struct bio_graph* self, int* n_comps)
{
        bool* visited_vert = malloc(sizeof(*visited_vert)*self->num_verts);
        int i;
        for (i = 0; i < self->num_verts; i ++) {
                visited_vert[i] = false;
        }
        struct bio_graph* new_graphs = malloc(sizeof(*new_graphs)*self->num_verts);
        int num_connected = 0;
        for (i = 0; i < self->num_verts; i ++) {
                if (!visited_vert[i]) {
                        visited_vert[i] = true;
                        __bio_graph_init(&new_graphs[num_connected], self->num_verts);
                        int num_verts = 0;
                        __bio_graph_traverse_build_graph_dfs(&self->verts[i], visited_vert, &new_graphs[num_connected], &num_verts);
                        new_graphs[num_connected].num_verts = num_verts;
                        num_connected ++;
                }
        }
        free(visited_vert);
        *n_comps = num_connected;
        struct bio_graph* o = malloc(sizeof(*o)*num_connected);
        for (i = 0; i < num_connected; i ++) {
                o[i] = new_graphs[i];
        }
        free(new_graphs);
        return o;
}

static void __bio_graph_traverse_dfs(const struct bio_graph_vertex* vert, bool* visited_vert)
{
        // mark visited and recurse
        struct bio_graph_list* list = vert->head;
        while (list->list_next) {
                if (!visited_vert[list->vert_next->id]) {
                        visited_vert[list->vert_next->id] = true;
                        __bio_graph_traverse_dfs(list->vert_next, visited_vert);
                }
                list = list->list_next;
        }
}

int bio_graph_count_connected_components(const struct bio_graph* self)
{
        bool* visited_vert = malloc(sizeof(*visited_vert)*self->num_verts);
        int i;
        for (i = 0; i < self->num_verts; i ++) {
                visited_vert[i] = false;
        }
        int num_connected = 0;
        for (i = 0; i < self->num_verts; i ++) {
                if (!visited_vert[i]) {
                        visited_vert[i] = true;
                        __bio_graph_traverse_dfs(&self->verts[i], visited_vert);
                        num_connected ++;
                }
        }
        free(visited_vert);
        return num_connected;
}

int* bio_graph_find_deg_distri(const struct bio_graph* self, int* num_distri)
{
        int* distri = malloc(sizeof(*distri)*(self->num_verts));    // assuming simple, max(deg(v)) == n - 1
        int i;
        for (i = 0; i < self->num_verts; i ++) {
                distri[i] = 0;
        }
        for (i = 0; i < self->num_verts; i ++) {
                distri[self->verts[i].degree] ++;
        }
        *num_distri = self->num_verts;
        return distri;
}

struct bio_graph* bio_graph_get_graal_alignment(struct bio_graph* g, struct bio_graph* h)
{
        return nullptr;
}

struct bio_graph* bio_graph_get_sana_alignment(struct bio_graph* g, struct bio_graph* h)
{
        return nullptr;
}

int bio_graph_get_vertex_num(const struct bio_graph* g)
{
        return g->num_verts;
}

static void __bio_graph_traverse_dfs2(const struct bio_graph_vertex* vert, bool* visited_vert,
                                      f_Bio_Graph_Edge_Visitor visitor, void* user_data)
{
        // copy current vertex
        struct bio_graph_list* list = vert->head;
        while (list->list_next) {
                visitor(vert, list->vert_next, user_data);
                list = list->list_next;
        }
        // mark visited and recurse
        list = vert->head;
        while (list->list_next) {
                if (!visited_vert[list->vert_next->id]) {
                        visited_vert[list->vert_next->id] = true;
                        __bio_graph_traverse_dfs2(list->vert_next, visited_vert, visitor, user_data);
                }
                list = list->list_next;
        }
}

void bio_graph_visit_edges(const struct bio_graph* self, f_Bio_Graph_Edge_Visitor visitor, void* user_data)
{
        bool* visited_vert = malloc(sizeof(*visited_vert)*self->num_verts);
        int i;
        for (i = 0; i < self->num_verts; i ++) {
                visited_vert[i] = false;
        }
        for (i = 0; i < self->num_verts; i ++) {
                if (!visited_vert[i]) {
                        visited_vert[i] = true;
                        __bio_graph_traverse_dfs2(&self->verts[i], visited_vert, visitor, user_data);
                }
        }
        free(visited_vert);
}

void bio_graph_visit_vertices(const struct bio_graph* self, f_Bio_Graph_Vertex_Visitor visitor, void* user_data)
{
        int i;
        for (i = 0; i < self->num_verts; i ++) {
                visitor(&self->verts[i], user_data);
        }
}

int bio_graph_vertex_get_id(const struct bio_graph_vertex* self)
{
        return self->id;
}

int bio_graph_vertex_get_degree(const struct bio_graph_vertex* self)
{
        return self->degree;
}

void bio_graph_vertex_bind_data(struct bio_graph_vertex* self, void* data)
{
        self->data = data;
}

void* bio_graph_vertex_retrieve_data(const struct bio_graph_vertex* self)
{
        return self->data;
}
