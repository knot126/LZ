#!/usr/bin/env bash

echo -- picolz --
clang -g -o picolz -DPLZ_TEST=1 picolz.c

echo -- describelz --
clang -g -o describelz -DPLZ_TEST=2 picolz.c
