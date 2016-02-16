#ifndef GRAPH_EXPORTER_H_INCLUDED
#define GRAPH_EXPORTER_H_INCLUDED

bool graph_exporter_write_distri(const int* collection, const int num_coll, const char* filename);
bool graph_exporter_write_distri2(const int* collection, const int num_coll, FILE* f);
bool graph_exporter_write_ppm_image(const void* image, const int width, const int height, const int ps, const char* filename);
bool graph_exporter_write_txt_file(const struct bio_graph* self, const char* filename);
bool graph_exporter_write_gexf_file(const struct bio_graph* self, const char* filename);
bool graph_exporter_write_gw_file(const struct bio_graph* self, const char* filename);


#endif // GRAPH_EXPORTER_H_INCLUDED
