#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint8_t tag_type;           // 1 字节
    uint32_t data_size;         // 3 字节（手动解析）
    uint32_t timestamp;         // 3 字节（手动解析）
    uint8_t timestamp_extended; // 1 字节
} FLVTagHeader;
uint32_t PreviousTagSize;
// 从字节数组中提取 24 位值
uint32_t read_24bit(uint8_t *buffer) {
    return (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
}

uint32_t read_flv_timestamps(const char *filename, int max_tags) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open FLV file");
        return -1;
    }

    // 跳过 FLV 文件头部 (9 字节 + 4 字节 PreviousTagSize0)
    fseek(file, 9 + 4, SEEK_SET);

    printf("Reading timestamps from FLV tags...\n");
    int typedex = 0;
    for (int i = 0; i < max_tags; ++i) {
        
        FLVTagHeader header;
        uint8_t buffer[11];

        // 读取 Tag Header (11 字节)
        if (fread(buffer, 1, 11, file) != 11) {
            printf("End of file reached or read error\n");
            break;
        }

        // 解析字段
        header.tag_type = buffer[0];
        header.data_size = read_24bit(&buffer[1]);
        header.timestamp = read_24bit(&buffer[4]);
        header.timestamp_extended = buffer[7];

        // 计算完整时间戳
        uint32_t full_timestamp = (header.timestamp_extended << 24) | header.timestamp;
       
        // 打印 Tag 信息
        if(header.tag_type==8||header.tag_type==9||header.tag_type==18){
            printf("Tag %d - Type: %u, Timestamp: %u ms\n", typedex++, header.tag_type, full_timestamp);
        }else{
            printf("error Tag- Type: %u, Timestamp: %u ms  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", header.tag_type, full_timestamp);
        }
       
        if(full_timestamp!=0){
           //return full_timestamp;
        }
        // 跳过 Tag 数据部分和 PreviousTagSize (4 字节)
        fseek(file, header.data_size + 4, SEEK_CUR);
        //  printf("===DataSize===: %u \n",header.data_size);
        // fseek(file, header.data_size, SEEK_CUR);
        // fread(&PreviousTagSize,4,1,file);
        // printf("===PreviousTagSize===: %u \n",PreviousTagSize);
    }

    fclose(file);
}
// 将 24 位值写入字节数组
void write_24bit(uint8_t *buffer, uint32_t value) {
    buffer[0] = (value >> 16) & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = value & 0xFF;
}

// 更新时间戳
void update_flv_timestamps(const char *input_file, const char *output_file, uint32_t new_timestamp) {
    FILE *infile = fopen(input_file, "rb");
    FILE *outfile = fopen(output_file, "wb");
    if (!infile || !outfile) {
        perror("Failed to open files");
        if (infile) fclose(infile);
        if (outfile) fclose(outfile);
        return;
    }

    // 缓冲区
    uint8_t buffer[11];
    uint8_t data_buffer[4096];
    size_t bytes_read;

    // 复制 FLV 文件头部
    fread(data_buffer, 1, 9 + 4, infile); // 文件头 (9 字节) + PreviousTagSize0 (4 字节)
    fwrite(data_buffer, 1, 9 + 4, outfile);
  
    while ((bytes_read = fread(buffer, 1, 11, infile)) == 11) { // 读取每个 Tag 的头部
        // 解析字段
        uint8_t tag_type = buffer[0];
        uint32_t data_size = read_24bit(&buffer[1]);
        uint32_t timestamp = read_24bit(&buffer[4]);
        uint8_t timestamp_extended = buffer[7];

        // 如果时间戳为 0 且类型为 8 或 9，则更新时间戳
        if (timestamp == 0 && (tag_type == 8 || tag_type == 9)) {
            printf("Updating Tag - Type: %u, Old Timestamp: %u ms\n", tag_type, timestamp);
            write_24bit(&buffer[4], new_timestamp & 0xFFFFFF); // 更新 timestamp
            buffer[7] = (new_timestamp >> 24) & 0xFF;         // 更新 timestamp_extended
        }

        // 写入更新后的头部到输出文件
        fwrite(buffer, 1, 11, outfile);
        // 复制 Tag 数据部分
        while (data_size > 0) {
            size_t to_read = (data_size < sizeof(data_buffer)) ? data_size : sizeof(data_buffer);
            bytes_read = fread(data_buffer, 1, to_read, infile);
            fwrite(data_buffer, 1, bytes_read, outfile);
            data_size -= bytes_read;
        }

        // 复制 PreviousTagSize
        fread(data_buffer, 1, 4, infile);
        fwrite(data_buffer, 1, 4, outfile);
       
    }

    fclose(infile);
    fclose(outfile);
}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <FLV file> [max_tags]\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    int max_tags = (argc >= 3) ? atoi(argv[2]) : 5; // 默认读取前 5 个 Tag

    uint32_t full_timestamp = read_flv_timestamps(filename, max_tags);
    // 更新文件中的时间戳
    //update_flv_timestamps(argv[1], "newfile2.flv", full_timestamp);

    return 0;
}
