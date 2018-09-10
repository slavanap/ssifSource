#!/bin/sh

cat result_0017512.txt | cut -f 2 | tr -d '\r\n' | base64 -d > out.png