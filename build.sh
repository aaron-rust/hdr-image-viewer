#!/bin/bash

set -e

# Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel "$(nproc)"