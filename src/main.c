#include "common.h"
#include "bio_graph.h"
#include "graph_importer.h"
#include "graph_exporter.h"
#include "graph_display.h"


enum OperationType {
        OperationMayday,
        OperationFunTest,
        OperationConversion,
        OperationDisplayGraph,
        OperationAlignGraph,
        OperationGenerateGraphImage,
};

struct config_file {
        enum OperationType      op_type;
        char*                   g_graph;
        char*                   h_graph;
        char*                   acc_struct;
        char*                   graph_image;
        char*                   graph_width;
        char*                   graph_height;
        char*                   graph_converted;
        char*                   graph_alignment_method;
        int*                    argc;
        char***                 argv;
};

static void mayday()
{
        puts("Usage: bio-graph [Options] GraphInputs... ");
        puts("Options:");
        puts("\t--help");
        puts("\t--test");
        puts("\t--convert");
        puts("\t--display");
        puts("\t--align");
        puts("\t--generate-image");
}

static const char*              __get_file_suffix(const char* filename);
static const char*              __get_file_name(const char* path);
static struct bio_graph*        __read_graph_file(const char* filename);
static bool                     __write_graph_file(struct bio_graph* graph, const char* filename);


static struct bio_graph* __read_graph_file(const char* filename)
{
        struct bio_graph* graph;
        if (filename == nullptr) {
                puts("graph input is not specified");
                mayday();
                return nullptr;
        } else {
                const char* suffix = __get_file_suffix(filename);
                if (!strcmp("txt", suffix)) {
                        graph = graph_importer_read_txt_file(filename);
                } else if (!strcmp("gexf", suffix)) {
                        graph = graph_importer_read_gexf_file(filename);
                } else if (!strcmp("gw", suffix)) {
                        graph = graph_importer_read_gw_file(filename);
                } else {
                        printf("cannot recognize the file format of %s\n", filename);
                        return nullptr;
                }
                if (!graph) {
                        printf("failed to load the file %s\n", filename);
                        return nullptr;
                }
        }
        return graph;
}

static bool __write_graph_file(struct bio_graph* graph, const char* filename)
{
        if (filename == nullptr) {
                puts("graph output is not specified");
                mayday();
        } else {
                const char* suffix = __get_file_suffix(filename);
                if (!strcmp("txt", suffix)) {
                        if (!graph_exporter_write_txt_file(graph, filename)) {
                                return false;
                        }
                } else if (!strcmp("gexf", suffix)) {
                        if (!graph_exporter_write_gexf_file(graph, filename)) {
                                return false;
                        }
                } else if (!strcmp("gw", suffix)) {
                        if (!graph_exporter_write_gw_file(graph, filename)) {
                                return false;
                        }
                } else {
                        printf("cannot recognize the file format of %s\n", filename);
                        return false;
                }
        }
        printf("the file has been saved to: %s\n", filename);
        return true;
}

static const char* __get_file_name(const char* path)
{
        const char* s = path;
        while(*s != '\0') s ++;
        while(s != path) {
                if (*s == '/') return ++ s;
                s --;
        }
        return path;
}

// test on the basic data structures
static void test(struct config_file* cfg)
{
        puts("\ntest is launching...");

        static const char* tests[] = {
                "./gexf_graph/athal.gexf",
                "./gexf_graph/cjejuni.gexf",
                "./gexf_graph/dmel.gexf",
                "./gexf_graph/ecoli.gexf",
                "./gexf_graph/scere05.gexf",
                "./gexf_graph/scere10.gexf",
                "./gexf_graph/scere15.gexf",
                "./gexf_graph/scere20.gexf",
                "./gexf_graph/scerehc.gexf",
                "./txt_graph/n10.txt",
                "./txt_graph/n100.txt",
                "./txt_graph/n1000.txt",
                "./txt_graph/n10000.txt",
                "./txt_graph/s1.txt",
        };
        unsigned i;
        for (i = 0; i < sizeof(tests)/sizeof(char*); i ++) {
                char res_file_name[32];
                sprintf(res_file_name, "./test_result/%s.test", __get_file_name(tests[i]));
                FILE* fres = fopen(res_file_name, "w");
                assert(fres);

                struct bio_graph* g = __read_graph_file(tests[i]);
                assert(g);

                fprintf(fres, "the number of connected components is: %d\n", bio_graph_count_connected_components(g));
                int n;
                int* collection = bio_graph_find_deg_distri(g, &n);
                graph_exporter_write_distri2(collection, n, fres);

                free(collection);
                bio_graph_free(g);
                fprintf(fres, "==========result for %s ========\n\n", tests[i]);
                fclose(fres);
        }

        puts("tests have been run");
}

