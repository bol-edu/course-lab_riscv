#!/bin/bash

make testbench.sim
if [ "$?" -ne "0" ]; then
  echo "Failed to compile."
  exit 1
fi

for benchmark in 'matmul4_4' \
                 'matmul8_8' \
                 'matmul16_16' \
                 'qsort_10' \
                 'qsort_50' \
                 'qsort_100' \
                 'qsort_500' \
                 'test_1'
do
  echo $benchmark
  ./testbench.sim benchmarks/correct/${benchmark}/
  if [ "$?" -ne "0" ]; then
    echo "Failed test ${benchmark}."
    exit 1
  fi
done
