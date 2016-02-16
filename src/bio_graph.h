// util_graph.h Wen, Chifeng - Sept. 26, 2015
#ifndef UTIL_GRAPH_H_INCLUDED
#define UTIL_GRAPH_H_INCLUDED


struct bio_graph_list;
struct bio_graph_vertex;
struct bio_graph;

typedef void (*f_Bio_Graph_Edge_Visitor) (const struct bio_graph_vertex* v0, const struct bio_graph_vertex* v1, void* user_data);
typedef void (*f_Bio_Graph_Vertex_Visitor) (const struct bio_graph_vertex* v, void* user_data);


struct bio_graph*       bio_graph_create(int num_verts);
void                    bio_graph_free(struct bio_graph* self);
void                    bio_graph_make_edge_undirected(struct bio_graph* self, int v0, int v1);
struct bio_graph*       bio_graph_get_connected_components(const struct bio_graph* self, int* n_comps);
int                     bio_graph_count_connected_components(const struct bio_graph* self);
int*                    bio_graph_find_deg_distri(const struct bio_graph* self, int* num_distri);
struct bio_graph*       bio_graph_get_graal_alignment(struct bio_graph* g, struct bio_graph* h);
struct bio_graph*       bio_graph_get_sana_alignment(struct bio_graph* g, struct bio_graph* h);

int                     bio_graph_get_vertex_num(const struct bio_graph* g);
void                    bio_graph_visit_edges(const struct bio_graph* self, f_Bio_Graph_Edge_Visitor visitor, void* user_data);
void                    bio_graph_visit_vertices(const struct bio_graph* self, f_Bio_Graph_Vertex_Visitor visitor, void* user_data);

int                     bio_graph_vertex_get_id(const struct bio_graph_vertex* self);
int                     bio_graph_vertex_get_degree(const struct bio_graph_vertex* self);
void                    bio_graph_vertex_bind_data(struct bio_graph_vertex* self, void* data);
void*                   bio_graph_vertex_retrieve_data(const struct bio_graph_vertex* self);


#endif // UTIL_GRAPH_H_INCLUDED
