#!/bin/bash

echo "Running Application..."

ORIG_DIR=$(pwd)
cd bin && ./rcadia
cd "$ORIG_DIR"

echo "Stop Application."
