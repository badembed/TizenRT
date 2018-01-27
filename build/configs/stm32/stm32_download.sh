OS_DIR_PATH=${PWD}
BUILD_DIR_PATH=${OS_DIR_PATH}/../build
OUTPUT_BINARY_PATH=${BUILD_DIR_PATH}/output/bin

st-flash --reset write ${OUTPUT_BINARY_PATH}/tinyara.bin 0x08000000
