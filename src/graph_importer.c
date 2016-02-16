#include "common.h"
#include "bio_graph.h"
#include "graph_importer.h"


#define c_MaxLineLength         256

struct bio_graph* graph_importer_read_txt_file(const char* filename)
{
        FILE* f;
        f = fopen(filename, "r");
        if (f == nullptr) {
                printf("cannot open txt graph file: %s\n", filename);
                return nullptr;
        }

        int num_nodes;
        if (1 != fscanf(f, "%d", &num_nodes)) {
                printf("bad txt graph file: %s\n", filename);
                fclose(f);
                return nullptr;
        }

        struct bio_graph* self = bio_graph_create(num_nodes);

        while (!feof(f)) {
                int v0, v1;
                if (2 != fscanf(f, "%d %d", &v0, &v1)) {
                        continue;
                }
                bio_graph_make_edge_undirected(self, v0, v1);
        }

        fclose(f);
        return self;
}

static const char* strip_prespace(const char* line)
{
        while(*line == ' ') line ++;
        return line;
}

static void free_node_dict(char** node_dict, int num_nodes)
{
        int i;
        for (i = 0; i < num_nodes; i ++) {
                free(node_dict[i]);
                node_dict[i] = nullptr;
        }
        free(node_dict);
}

struct bio_graph* graph_importer_read_gexf_file(const char* filename)
{
        FILE* f = fopen(filename, "r");
        if (f == nullptr) {
                printf("cannot open gexf graph file: %s\n", filename);
                return nullptr;
        }
        // check if it is a xml file
        char header[c_MaxLineLength];
        if (!fgets(header, c_MaxLineLength, f)) {
                printf("bad gexf graph file: %s\n", filename);
                fclose(f);
                return nullptr;
        }
        if (0 != strncmp("<?xml version=", header, strlen("<?xml version="))) {
                printf("bad gexf graph file: %s\n", filename);
                fclose(f);
                return nullptr;
        }
        // on the first pass, scan the number of noes
        int num_nodes = 0;
        while (!feof(f)) {
                char buffer[c_MaxLineLength];
                if (!fgets(buffer, c_MaxLineLength, f)) {
                        //continue;
                }
                const char* line = strip_prespace(buffer);
                if (!strncmp("<node id=", line, strlen("<node id="))) {
                        num_nodes ++;
                }
        }
        struct bio_graph* self = bio_graph_create(num_nodes);
        // on the second pass, read node id into dictionary
        int k = 0;
        char** node_dict = malloc(sizeof(*node_dict)*num_nodes);
        fseek(f, 0, SEEK_SET);
        while (!feof(f)) {
#define c_MaxIdLength           32
                char buffer[c_MaxLineLength];
                if (!fgets(buffer, c_MaxLineLength, f)) {
                        continue;
                }
                const char* line = strip_prespace(buffer);
                if (!strncmp("<node id=", line, strlen("<node id="))) {
                        // sample line: <node id="Q9LZV6" label="Q9LZV6">
                        char id_string[c_MaxIdLength] = {0};
                        char rest_string[c_MaxLineLength];
                        sscanf(line, "<node id=%s %s", id_string, rest_string);
                        node_dict[k] = malloc(strlen(id_string) + 1);
                        memset(node_dict[k], 0, strlen(id_string) + 1);
                        strncpy(node_dict[k], id_string, strlen(id_string));
                        k ++;
                }
        }
        // on the third pass, extract edge info and build the graph
        fseek(f, 0, SEEK_SET);
        while (!feof(f)) {
                char buffer[c_MaxLineLength];
                if (!fgets(buffer, c_MaxLineLength, f)) {
                        continue;
                }
                const char* line = strip_prespace(buffer);
                if (!strncmp("<edge id=", line, strlen("<edge id="))) {
                        // sample line: <edge id="0" source="Q8L765" target="Q94B33" weight="1.1" />
                        char edge_id_string[c_MaxIdLength] = {0};
                        char source_id_string[c_MaxIdLength] = {0};
                        char dest_id_string[c_MaxIdLength] = {0};
                        char rest_id_string[c_MaxLineLength] = {0};
                        sscanf(line, "<edge id=%s source=%s target=%s %s",
                               edge_id_string, source_id_string, dest_id_string, rest_id_string);
                        // linear search over the dictionary
                        int source_id = -1, dest_id = -1;
                        int i;
                        for (i = 0; i < num_nodes; i ++) {
                                if (!strcmp(node_dict[i], source_id_string)) {
                                        source_id = i;
                                        break;
                                }
                        }
                        for (i = 0; i < num_nodes; i ++) {
                                if (!strcmp(node_dict[i], dest_id_string)) {
                                        dest_id = i;
                                        break;
                                }
                        }
                        if (source_id == -1 || dest_id == -1) {
                                printf("bad gexf graph file: %s\n", filename);
                                fclose(f);
                                free_node_dict(node_dict, num_nodes);
                                bio_graph_free(self);
                                return nullptr;
                        }
                        bio_graph_make_edge_undirected(self, source_id, dest_id);
                }
        }
        // release resources
        free_node_dict(node_dict, num_nodes);
        fclose(f);
        return self;
}

