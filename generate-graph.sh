#!/bin/bash

cd bin

valgrind --tool=callgrind ./text-search 
callgrind_annotate --auto=yes callgrind.out.*
~/Downloads/gprof2dot-2017.09.19/gprof2dot.py --format=callgrind --output=out.dot callgrind.out.*
dot -Tpng out.dot -o graph.png
xdg-open graph.png

cd ..
