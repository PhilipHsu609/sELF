#ifndef ELF_UTILS_H
#define ELF_UTILS_H

#include <assert.h>
#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  int fd;
  uint8_t *file;
  Elf64_Ehdr *ehdr;
  Elf64_Phdr *phdr;
  Elf64_Shdr *shdr;
  char *shstrtab;
  char *strtab;
  Elf64_Sym *symtab;
  int symtab_entries;
} Elf_File;

extern inline Elf_File *elf_init(const char *filename) {
  const int fd = open(filename, O_RDONLY);
  const off_t file_size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  Elf_File *elf = (Elf_File *)malloc(sizeof(Elf_File));
  elf->fd = fd;
  elf->file = (uint8_t *)malloc(file_size);
  elf->ehdr = (Elf64_Ehdr *)malloc(sizeof(Elf64_Ehdr));
  elf->phdr = NULL;
  elf->shdr = NULL;
  elf->shstrtab = NULL;
  elf->strtab = NULL;
  elf->symtab = NULL;
  elf->symtab_entries = 0;

  read(fd, elf->file, file_size);
  memcpy(elf->ehdr, elf->file, sizeof(Elf64_Ehdr));

  return elf;
}

extern inline void elf_free(Elf_File *elf) {
  close(elf->fd);
  free(elf->file);
  free(elf->ehdr);
  free(elf->phdr);
  free(elf->shdr);
  free(elf->shstrtab);
  free(elf->strtab);
  free(elf->symtab);
  free(elf);
}

