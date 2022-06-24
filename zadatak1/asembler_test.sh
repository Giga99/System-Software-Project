#!/bin/bash

./asembler -o ./tests/projmain.o ./tests/projmain.s
./asembler -o ./tests/projinterrupts.o ./tests/projinterrupts.s
./asembler -o ./tests/test_write_part1.o ./tests/test_write_part1.s
./asembler -o ./tests/test_write_part2.o ./tests/test_write_part2.s
