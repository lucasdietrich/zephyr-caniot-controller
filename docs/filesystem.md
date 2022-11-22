# Filesystem

Following table describe filesystems available for the application:

| Filesystem | Description                 | nucleo_f429zi | qemu |
| ---------- | --------------------------- | ------------- | -------- |
| SD:        | SDMMC FAT32 - Persistent    | yes           | no       |
| RAM:       | RAM memory FAT32 - Volatile | (yes)         | yes      |

## Files structure

## File upload/download

Files in the filesystem can be accesses using the following API: `/files`.
