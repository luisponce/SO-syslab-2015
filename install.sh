#!/bin/bash

CUR_DIR="$(pwd)"
BIN_DIR="/bin"


echo "Installing..."

PATH=$PATH:$CUR_DIR$BIN_DIR
export PATH
echo $PATH

