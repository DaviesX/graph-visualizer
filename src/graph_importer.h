#ifndef GRAPH_IMPORTER_H_INCLUDED
#define GRAPH_IMPORTER_H_INCLUDED


struct bio_graph* graph_importer_read_txt_file(const char* filename);
struct bio_graph* graph_importer_read_gexf_file(const char* filename);
struct bio_graph* graph_importer_read_gw_file(const char* filename);


#endif // GRAPH_IMPORTER_H_INCLUDED
