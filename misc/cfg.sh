#!/bin/bash

type=${2:-pdf}
opt-15 -passes='default<O0>,dot-cfg' "$1".ll >/dev/null
dot -T $type .main.dot > "$1".$type
rm .main.dot