static void conversion(struct config_file* cfg)
{
        puts("converting graph file...");

        // load in the graph
        struct bio_graph* graph = __read_graph_file(cfg->g_graph);
        if (graph == nullptr) goto failed;

        // save the graph
        if (!__write_graph_file(graph, cfg->graph_converted)) goto failed;
failed:
        bio_graph_free(graph);
}

static void display_graph(struct config_file* cfg)
{
        puts("displaying the graph...");

        struct graph_display* display;
        if (cfg->acc_struct) {
                if (!strcmp("none", cfg->acc_struct)) {
                        display = graph_display_create(AccelerateMethodNone);
                } else if (!strcmp("grid", cfg->acc_struct)) {
                        display = graph_display_create(AccelerateMethodGrid);
                } else if (!strcmp("FADE", cfg->acc_struct)) {
                        display = graph_display_create(AccelerateMethodFADE);
                } else {
                        printf("no such accelerating structure as: %s\n", cfg->acc_struct);
                        mayday();
                        return ;
                }
        } else {
                display = graph_display_create(AccelerateMethodNone);
        }

        // load in the graph
        struct bio_graph* graph = __read_graph_file(cfg->g_graph);
        if (graph == nullptr) goto failed;

        graph_display_set_dimension(display, atoi(cfg->graph_width), atoi(cfg->graph_height));
        graph_display_progressive_draw_to_gtk_screen(display, graph, nullptr, cfg->argc, cfg->argv);
failed:
        graph_display_free(display);
        bio_graph_free(graph);
}

static void align_graph(struct config_file* cfg)
{
}

static const char* __get_file_suffix(const char* filename)
{
        const char* s = filename;
        while(*s != '\0') s ++;
        while(s != filename) {
                if (*s == '.') return ++ s;
                s --;
        }
        return "";
}

static void generate_graph_image(struct config_file* cfg)
{
        puts("generating graph image...");
        struct graph_display* display;
        if (cfg->acc_struct) {
                if (!strcmp("none", cfg->acc_struct)) {
                        display = graph_display_create(AccelerateMethodNone);
                } else if (!strcmp("grid", cfg->acc_struct)) {
                        display = graph_display_create(AccelerateMethodGrid);
                } else if (!strcmp("FADE", cfg->acc_struct)) {
                        display = graph_display_create(AccelerateMethodFADE);
                } else {
                        printf("no such accelerating structure as: %s\n", cfg->acc_struct);
                        mayday();
                        return ;
                }
        } else {
                display = graph_display_create(AccelerateMethodNone);
        }

        // load in the graph
        struct bio_graph* graph = __read_graph_file(cfg->g_graph);
        if (graph == nullptr) goto failed;

        // display it
        graph_display_set_dimension(display, atoi(cfg->graph_width), atoi(cfg->graph_height));
        graph_display_force_directed(display, graph, 20000);
        graph_display_rasterize(display);
        int w, h, s;
        const void* image = graph_display_fetch_memory(display, &w, &h, &s);
        if (!graph_exporter_write_ppm_image(image, w, h, s, cfg->graph_image)) {
                goto failed;
        }
        printf("the image has been saved to: %s\n", cfg->graph_image);
failed:
        graph_display_free(display);
        bio_graph_free(graph);
}

