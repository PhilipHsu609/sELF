#include "elf_utils.h"

#include <assert.h>
#include <elf.h>
#include <stdio.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <ELF file>\n", argv[0]);
    return 1;
  }

  printf("Reading ELF file %s...\n", argv[1]);

  // Read ELF file and headers
  Elf_File *elf = elf_init(argv[1]);

  elf_read_strtab(elf);  // Read string tables
  elf_read_symtab(elf);  // Read symbol tables
  elf_read_dynamic(elf); // Read .dynamic section

  elf_print_headers(elf);  // Print ELF, program and section headers
  elf_print_sections(elf); // Print section names
  elf_print_symbols(elf);  // Print symbols

  elf_free(elf);
  return 0;
}
