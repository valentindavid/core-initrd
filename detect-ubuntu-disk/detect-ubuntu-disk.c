#include <blkid/blkid.h>
#include <unicode/utf8.h>
#include <unicode/utf16.h>
#include <unicode/uchar.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <errno.h>

int get_loader_part_uuid(const char** uuid) {
  struct stat st;
  uint16_t* original = NULL;
  char* ret_value = NULL;
  int ret = 0;
  FILE *file = NULL;
  size_t out_size;
  size_t out_pos;
  size_t pos;

  file = fopen("/sys/firmware/efi/efivars/LoaderDevicePartUUID-4a67b082-0a4c-41cf-b6c7-440b29bb8c4f", "rb");

  if (file == NULL) {
    if (errno == ENOENT) {
      ret = 1;
    } else {
      perror("detect-ubuntu-disk");
    }
    goto exit;
  }

  if (fstat(fileno(file), &st) != 0) {
    perror("detect-ubuntu-disk");
    goto exit;
  }

  original = malloc(2*((st.st_size+1)/2));
  if (!original) {
    perror("detect-ubuntu-disk");
    goto exit;
  }

  if (fread(original, 1, st.st_size, file) != (size_t)st.st_size) {
    perror("detect-ubuntu-disk");
    goto exit;
  }

  out_size = st.st_size + 4;
  ret_value = malloc(out_size);
  if (!ret_value) {
    perror("detect-ubuntu-disk");
    goto exit;
  }

  out_pos = 0;
  for (pos = 2; pos < (size_t)((st.st_size+1)/2); ) {
    UChar32 c;
    int error=0;
    U16_NEXT(original, pos, ((st.st_size+1)/2), c);
    U8_APPEND(ret_value, out_pos, out_size, u_tolower(c), error);
    if (error) {
      free(ret_value);
      ret_value = NULL;
      goto exit;
    }
  }
  ret = 1;

 exit:
  if (original)
    free(original);
  if (file)
    fclose(file);
  *uuid = ret_value;
  return ret;
}

int is_gpt(blkid_probe probe) {
  const char* value;
  size_t value_len;
  int res;

  res = blkid_probe_lookup_value(probe, "PTTYPE", &value, &value_len);

  if (res < 0)
    return 0;

  if (value_len != 4)
    return 0;

  return 0 == strcmp("gpt", value);
}

int main(int argc, char* argv[]) {
  blkid_probe probe = blkid_new_probe_from_filename(getenv("DEVNAME"));
  if (probe == NULL) {
    perror("detect-ubuntu-disk");
    return 1;
  }

  blkid_probe_enable_partitions(probe, 1);
  blkid_probe_set_partitions_flags(probe, BLKID_PARTS_ENTRY_DETAILS);
  blkid_probe_enable_superblocks(probe, 1);

  if (blkid_do_safeprobe(probe) < 0) {
    perror("detect-ubuntu-disk");
    return 1;
  }

  if (!is_gpt(probe)) {
    return 1;
  }

  blkid_partlist partitions = blkid_probe_get_partitions(probe);
  if (partitions == NULL) {
    perror("detect-ubuntu-disk");
    return 1;
  }

  size_t npartitions = blkid_partlist_numof_partitions(partitions);
  size_t i;

  const char* loader_uuid;
  if (!get_loader_part_uuid(&loader_uuid)) {
    return 1;
  }

  int is_boot_disk = 0;
  for (i = 0; i < npartitions; ++i) {
    blkid_partition partition;
    partition = blkid_partlist_get_partition(partitions, i);
    const char* type = blkid_partition_get_type_string(partition);
    const char* label = blkid_partition_get_name(partition);
    const char* uuid = blkid_partition_get_uuid(partition);

    if ((type == NULL) || (label == NULL) || (uuid == NULL))
      continue ;

    if ((0 == strcmp("c12a7328-f81f-11d2-ba4b-00a0c93ec93b", type))
        && (0==strcmp(label, "ubuntu-seed"))) {
      if (!loader_uuid || (0 == strcmp(loader_uuid, uuid))) {
        is_boot_disk = 1;
      }
    } else if (label && (0==strcmp(label, "ubuntu-boot"))) {
      if (!loader_uuid || (0 == strcmp(loader_uuid, uuid))) {
        is_boot_disk = 1;
      }
    }
  }

  if (is_boot_disk) {
    printf("UBUNTU_CORE_BOOT_DISK=1\n");
  }

  return 0;
}