int main(int argc, char* argv[])
{
        struct config_file cfg = {0};
        // fill in configuration from arguments
        cfg.argc = &argc;
        cfg.argv = &argv;
        int i;
        for (i = 1; i < argc; i ++) {
                if (!strcmp("--test", argv[i]) || !strcmp("-t", argv[i])) {
                        cfg.op_type = OperationFunTest;
                } else if (!strcmp("--convert", argv[i]) || !strcmp("-c", argv[i])) {
                        cfg.op_type                     = OperationConversion;
                } else if (!strcmp("--generate-display", argv[i]) || !strcmp("-gd", argv[i])) {
                        if (i + 2 >= argc || !strncmp("-", argv[i + 1], 1) || !strncmp("-", argv[i + 2], 1)) {
                                puts("not enough arguments for --generate-image");
                                cfg.op_type = OperationMayday;
                                break;
                        }
                        cfg.op_type = OperationDisplayGraph;
                        cfg.graph_width  = argv[i + 1];
                        cfg.graph_height = argv[i + 2];
                        i += 2;
                } else if (!strcmp("--generate-image", argv[i]) || !strcmp("-gi", argv[i])) {
                        if (i + 3 >= argc || !strncmp("-", argv[i + 1], 1) ||
                            !strncmp("-", argv[i + 2], 1) || !strncmp("-", argv[i + 3], 1)) {
                                puts("not enough arguments for --generate-image");
                                cfg.op_type = OperationMayday;
                                break;
                        }
                        cfg.op_type = OperationGenerateGraphImage;
                        cfg.graph_image  = argv[i + 1];
                        cfg.graph_width  = argv[i + 2];
                        cfg.graph_height = argv[i + 3];
                        i += 3;
                } else if (!strcmp("--output", argv[i]) || !strcmp("-o", argv[i])) {
                        if (i + 1 >= argc || !strncmp("-", argv[i + 1], 1)) {
                                puts("not enough arguments for --output");
                                cfg.op_type = OperationMayday;
                                break;
                        } else {
                                i += 1;
                                cfg.graph_converted = argv[i];
                        }
                } else if (!strcmp("--align", argv[i]) || !strcmp("-a", argv[i])) {
                        if (i + 1 >= argc || !strncmp("-", argv[i + 1], 1)) {
                                cfg.graph_alignment_method = "sana";
                        } else {
                                cfg.graph_alignment_method = argv[i + 1];
                                i += 1;
                        }
                        cfg.op_type = OperationAlignGraph;
                } else if (!strcmp("--accelerate-structure", argv[i])) {
                        if (i + 1 >= argc || !strncmp("-", argv[i + 1], 1)) {
                                puts("not enough arguments for --accelerate-structure");
                                cfg.op_type = OperationMayday;
                                break;
                        }
                        cfg.acc_struct = argv[i + 1];
                        i += 1;
                } else if (!strcmp("--help", argv[i]) || !strcmp("-h", argv[i])) {
                        cfg.op_type = OperationMayday;
                        break;
                } else {
                        if (!strncmp("-", argv[i], 1)) {
                                puts("invalid argument");
                                cfg.op_type = OperationMayday;
                                break;
                        } else {
                                if (!cfg.g_graph) {
                                        cfg.g_graph = argv[i];
                                } else {
                                        cfg.h_graph = argv[i];
                                }
                        }
                }
        }
        // interpret configuration and run
        clock_t start = clock();
        switch (cfg.op_type) {
        case OperationMayday:
                mayday(&cfg);
                break;
        case OperationFunTest:
                test(&cfg);
                break;
        case OperationConversion:
                conversion(&cfg);
                break;
        case OperationDisplayGraph:
                display_graph(&cfg);
                break;
        case OperationAlignGraph:
                align_graph(&cfg);
                break;
        case OperationGenerateGraphImage:
                generate_graph_image(&cfg);
                break;
        }
        clock_t end = clock();
        float t = (end - start)/(float) CLOCKS_PER_SEC;
        printf("Time used: %f\n", t);
        return 0;
}
