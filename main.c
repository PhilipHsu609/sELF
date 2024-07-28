#include "elf_utils.h"

#include <assert.h>
#include <elf.h>
#include <stdio.h>

void woah() { printf("woah\n"); }

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <ELF file>\n", argv[0]);
    return 1;
  }

  printf("Reading ELF file %s...\n", argv[1]);

  Elf_File *elf = elf_init(argv[1]);

  assert(elf->ehdr->e_ehsize == sizeof(Elf64_Ehdr));
  assert(elf->ehdr->e_phentsize == sizeof(Elf64_Phdr));
  assert(elf->ehdr->e_shentsize == sizeof(Elf64_Shdr));

  printf("ELF header:\n");
  printf("\tType: %s\n", get_type(elf->ehdr));
  printf("\tMachine: %d\n", elf->ehdr->e_machine);
  printf("\tEntry point: 0x%lx\n", elf->ehdr->e_entry);

  elf_read_phdr(elf);     // Read program headers
  elf_read_shdr(elf);     // Read section headers
  elf_read_shstrtab(elf); // Read section header string table
  elf_read_strtab(elf);   // Read string table
  elf_read_symtab(elf);   // Read symbol table

  const int main_idx = elf_get_sym_idx(elf, "main");
  const int woah_idx = elf_get_sym_idx(elf, "woah");

  void *main_ptr = &main;
  unsigned long long base_addr =
      (unsigned long long)main_ptr - elf->symtab[main_idx].st_value;

  void (*woah_ptr)() = base_addr + (void (*)())elf->symtab[woah_idx].st_value;
  woah_ptr();

  elf_free(elf);
  return 0;
}
