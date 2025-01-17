#ifndef ELF_UTILS_H
#define ELF_UTILS_H

#include <assert.h>
#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HEADER_LIMIT 3

typedef struct {
  int fd;
  uint8_t *file;
  // headers
  Elf64_Ehdr *ehdr;
  Elf64_Phdr *phdr;
  Elf64_Shdr *shdr;
  // sections - string tables
  char *shstrtab;
  char *strtab;
  char *dynstr;
  // sections - symbol tables
  Elf64_Sym *symtab;
  int symtab_entries;
  Elf64_Sym *dynsym;
  int dynsym_entries;
  // sections - dynamic section
  Elf64_Dyn *dynamic;
  int dynamic_entries;
  // sections - relocation section
} Elf_File;

extern inline Elf_File *elf_init(const char *filename) {
  const int fd = open(filename, O_RDONLY);
  const off_t file_size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  Elf_File *elf = (Elf_File *)malloc(sizeof(Elf_File));
  elf->fd = fd;
  elf->file = (uint8_t *)malloc(file_size);
  // headers
  elf->ehdr = (Elf64_Ehdr *)malloc(sizeof(Elf64_Ehdr));
  elf->phdr = NULL;
  elf->shdr = NULL;
  // sections - string tables
  elf->shstrtab = NULL;
  elf->strtab = NULL;
  elf->dynstr = NULL;
  // sections - symbol tables
  elf->symtab = NULL;
  elf->symtab_entries = 0;
  elf->dynsym = NULL;
  elf->dynsym_entries = 0;
  // sections - dynamic section
  elf->dynamic = NULL;
  elf->dynamic_entries = 0;

  read(fd, elf->file, file_size);

  // ELF header
  memcpy(elf->ehdr, elf->file, sizeof(Elf64_Ehdr));

  const Elf64_Ehdr *ehdr = elf->ehdr;

  // Program headers
  if (ehdr->e_phnum > 0) {
    elf->phdr = (Elf64_Phdr *)malloc(ehdr->e_phnum * sizeof(Elf64_Phdr));
    for (int i = 0; i < ehdr->e_phnum; i++) {
      memcpy(&elf->phdr[i], &elf->file[ehdr->e_phoff + i * sizeof(Elf64_Phdr)],
             sizeof(Elf64_Phdr));
    }
  }

  // Section headers
  if (ehdr->e_shnum > 0) {
    elf->shdr = (Elf64_Shdr *)malloc(ehdr->e_shnum * sizeof(Elf64_Shdr));
    for (int i = 0; i < ehdr->e_shnum; i++) {
      memcpy(&elf->shdr[i], &elf->file[ehdr->e_shoff + i * sizeof(Elf64_Shdr)],
             sizeof(Elf64_Shdr));
    }
  }

  return elf;
}

extern inline void elf_free(Elf_File *elf) {
  close(elf->fd);
  free(elf->file);
  // headers
  free(elf->ehdr);
  free(elf->phdr);
  free(elf->shdr);
  // sections - string tables
  free(elf->shstrtab);
  free(elf->strtab);
  free(elf->dynstr);
  // sections - symbol tables
  free(elf->symtab);
  free(elf->dynsym);
  // sections - dynamic section
  free(elf->dynamic);
  free(elf);
}

extern inline int elf_get_section_idx(const Elf_File *elf,
                                      const char *section_name) {
  assert(elf->shstrtab != NULL && "elf_read_strtab must be called first");
  for (int i = 0; i < elf->ehdr->e_shnum; i++) {
    if (strcmp(&elf->shstrtab[elf->shdr[i].sh_name], section_name) == 0) {
      return i;
    }
  }
  return -1;
}

extern inline void elf_read_section(const Elf_File *elf, const int idx,
                                    void *buf) {
  const Elf64_Shdr *shdr = elf->shdr;
  memcpy(buf, &elf->file[shdr[idx].sh_offset], shdr[idx].sh_size);
}

extern inline int elf_get_sym_idx(const Elf_File *elf, const char *sym_name) {
  assert(elf->strtab != NULL && "elf_read_strtab must be called first");
  assert(elf->symtab != NULL && "elf_read_symtab must be called first");

  for (int i = 0; i < elf->symtab_entries; i++) {
    if (strcmp(sym_name, &elf->strtab[elf->symtab[i].st_name]) == 0) {
      return i;
    }
  }

  return -1;
}

extern inline void elf_read_strtab(Elf_File *elf) {
  const Elf64_Shdr *shdr = elf->shdr;

  // .shstrtab
  const int shstrtab_idx = elf->ehdr->e_shstrndx;
  elf->shstrtab = (char *)malloc(shdr[shstrtab_idx].sh_size);
  memcpy(elf->shstrtab, &elf->file[shdr[shstrtab_idx].sh_offset],
         shdr[shstrtab_idx].sh_size);

  // .strtab
  const int strtab_idx = elf_get_section_idx(elf, ".strtab");
  if (strtab_idx != -1) {
    elf->strtab = (char *)malloc(elf->shdr[strtab_idx].sh_size);
    elf_read_section(elf, strtab_idx, elf->strtab);
  }

  // .dynstr
  const int dynstr_idx = elf_get_section_idx(elf, ".dynstr");
  if (dynstr_idx != -1) {
    elf->dynstr = (char *)malloc(elf->shdr[dynstr_idx].sh_size);
    elf_read_section(elf, dynstr_idx, elf->dynstr);
  }
}

