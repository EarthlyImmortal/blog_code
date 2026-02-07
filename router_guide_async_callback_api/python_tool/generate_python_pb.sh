#!/bin/sh
mkdir -p python_pb

CURRENT_PATH=`pwd`
GRPC_PATH=${CURRENT_PATH}/../../grpc
GRPC_BIN_PATH=${GRPC_PATH}/bin
PROTOC_PATH=${GRPC_BIN_PATH}/protoc
PROTO_FILE_PATH=${CURRENT_PATH}/../proto
PYTHON_OUT_PATH=${CURRENT_PATH}/python_pb

echo "开始编译proto文件..."
for proto_file in "${PROTO_FILE_PATH}"/*.proto; do
    if [ -f "${proto_file}" ]; then
        filename=$(basename -- "${proto_file}")
        echo "正在编译: ${filename}"
        
        # 使用protoc编译proto文件
        "${PROTOC_PATH}" --proto_path="${PROTO_FILE_PATH}" \
                      --python_out="${PYTHON_OUT_PATH}" \
                      "${proto_file}"
        
        if [ $? -eq 0 ]; then
            echo "✓ 成功编译: $filename"
        else
            echo "✗ 编译失败: $filename"
        fi
    fi
done

