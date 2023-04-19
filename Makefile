.PHONY : all clean

HUFFMAN_DS := huffman_data_structures
LIB := lib$(HUFFMAN_DS).a
project_root_dir:= "$(CURDIR)"
COMPILE_FLAGS = --std=c11 -Wall -Wextra -pedantic -Wpedantic -Werror -Wshadow -I"$(project_root_dir)"/common -c *.c

compile_project_root_dir:
	@gcc $(COMPILE_FLAGS)
compile_huffman_data_structures:
	@cd "$(HUFFMAN_DS)"; \
	gcc $(COMPILE_FLAGS)
huffman_static_library: compile_huffman_data_structures 
	@cd "$(HUFFMAN_DS)"; \
	ar crs "$(LIB)" *.o; \
	mv "$(LIB)" "$(CURDIR)";
debug: COMPILE_FLAGS += -DDEBUG -g
debug: huffman_static_library compile_project_root_dir
	@gcc -DDEBUG -g -o huff_debug *.c -L. -l"$(HUFFMAN_DS)" -I"$(project_root_dir)"/common
release: clean huffman_static_library compile_project_root_dir
	@gcc -Ofast -o huff *.c -L. -l"$(HUFFMAN_DS)" -I"$(project_root_dir)"/common
clean:
	@rm "$(CURDIR)"/*.o 2>/dev/null || true
	@rm "$(CURDIR)"/*.a 2>/dev/null || true
	@rm "$(HUFFMAN_DS)"/*.o 2>/dev/null || true
	@rm "$(HUFFMAN_DS)"/*.a 2>/dev/null || true
