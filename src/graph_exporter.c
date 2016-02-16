#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>

extern "C" {
#include "common.h"
#include "bio_graph.h"
#include "graph_display.h"
#include "graph_exporter.h"
}

bool graph_exporter_write_distri2(const int* collection, const int num_coll, FILE* f)
{
        int i;
        for (i = 0; i < num_coll; i ++) {
                fprintf(f, "%d\t%d\n", i, collection[i]);
        }
        return true;
}

bool graph_exporter_write_distri(const int* collection, const int num_coll, const char* filename)
{
        FILE* fres = fopen(filename, "w+");
        if (fres == nullptr) {
                printf("failed to write distribution to the file: %s\n", filename);
                return false;
        }
        if (!graph_exporter_write_distri2(collection, num_coll, fres)) {
                fclose(fres);
                return false;
        }
        fclose(fres);
        return true;
}

bool graph_exporter_write_ppm_image(const void* image, const int width, const int height, const int ps, const char* filename)
{
        assert(image);

        const uint8_t* buffer = static_cast<const uint8_t*>(image);
        FILE *f = fopen(filename, "w+");
        if (f == nullptr) {
                printf("failed to write ppm image to the file: %s\n", filename);
                return false;
        }
        fprintf(f, "P3\n%d %d\n%d\n", width, height, 255);
        int i, j;
        for (i = 0; i < height; i ++) {
                for (j = 0; j < width; j ++) {
                        fprintf(f, "%d %d %d ", buffer[(j + i*width)*ps + 2],
                                buffer[(j + i*width)*ps + 1],
                                buffer[(j + i*width)*ps + 0]);
                }
        }
        fclose(f);
        return true;
}

static void __txt_write_edge_visitor(const struct bio_graph_vertex* v0, const struct bio_graph_vertex* v1, void* file_ptr)
{
        fprintf(static_cast<FILE*>(file_ptr), "%d %d\n", bio_graph_vertex_get_id(v0), bio_graph_vertex_get_id(v1));
}

bool graph_exporter_write_txt_file(const struct bio_graph* self, const char* filename)
{
        assert(self);

        FILE *f = fopen(filename, "w+");
        if (f == nullptr) {
                printf("failed to write graph to the file: %s\n", filename);
                return false;
        }
        fprintf(f, "%d\n", bio_graph_get_vertex_num(self));
        bio_graph_visit_edges(self, __txt_write_edge_visitor, f);
        fclose(f);
        return true;
}

bool graph_exporter_write_gexf_file(const struct bio_graph* self, const char* filename)
{
        assert(self);

        try {
                xercesc::XMLPlatformUtils::Initialize();
        } catch (const xercesc::XMLException& toCatch) {
                printf("failed to initiailze xercesc\n");
                return false;
        }
        xercesc::DOMImplementation* impl = xercesc::DOMImplementation::getImplementation();
        xercesc::DOMLSOutput* output_desc = ((xercesc::DOMImplementationLS*) impl)->createLSOutput();
        xercesc::XMLFormatTarget* target = new xercesc::LocalFileFormatTarget(xercesc::XMLString::transcode(filename));

        // write header
        xercesc::DOMDocument* doc = impl->createDocument();
        xercesc::DOMElement* graph_elm = doc->createElement(xercesc::XMLString::transcode("graph"));
        // <attributes class="node" mode="static">
        //  <attribute id="0" title="gname" type="string" />
        // </attributes>
        xercesc::DOMNode* attris = graph_elm->appendChild(doc->createTextNode(xercesc::XMLString::transcode("attributes")));

        // node section
        int i;
        for (i = 0; i < bio_graph_get_vertex_num(self); i ++) {
        }

        // edge section
        int num_edge = 0;
        bio_graph_visit_edges(self, __edge_count_visitor, &num_edge);
        fprintf(f, "%d\n", num_edge);
        bio_graph_visit_edges(self, __gw_edge_writer_visitor, f);
        fclose(f);

        delete target;
        xercesc::XMLPlatformUtils::Terminate();
        return false;
}

static void __edge_count_visitor(const struct bio_graph_vertex* v0, const struct bio_graph_vertex* v1, void* edge_num)
{
        int e = *(int*) edge_num;
        *(int*) edge_num = e + 1;
}

static void __gw_edge_writer_visitor(const struct bio_graph_vertex* v0, const struct bio_graph_vertex* v1, void* file_ptr)
{
        fprintf(static_cast<FILE*>(file_ptr), "%d %d 0 |{}|\n", bio_graph_vertex_get_id(v0) + 1, bio_graph_vertex_get_id(v1) + 1);
}

bool graph_exporter_write_gw_file(const struct bio_graph* self, const char* filename)
{
        assert(self);

        FILE* f;
        f = fopen(filename, "w+");
        if (f == nullptr) {
                printf("failed to write graph to the file: %s\n", filename);
                return false;
        }
        // write header
        fprintf(f, "LEDA.GRAPH\n");
        fprintf(f, "string\n");
        fprintf(f, "int\n");
        fprintf(f, "-2\n");

        // node section
        fprintf(f, "%d\n", bio_graph_get_vertex_num(self));
        int i;
        for (i = 0; i < bio_graph_get_vertex_num(self); i ++) {
                fprintf(f, "|{%d}|\n", i);
        }

        // edge section
        int num_edge = 0;
        bio_graph_visit_edges(self, __edge_count_visitor, &num_edge);
        fprintf(f, "%d\n", num_edge);
        bio_graph_visit_edges(self, __gw_edge_writer_visitor, f);
        fclose(f);
        return true;
}
