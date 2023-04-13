.PHONY : all clean

HUFFMAN_DS := huffman_data_structures
LIB := lib$(HUFFMAN_DS).a

compile_curdir:
	@gcc --std=c11 -Wall -Wextra -pedantic -Wpedantic -Werror -Wshadow -c "$(CURDIR)"/*.c
compile_huffman_data_structures:
	@cd "$(HUFFMAN_DS)"; \
	gcc --std=c11 -Wall -Wextra -pedantic -Wpedantic -Werror -Wshadow -Ofast -fPIC -c *.c;
huffman_static_library: compile_huffman_data_structures 
	@cd "$(HUFFMAN_DS)"; \
	ar crs "$(LIB)" *.o; \
	mv "$(LIB)" "$(CURDIR)";
binary: huffman_static_library compile_curdir
	@gcc -Ofast -g -o "$(name)" *.c -L. -l"$(HUFFMAN_DS)"
all: clean huffman_static_library compile_curdir
	@gcc -Ofast -g -o huff *.c -L. -l"$(HUFFMAN_DS)"
clean:
	@rm "$(CURDIR)"/*.o 2>/dev/null || true
	@rm "$(CURDIR)"/*.a 2>/dev/null || true
	@rm "$(HUFFMAN_DS)"/*.o 2>/dev/null || true
	@rm "$(HUFFMAN_DS)"/*.a 2>/dev/null || true
