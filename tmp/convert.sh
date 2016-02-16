#!/bin/sh

./bio-graph --convert ./gexf_graph/athal.gexf --output ./athal.txt
./list2leda.sh ./athal.txt>./networks/athal/athal.gw

./bio-graph --convert ./gexf_graph/cjejuni.gexf --output ./cjejuni.txt
./list2leda.sh ./cjejuni.txt>./networks/cjejuni/cjejuni.gw

./bio-graph --convert ./gexf_graph/dmel.gexf --output ./dmel.txt
./list2leda.sh ./dmel.txt>./networks/dmel/dmel.gw

./bio-graph --convert ./gexf_graph/ecoli.gexf --output ./ecoli.txt
./list2leda.sh ./ecoli.txt>./networks/ecoli/ecoli.gw

./bio-graph --convert ./gexf_graph/yeasthc.gexf --output ./yeasthc.txt
./list2leda.sh ./yeasthc.txt>./networks/yeasthc/yeasthc.gw

./bio-graph --convert ./gexf_graph/yeast05.gexf --output ./yeast05.txt
./list2leda.sh ./yeast05.txt>./networks/yeast05/yeast05.gw

./bio-graph --convert ./gexf_graph/yeast10.gexf --output ./yeast10.txt
./list2leda.sh ./yeast10.txt>./networks/yeast10/yeast10.gw

./bio-graph --convert ./gexf_graph/yeast15.gexf --output ./yeast15.txt
./list2leda.sh ./yeast15.txt>./networks/yeast15/yeast15.gw

./bio-graph --convert ./gexf_graph/yeast20.gexf --output ./yeast20.txt
./list2leda.sh ./yeast20.txt>./networks/yeast20/yeast20.gw
