CC = gcc
CFLAGS = -Wall -Wextra -g
LIBS = -lpthread

# 目录定义
INCLUDE_DIR = include
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = .

# 源文件和目标文件
SOURCES = main.c $(SRC_DIR)/priority_threadpool.c
OBJECTS = $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.o)))
TARGET = $(BIN_DIR)/priority_threadpool_app

# 默认目标
all: $(TARGET)

# 生成可执行文件
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo "[✓] 编译成功: $@"

# 生成目标文件
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@
	@echo "[✓] 编译: $<"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@
	@echo "[✓] 编译: $<"

# 运行程序
run: $(TARGET)
	@echo "\n========== 运行优先级线程池演示 ==========\n"
	./$(TARGET)

# 清理生成的文件
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	@echo "[✓] 清理完成"

# 再次清理并编译
rebuild: clean all

.PHONY: all run clean rebuild
