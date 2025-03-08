#!/bin/bash
update_num=320000 #$1 #320000
clockg_binary=../../build/memgraph #$2 #/home/hjm/vldb/clockg/build/memgraph
memgraph_binary=../../build/memgraph #$3 #/home/hjm/vldb/memgraph-master/build/memgraph

# # download T-mgBench
echo "Prepare datasets"
#download original dataset
mkdir -p ../datasets/T-mgBench
mgbench_download_dir="../datasets/T-mgBench"
dataset_url="https://s3.eu-west-1.amazonaws.com/deps.memgraph.io/dataset/pokec/benchmark/pokec_small_import.cypher"
curl -o "$mgbench_download_dir/cypher.cypher" "$dataset_url"
index_url="https://s3.eu-west-1.amazonaws.com/deps.memgraph.io/dataset/pokec/benchmark/memgraph.cypher"
curl -o "$mgbench_download_dir/cypher_index.cypher" "$index_url"
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

#Generate data for VT
original_dataset="$mgbench_download_dir/cypher.cypher"
python_script="../scripts/ConvertFromNonVTtoVT.py"
echo "Generating VT inserts"
output=$(python3 "$python_script" $original_dataset)
echo $output
echo "Generated VT inserts"
mv indexNew.Cypher $mgbench_download_dir/cypherVT.cypher

#Create AeonG temporal database, get graph operation latency, and get space. Use INF VT
aeong_binary="--aeong-binary ../../build/memgraph"
client_binary="--client-binary ../../build/tests/mgbench/client"
number_workers="--num-workers 20"
rm -rf $prefix_path/database/aeong
mkdir -p $prefix_path/database/aeong
database_directory="--data-directory $prefix_path/database/aeong"
original_dataset="--original-dataset-cypher-path $mgbench_download_dir/cypherVT.cypher"
index_path="--index-cypher-path $mgbench_download_dir/cypher_index.cypher"
graph_op_cypher_path="--graph-operation-cypher-path $graph_op_path/cypher.txt"
python_script="../scripts/create_temporal_database.py"
echo "=============AeonG create database, it cost time==========="
output=$(python3 "$python_script" $aeong_binary $client_binary $number_workers $database_directory $original_dataset $index_path $graph_op_cypher_path)
graph_op_latency=$(echo "$output" | awk '{print $1}')
storage_consumption=$(echo "$output" | awk '{print $2}')
start_time=$(echo "$output" | awk '{print $3}')
end_time=$(echo "$output" | awk '{print $4}')
memory=$(echo "$output" | awk '{print $5}')
echo "=============AeonG graph operation latency & spance==========="
echo "graph_op_latency:$graph_op_latency"
echo "storage_consumption:$storage_consumption"
echo "main memory consumption:"$memory

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
output=$(python3 "$python_script" $aeong_binary $client_binary $number_workers $database_directory $original_dataset $index_path $graph_op_cypher_path)
graph_op_latency=$(echo "$output" | awk '{print $1}')
storage_consumption=$(echo "$output" | awk '{print $2}')
start_time=$(echo "$output" | awk '{print $3}')
end_time=$(echo "$output" | awk '{print $4}')
memory=$(echo "$output" | awk '{print $5}')
echo "=============AeonG graph operation latency & spance==========="
echo "graph_op_latency:$graph_op_latency"
echo "storage_consumption:$storage_consumption"
echo "main memory consumption:"$memory

#Generate data for VT
original_dataset="--original-dataset-cypher-path $mgbench_download_dir/cypher.cypher"
python_script="../scripts/ConvertFromNonVTtoVT.py"
echo "Generating VT inserts"
output=$(python3 "$python_script" $original_dataset)
mv indexNew.Cypher $mgbench_download_dir/cypherVT.cypher

#Create AeonG temporal database, get graph operation latency, and get space. Use INF VT
aeong_binary="--aeong-binary ../../build/memgraph"
client_binary="--client-binary ../../build/tests/mgbench/client"
number_workers="--num-workers 20"
rm -rf $prefix_path/database/aeong
mkdir -p $prefix_path/database/aeong
database_directory="--data-directory $prefix_path/database/aeong"
original_dataset="--original-dataset-cypher-path $mgbench_download_dir/cypherVT.cypher"
index_path="--index-cypher-path $mgbench_download_dir/cypher_index.cypher"
graph_op_cypher_path="--graph-operation-cypher-path $graph_op_path/cypher.txt"
python_script="../scripts/create_temporal_database.py"
echo "=============AeonG create database, it cost time==========="
output=$(python3 "$python_script" $aeong_binary $client_binary $number_workers $database_directory $original_dataset $index_path $graph_op_cypher_path)
graph_op_latency=$(echo "$output" | awk '{print $1}')
storage_consumption=$(echo "$output" | awk '{print $2}')
start_time=$(echo "$output" | awk '{print $3}')
end_time=$(echo "$output" | awk '{print $4}')
memory=$(echo "$output" | awk '{print $5}')
echo "=============AeonG graph operation latency & spance==========="
echo "graph_op_latency:$graph_op_latency"
echo "storage_consumption:$storage_consumption"
echo "main memory consumption:"$memory