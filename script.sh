#!/bin/bash

proj_dir=$(readlink -f $0 | xargs dirname)
echo $proj_dir

bin_dir=${proj_dir}/cmake-build-debug
result_dir=${proj_dir}/trace4_16kb_sparse_recipe_variableIOcap
cfgfile=${proj_dir}/configure

if [ ! -d ${result_dir} ]; then
    mkdir ${result_dir}
fi
run () {
    sed -i "33c IO_cap ${2}"  $1
    #sed -i "36c bit_num1 ${2}"  $1
    #sed -i "37c bit_num2 ${2}"  $1
    #sed -i "38c random_pick_ratio ${2}"  $1
    #sed -i "48c recipe_version_bound ${2}"  $1
    ${bin_dir}/configurable_dedup $1 |  tee ${result_dir}/${out_file}
}

for bit in 1 2 3 4 5 6 7 8 10 12 14 16 18 20 24; do
  out_file=trace4_16kb_sparse_recipe_IOcap${bit}
	run ${cfgfile} ${bit}
done
