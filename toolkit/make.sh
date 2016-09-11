#!/usr/bin/env bash

cd src
g++ -std=c++11 -o ../bin/preprint preprint.cpp $(Magick++-config --cppflags --cxxflags --ldflags --libs)

