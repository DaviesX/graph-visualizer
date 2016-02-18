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

#include <string>

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

class GEXFContext
{
public:
        GEXFContext(xercesc::DOMElement* container, xercesc::DOMDocument* doc) : m_doc(doc), m_container(container)
        {}

        xercesc::DOMDocument* get_document()
        {
                return m_doc;
        }

        xercesc::DOMElement* get_container()
        {
                return m_container;
        }

        int next_global_edge_count()
        {
                return m_glb_edges_count ++;
        }
private:
        xercesc::DOMDocument*   m_doc;
        xercesc::DOMElement*    m_container;
        int                     m_glb_edges_count = 0;
};

static void __gexf_write_node_visitor(const struct bio_graph_vertex* v, void* data)
{
        int vid = bio_graph_vertex_get_id(v);

        GEXFContext* gexfctx = static_cast<GEXFContext*>(data);
        // <node id="Q8L765" label="Q8L765">
        //  <attvalues>
        //   <attvalue for="0" value="Q8L765" />
        //  </attvalues>
        // </node>
        xercesc::DOMElement* node = gexfctx->get_document()->createElement(xercesc::XMLString::transcode("node"));
        node->setAttribute(xercesc::XMLString::transcode("id"),
                           xercesc::XMLString::transcode(std::to_string(vid).c_str()));
        node->setAttribute(xercesc::XMLString::transcode("label"),
                           xercesc::XMLString::transcode(std::to_string(vid).c_str()));

        xercesc::DOMElement* node_attrs = gexfctx->get_document()->createElement(xercesc::XMLString::transcode("attributes"));
        xercesc::DOMElement* node_attr = gexfctx->get_document()->createElement(xercesc::XMLString::transcode("attribute"));
        node_attr->setAttribute(xercesc::XMLString::transcode("for"), xercesc::XMLString::transcode("0"));
        node_attr->setAttribute(xercesc::XMLString::transcode("value"),
                                xercesc::XMLString::transcode(std::to_string(vid).c_str()));
        node_attrs->appendChild(node_attr);
        node->appendChild(node_attrs);
        gexfctx->get_container()->appendChild(node);
}

static void __gexf_write_edge_visitor(const struct bio_graph_vertex* v0, const struct bio_graph_vertex* v1, void* data)
{
        int v0id = bio_graph_vertex_get_id(v0);
        int v1id = bio_graph_vertex_get_id(v1);

        GEXFContext* gexfctx = static_cast<GEXFContext*>(data);
        // <edge id="0" source="Q8L765" target="Q94B33" weight="1.1" />
        xercesc::DOMElement* edge = gexfctx->get_document()->createElement(xercesc::XMLString::transcode("edge"));
        edge->setAttribute(xercesc::XMLString::transcode("id"),
                           xercesc::XMLString::transcode(std::to_string(gexfctx->next_global_edge_count()).c_str()));
        edge->setAttribute(xercesc::XMLString::transcode("source"),
                           xercesc::XMLString::transcode(std::to_string(v0id).c_str()));
        edge->setAttribute(xercesc::XMLString::transcode("target"),
                           xercesc::XMLString::transcode(std::to_string(v1id).c_str()));
        edge->setAttribute(xercesc::XMLString::transcode("weight"),
                           xercesc::XMLString::transcode("1.0"));
        gexfctx->get_container()->appendChild(edge);
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


        xercesc::DOMImplementation* impl = xercesc::DOMImplementationRegistry::getDOMImplementation(
                        xercesc::XMLString::transcode("LS"));
        // write header
        xercesc::DOMDocument* doc = impl->createDocument();
        xercesc::DOMElement* gexf_header = doc->createElement(xercesc::XMLString::transcode("gexf"));
        gexf_header->setAttribute(xercesc::XMLString::transcode("xmlns"),
                                  xercesc::XMLString::transcode("http://www.gexf.net/1.1draft"));
        gexf_header->setAttribute(xercesc::XMLString::transcode("version"),
                                  xercesc::XMLString::transcode("1.1"));
        doc->appendChild(gexf_header);
        // <graph defaultedgetype="undirected" mode="static">
        xercesc::DOMElement* graph_elm = doc->createElement(xercesc::XMLString::transcode("graph"));
        graph_elm->setAttribute(xercesc::XMLString::transcode("defaultedgetype"), xercesc::XMLString::transcode("undirected"));
        graph_elm->setAttribute(xercesc::XMLString::transcode("mode"), xercesc::XMLString::transcode("static"));
        gexf_header->appendChild(graph_elm);
        // <attributes class="node" mode="static">
        //  <attribute id="0" title="gname" type="string" />
        // </attributes>
        xercesc::DOMElement* attris = doc->createElement(xercesc::XMLString::transcode("attributes"));
        attris->setAttribute(xercesc::XMLString::transcode("class"), xercesc::XMLString::transcode("node"));
        attris->setAttribute(xercesc::XMLString::transcode("mode"), xercesc::XMLString::transcode("static"));
        graph_elm->appendChild(attris);
        xercesc::DOMElement* attri0 = doc->createElement(xercesc::XMLString::transcode("attribute"));
        attri0->setAttribute(xercesc::XMLString::transcode("id"), xercesc::XMLString::transcode("0"));
        attri0->setAttribute(xercesc::XMLString::transcode("title"), xercesc::XMLString::transcode("gname"));
        attri0->setAttribute(xercesc::XMLString::transcode("type"), xercesc::XMLString::transcode("string"));
        attris->appendChild(attri0);

        // node section
        xercesc::DOMElement* nodes = doc->createElement(xercesc::XMLString::transcode("nodes"));
        GEXFContext node_ctx(nodes, doc);
        bio_graph_visit_vertices(self, __gexf_write_node_visitor, &node_ctx);
        graph_elm->appendChild(nodes);

        // edge section
        xercesc::DOMElement* edges = doc->createElement(xercesc::XMLString::transcode("edges"));
        GEXFContext edge_ctx(edges, doc);
        bio_graph_visit_edges(self, __gexf_write_edge_visitor, &edge_ctx);
        graph_elm->appendChild(edges);

        // serialize the DOM tree
        xercesc::XMLFormatTarget* file_target = new xercesc::LocalFileFormatTarget(xercesc::XMLString::transcode(filename));
        xercesc::DOMLSOutput* output_desc = ((xercesc::DOMImplementationLS*) impl)->createLSOutput();
        output_desc->setByteStream(file_target);

        xercesc::DOMLSSerializer* serializer = ((xercesc::DOMImplementationLS*) impl)->createLSSerializer();
        if (serializer->getDomConfig()->canSetParameter(xercesc::XMLUni::fgDOMWRTFormatPrettyPrint, true))
                serializer->getDomConfig()->setParameter(xercesc::XMLUni::fgDOMWRTFormatPrettyPrint, true);
        serializer->write(doc, output_desc);

        delete file_target;
        xercesc::XMLPlatformUtils::Terminate();
        return false;
}

static void __edge_count_visitor(const struct bio_graph_vertex* v0, const struct bio_graph_vertex* v1, void* edge_num)
{
        (*static_cast<int*>(edge_num)) ++;
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