extern inline void elf_read_symtab(Elf_File *elf) {
  const int symtab_idx = elf_get_section_idx(elf, ".symtab");
  if (symtab_idx != -1) {
    elf->symtab_entries = elf->shdr[symtab_idx].sh_size / sizeof(Elf64_Sym);
    elf->symtab = (Elf64_Sym *)malloc(elf->symtab_entries * sizeof(Elf64_Sym));
    elf_read_section(elf, symtab_idx, elf->symtab);
  }

  const int dynsym_idx = elf_get_section_idx(elf, ".dynsym");
  if (dynsym_idx != -1) {
    elf->dynsym_entries = elf->shdr[dynsym_idx].sh_size / sizeof(Elf64_Sym);
    elf->dynsym = (Elf64_Sym *)malloc(elf->dynsym_entries * sizeof(Elf64_Sym));
    elf_read_section(elf, dynsym_idx, elf->dynsym);
  }
}

extern inline void elf_read_dynamic(Elf_File *elf) {
  const int dynamic_idx = elf_get_section_idx(elf, ".dynamic");
  if (dynamic_idx != -1) {
    elf->dynamic_entries = elf->shdr[dynamic_idx].sh_size / sizeof(Elf64_Dyn);
    elf->dynamic =
        (Elf64_Dyn *)malloc(elf->dynamic_entries * sizeof(Elf64_Dyn));
    elf_read_section(elf, dynamic_idx, elf->dynamic);
  }
}

extern inline void elf_print_headers(const Elf_File *elf) {
  printf("Printing *PART* of ELF headers...\n");

  const Elf64_Ehdr *ehdr = elf->ehdr;
  printf("ELF header:\n");
  printf("  e_type: %d\n", ehdr->e_type);
  printf("  e_machine: %d\n", ehdr->e_machine);
  printf("  e_entry: %lx\n", ehdr->e_entry);

  const Elf64_Phdr *phdr = elf->phdr;
  printf("Program headers (first %d headers):\n", HEADER_LIMIT);
  if (phdr != NULL) {
    int limit = ehdr->e_phnum > HEADER_LIMIT ? HEADER_LIMIT : ehdr->e_phnum;
    for (int i = 0; i < limit; i++) {
      printf("  header %d\n", i);
      printf("    p_offset: %lx\n", phdr[i].p_offset);
      printf("    p_type: %d\n", phdr[i].p_type);
      printf("    p_vaddr: %lx\n", phdr[i].p_vaddr);
      printf("    p_filesz: %lx\n", phdr[i].p_filesz);
      printf("    p_memsz: %lx\n", phdr[i].p_memsz);
    }
  } else {
    printf("  No program headers\n");
  }

  const Elf64_Shdr *shdr = elf->shdr;
  printf("Section headers (first %d headers):\n", HEADER_LIMIT);
  if (shdr != NULL) {
    int limit = ehdr->e_shnum > HEADER_LIMIT ? HEADER_LIMIT : ehdr->e_shnum;
    for (int i = 0; i < limit; i++) {
      printf("  %s\n", &elf->shstrtab[shdr[i].sh_name]);
      printf("    sh_name: %d\n", shdr[i].sh_name);
      printf("    sh_type: %d\n", shdr[i].sh_type);
      printf("    sh_offset: %lx\n", shdr[i].sh_offset);
      printf("    sh_size: %lx\n", shdr[i].sh_size);
    }
  } else {
    printf("  No section headers\n");
  }
}

extern inline void elf_print_sections(const Elf_File *elf) {
  printf("Printing section names...\n");

  printf(".shstrtab:\n");
  if (elf->shstrtab != NULL) {
    for (int i = 0; i < elf->ehdr->e_shnum; i++) {
      printf("  %d: %s\n", i, &elf->shstrtab[elf->shdr[i].sh_name]);
    }
  } else {
    printf("  No .shstrtab\n");
  }
}

extern inline void elf_print_symbols(const Elf_File *elf) {
  printf("Printing symbols...\n");

  printf(".symtab:\n");
  if (elf->symtab != NULL) {
    for (int i = 0; i < elf->symtab_entries; i++) {
      printf("  %d: %s\n", i, &elf->strtab[elf->symtab[i].st_name]);
    }
  } else {
    printf("  No .symtab\n");
  }

  printf(".dynsym:\n");
  if (elf->dynsym != NULL) {
    for (int i = 0; i < elf->dynsym_entries; i++) {
      printf("  %d: %s\n", i, &elf->dynstr[elf->dynsym[i].st_name]);
    }
  } else {
    printf("  No .dynsym\n");
  }
}

#endif // ELF_UTILS_H