SECTION_DATA_PROLOGUE(.blkdev.partiton,,SUBALIGN(4))
{
  __blkdev_partition_start = .;
  KEEP(*(SORT(.blkdev*)));
  __blkdev_partition_end = .;
} GROUP_LINK_IN(ROMABLE_REGION)