extern inline int elf_get_section_idx(const Elf_File *elf,
                                      const char *section_name) {
  assert(elf->shstrtab != NULL && "elf_read_shstrtab must be called first");
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

extern inline void elf_read_phdr(Elf_File *elf) {
  const Elf64_Ehdr *ehdr = elf->ehdr;

  if (ehdr->e_phnum > 0) {
    elf->phdr = (Elf64_Phdr *)malloc(ehdr->e_phnum * sizeof(Elf64_Phdr));
    for (int i = 0; i < ehdr->e_phnum; i++) {
      memcpy(&elf->phdr[i], &elf->file[ehdr->e_phoff + i * sizeof(Elf64_Phdr)],
             sizeof(Elf64_Phdr));
    }
  }
}

extern inline void elf_read_shdr(Elf_File *elf) {
  const Elf64_Ehdr *ehdr = elf->ehdr;

  if (ehdr->e_shnum > 0) {
    elf->shdr = (Elf64_Shdr *)malloc(ehdr->e_shnum * sizeof(Elf64_Shdr));
    for (int i = 0; i < ehdr->e_shnum; i++) {
      memcpy(&elf->shdr[i], &elf->file[ehdr->e_shoff + i * sizeof(Elf64_Shdr)],
             sizeof(Elf64_Shdr));
    }
  }
}

extern inline void elf_read_shstrtab(Elf_File *elf) {
  const Elf64_Shdr *shdr = elf->shdr;
  const int shstrtab_idx = elf->ehdr->e_shstrndx;

  elf->shstrtab = (char *)malloc(shdr[shstrtab_idx].sh_size);
  memcpy(elf->shstrtab, &elf->file[shdr[shstrtab_idx].sh_offset],
         shdr[shstrtab_idx].sh_size);
}

extern inline void elf_read_strtab(Elf_File *elf) {
  const int strtab_idx = elf_get_section_idx(elf, ".strtab");
  elf->strtab = (char *)malloc(elf->shdr[strtab_idx].sh_size);
  elf_read_section(elf, strtab_idx, elf->strtab);
}

extern inline void elf_read_symtab(Elf_File *elf) {
  const int symtab_idx = elf_get_section_idx(elf, ".symtab");

  assert(elf->shdr[symtab_idx].sh_entsize == sizeof(Elf64_Sym));

  elf->symtab_entries = elf->shdr[symtab_idx].sh_size / sizeof(Elf64_Sym);
  elf->symtab = (Elf64_Sym *)malloc(elf->symtab_entries * sizeof(Elf64_Sym));
  elf_read_section(elf, symtab_idx, elf->symtab);
}

extern inline const char *get_class(const Elf64_Ehdr *ehdr) {
  switch (ehdr->e_ident[EI_CLASS]) {
  case ELFCLASS32:
    return "32-bit";
  case ELFCLASS64:
    return "64-bit";
  default:
    return "UNKNOWN";
  }
}

extern inline const char *get_data(const Elf64_Ehdr *ehdr) {
  switch (ehdr->e_ident[EI_DATA]) {
  case ELFDATA2LSB:
    return "2's complement, little endian";
  case ELFDATA2MSB:
    return "2's complement, big endian";
  default:
    return "UNKNOWN";
  }
}

extern inline const char *get_version(const Elf64_Ehdr *ehdr) {
  switch (ehdr->e_ident[EI_VERSION]) {
  case EV_CURRENT:
    return "1 (current)";
  default:
    return "UNKNOWN";
  }
}

extern inline const char *get_os_abi(const Elf64_Ehdr *ehdr) {
  switch (ehdr->e_ident[EI_OSABI]) {
  case ELFOSABI_NONE:
    return "UNIX System V ABI";
  case ELFOSABI_HPUX:
    return "HP-UX";
  case ELFOSABI_NETBSD:
    return "NetBSD";
  case ELFOSABI_LINUX:
    return "Linux";
  case ELFOSABI_SOLARIS:
    return "Sun Solaris";
  case ELFOSABI_AIX:
    return "AIX";
  case ELFOSABI_IRIX:
    return "IRIX";
  case ELFOSABI_FREEBSD:
    return "FreeBSD";
  case ELFOSABI_TRU64:
    return "Compaq TRU64 UNIX";
  case ELFOSABI_MODESTO:
    return "Novell Modesto";
  case ELFOSABI_OPENBSD:
    return "OpenBSD";
  case ELFOSABI_ARM:
    return "ARM";
  case ELFOSABI_STANDALONE:
    return "Standalone (embedded) application";
  default:
    return "UNKNOWN";
  }
}

extern inline int get_abi_version(const Elf64_Ehdr *ehdr) {
  return ehdr->e_ident[EI_ABIVERSION];
}

extern inline const char *get_type(const Elf64_Ehdr *ehdr) {
  switch (ehdr->e_type) {
  case ET_NONE:
    return "NONE (No file type)";
  case ET_REL:
    return "REL (Relocatable file)";
  case ET_EXEC:
    return "EXEC (Executable file)";
  case ET_DYN:
    return "DYN (Shared object file)";
  case ET_CORE:
    return "CORE (Core file)";
  default:
    return "UNKNOWN";
  }
}

extern inline const char *get_sh_type(const Elf64_Shdr *shdr) {
  switch (shdr->sh_type) {
  case SHT_NULL:
    return "NULL";
  case SHT_PROGBITS:
    return "PROGBITS";
  case SHT_SYMTAB:
    return "SYMTAB";
  case SHT_STRTAB:
    return "STRTAB";
  case SHT_RELA:
    return "RELA";
  case SHT_HASH:
    return "HASH";
  case SHT_DYNAMIC:
    return "DYNAMIC";
  case SHT_NOTE:
    return "NOTE";
  case SHT_NOBITS:
    return "NOBITS";
  case SHT_REL:
    return "REL";
  case SHT_SHLIB:
    return "SHLIB";
  case SHT_DYNSYM:
    return "DYNSYM";
  case SHT_INIT_ARRAY:
    return "INIT_ARRAY";
  case SHT_FINI_ARRAY:
    return "FINI_ARRAY";
  case SHT_PREINIT_ARRAY:
    return "PREINIT_ARRAY";
  case SHT_GROUP:
    return "GROUP";
  case SHT_SYMTAB_SHNDX:
    return "SYMTAB SECTION INDICIES";
  case SHT_NUM:
    return "NUM";
  case SHT_LOOS:
    return "LOOS";
  case SHT_GNU_ATTRIBUTES:
    return "GNU_ATTRIBUTES";
  case SHT_GNU_HASH:
    return "GNU_HASH";
  case SHT_GNU_LIBLIST:
    return "GNU_LIBLIST";
  case SHT_CHECKSUM:
    return "CHECKSUM";
  case SHT_LOSUNW:
    return "LOSUNW";
  case SHT_SUNW_COMDAT:
    return "SUNW_COMDAT";
  case SHT_SUNW_syminfo:
    return "SUNW_syminfo";
  case SHT_GNU_verdef:
    return "GNU_verdef";
  case SHT_GNU_verneed:
    return "GNU_verneed";
  case SHT_GNU_versym:
    return "GNU_versym";
  default:
    return "UNKNOWN";
  }
}

#endif // ELF_UTILS_H