static bool is_safe_line(const char* line)
{
        const char* s = line;
        while(*s != '\n') {
                if (*s == '#') {
                        return false;
                }
                s ++;
        }
        if (s == line) {
                return false;
        }
        return true;
}

struct bio_graph* graph_importer_read_gw_file(const char* filename)
{
        FILE* f;
        f = fopen(filename, "r");
        if (f == nullptr) {
                printf("cannot open gw graph file: %s\n", filename);
                return nullptr;
        }
        char buffer[c_MaxLineLength];
        // verify header
        if (!fgets(buffer, c_MaxLineLength, f) || 0 != strcmp("LEDA.GRAPH\n", buffer)) {
                printf("bad LEDA(.gw) graph file: %s\n", filename);
                fclose(f);
                return nullptr;
        }
        int i;
        for (i = 0; i < 3; i ++) {
                // ignore the rest of header section
                if (!fgets(buffer, c_MaxLineLength, f)) {
                        printf("bad LEDA(.gw) graph file: %s\n", filename);
                        fclose(f);
                        return nullptr;
                }
        }
        // node section
        int num_nodes;
        if (1 != fscanf(f, "%d", &num_nodes)) {
                fclose(f);
                printf("bad LEDA(.gw) graph file: %s\n", filename);
                return nullptr;
        }
        for (i = 0; i < num_nodes; i ++) {
                if (!fgets(buffer, c_MaxLineLength, f)) {
                        fclose(f);
                        printf("bad LEDA(.gw) graph file: %s\n", filename);
                        return nullptr;
                }
                if (!is_safe_line(buffer)) i --;
                if (feof(f)) {
                        fclose(f);
                        printf("bad LEDA(.gw) graph file: %s\n", filename);
                        return nullptr;
                }
        }

        struct bio_graph* self = bio_graph_create(num_nodes);

        // edge section
        int num_edge;
        if (1 != fscanf(f, "%d", &num_edge)) {
                fclose(f);
                bio_graph_free(self);
                printf("bad LEDA(.gw) graph file: %s\n", filename);
                return nullptr;
        }
        while (!feof(f)) {
                int v0, v1;
#define c_MaxUnusedLength       32
                char unused[c_MaxUnusedLength];
                if (!fgets(buffer, c_MaxLineLength, f) && feof(f)) {
                        continue;
                }
                if (3 != sscanf(buffer, "%d %d %s", &v0, &v1, unused)) {
                        continue;
                }
                bio_graph_make_edge_undirected(self, v0 - 1, v1 - 1);
        }

        fclose(f);
        return self;
}
