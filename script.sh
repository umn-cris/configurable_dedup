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
    #sync; echo 1 > /proc/sys/vm/drop_caches
    #sync; echo 2 > /proc/sys/vm/drop_caches
    #sync; echo 3 > /proc/sys/vm/drop_caches
    sed -i "33c IO_cap ${2}"  $1
    #sed -i "36c bit_num1 ${2}"  $1
    #sed -i "37c bit_num2 ${2}"  $1
    #sed -i "38c random_pick_ratio ${2}"  $1
    #sed -i "48c recipe_version_bound ${2}"  $1
    ${bin_dir}/configurable_dedup $1 |  tee ${result_dir}/${out_file}
}

for file in 1 2 3 5 6; do
        sed -i "53c dedup_trace_dir ./trace/trace${file}_16kb/" ${cfgfile}
        sed -i "54c trace_summary_file trace${file}_16kb.txt" ${cfgfile}
        result_dir=${proj_dir}/trace${file}_16kb_sort_cnr_variableIOcap
        if [ ! -d ${result_dir} ]; then
                mkdir ${result_dir}
        fi
        for bit in  4 6 20; do
                out_file=trace${file}_16kb_sort_cnr_IOcap${bit}
                run ${cfgfile} ${bit}
        done
done
