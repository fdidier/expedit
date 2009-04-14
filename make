#!/bin/sh
echo "g++ main.cpp screen.cpp term.cpp -o fe"
g++ main.cpp screen.cpp term.cpp -o fe
echo "To install, just copy ./fe where it can be found."
