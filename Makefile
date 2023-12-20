COMPILER = gcc       
COMPILER_FLAGS = -Wall -g -std=c99 -c -D_GNU_SOURCE
LINKER = gcc -pthread


#======== for "make all" =========================================================================
all: paster2.out #all executables should be listed here



#======= for each executable, link together relevant object files and libraries==================
#target: ingredients
	#recipe

paster2.out: paster2.o stack.o decompress.o compress.o crc.o zutil.o header.o
	$(LINKER) -o paster2.out 	paster2.o stack.o decompress.o compress.o crc.o zutil.o header.o -lz -lcurl

#more executables ...
  
  
  
  
#======= for each object file, compile it from source  ===========================================
#target: ingredients
	#recipe

paster2.o: ./paster2.c
	$(COMPILER) $(COMPILER_FLAGS) -o paster2.o       ./paster2.c 

stack.o: ./stack.c ./stack.h
	$(COMPILER) $(COMPILER_FLAGS) -o stack.o       ./stack.c 

decompress.o: ./decompress.c ./decompress.h
	$(COMPILER) $(COMPILER_FLAGS) -o decompress.o ./decompress.c

compress.o: ./compress_and_concat.c ./compress_and_concat.h
	$(COMPILER) $(COMPILER_FLAGS) -o compress.o        ./compress_and_concat.c

crc.o: ./crc.c ./crc.h
	$(COMPILER) $(COMPILER_FLAGS) -o crc.o          ./crc.c 

zutil.o: ./zutil.c ./zutil.h 
	$(COMPILER) $(COMPILER_FLAGS) -o zutil.o        ./zutil.c 

header.o: ./main_write_header_cb.c ./main_write_header_cb.h
	$(COMPILER) $(COMPILER_FLAGS) -o header.o       ./main_write_header_cb.c 

#======== for "make clean" ========================================================================
.PHONY: clean
clean:
	rm -f *.d *.o *.out



