# Walkthrough of FreeBSD 11's VFS's Subroutines

## Contents

1. Code Flow
2. Reading Checklist
3. Important Data Structures
4. Code Walkthrough

## Reading Checklist

This section lists the relevant functions for the walkthrough by filename,
where each function per filename is listed in the order that it is called.

The first '+' means that I have read the code or have a general idea of what it does.
The second '+' means that I have read the code closely and heavily commented it.
The third '+' means that I have added it to this document's code walkthrough.

```txt
File: vfs_subr.c
    vaccess             ++-
    vget                +--
    _vhold              +--
    vrele               ++-
    vputx               +--
    vinactive           +--
    _vdrop              ---

File: ufs_inode.c
    ufs_inactive        +--
```

## Important Data Structures

## General Overview (Code)

```c
```
