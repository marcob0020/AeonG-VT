#!/bin/bash
update_num=320000 #$1 #320000
memgraph_binary=../../build/memgraph #$3 #/home/hjm/vldb/memgraph-master/build/memgraph

# # download T-mgBench
echo "Prepare datasets"
#download original dataset
mkdir -p ../datasets/T-mgBench
mgbench_download_dir="../datasets/T-mgBench"
echo "Download mgbench dataset completed."

#create graph operation
#echo "Create graph operation query statements"
prefix_path="../results/"
graph_op_path="$prefix_path/graph_op"
rm -rf "$graph_op_path"
mkdir -p "$graph_op_path"
update_num_arg="--num-op $update_num"
write_path="--write-path $prefix_path"
dataset_path="--dataset-path ../datasets/T-mgBench/"
python_script="../benchmarks/T-mgBench/create_graph_op_queries.py"
output=$(python3 "$python_script" $update_num_arg $dataset_path $write_path)


#Create AeonG temporal database, get graph operation latency, and get space
aeong_binary="--aeong-binary ../../build/memgraph"
client_binary="--client-binary ../../build/tests/mgbench/client"
number_workers="--num-workers 20"
rm -rf $prefix_path/database/aeong
mkdir -p $prefix_path/database/aeong
database_directory="--data-directory $prefix_path/database/aeong"
original_dataset="--original-dataset-cypher-path $mgbench_download_dir/cypher.cypher"
index_path="--index-cypher-path $mgbench_download_dir/cypher_index.cypher"
graph_op_cypher_path="--graph-operation-cypher-path $graph_op_path/cypher.txt"
python_script="../scripts/create_temporal_database.py"
echo "=============AeonG create database, it cost time==========="
echo "$python_script"
python3 "$python_script" $aeong_binary $client_binary $number_workers $database_directory $original_dataset $index_path $graph_op_cypher_path
