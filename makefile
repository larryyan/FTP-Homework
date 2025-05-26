# 定义需要编译的子目录列表
SUBDIRS := C_ftp crawler

# 默认目标：编译所有子目录
all: $(SUBDIRS)

# 递归调用子目录的Makefile
$(SUBDIRS):
	$(MAKE) -C $@

# 清理所有子目录
clean:
	@for dir in $(SUBDIRS); do \
		echo "正在清理 $$dir..."; \
		$(MAKE) -C $$dir clean; \
	done

# 声明伪目标（避免与同名文件冲突）
.PHONY: all clean $(SUBDIRS)